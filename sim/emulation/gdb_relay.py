"""Let cortex-debug's Live Watch work with Renode: share Renode's one GDB connection between two gdbs.

Renode's GDB server serves a single connection, but answers memory reads while the CPU runs.
cortex-debug's Live Watch opens a second gdb to poll variables, so this relay listens where VS Code
connects and keeps the only connection to Renode:

  main gdb (first to connect)  -->  relay :3333  -->  Renode :3334
  Live Watch gdb (any later)   -->

GDB's remote protocol is request/reply, so the relay sends one request upstream at a time and
returns the reply to whoever asked. The exception is a continue/step from the main gdb: its reply
(a stop packet: T05..., S05, W00, O<hex> console output) arrives whenever the CPU stops, while Live
Watch reads keep flowing. Stop packets never look like a read reply (hex or Exx), which is how the
relay tells them apart. Live Watch gdbs are read/write-memory only: anything that would run, stop,
detach or set breakpoints is answered by the relay and never reaches Renode.

Serves one debug session: exits when the main gdb disconnects (Renode is restarted per session).
Usage: python3 gdb_relay.py [--listen 3333 | --listen-fd N] [--target 3334] [-v]
"""

import argparse
import re
import selectors
import socket
import sys
import time

RUN_PREFIXES = (b"c", b"C", b"s", b"S", b"vCont;", b"vRun", b"vAttach")
ASYNC_STOP = re.compile(rb"[TSWX][0-9a-fA-F]{2}|O[0-9a-fA-F]+$")
LIVE_REFUSED = (b"Z", b"z", b"G", b"P", b"R", b"r", b"vKill", b"qRcmd")


def is_run(content):
    return content.startswith(RUN_PREFIXES)


def frame(content):
    return b"$%s#%02x" % (content, sum(content) & 0xFF)


class Conn:
    """One TCP side. Splits the byte stream into acks (+/-), interrupts (0x03) and $...#xx packets."""

    def __init__(self, sock, name):
        self.sock = sock
        self.name = name
        self.buf = b""
        self.open = True

    def items(self):
        while self.buf:
            head = self.buf[:1]
            if head in (b"+", b"-", b"\x03"):
                self.buf = self.buf[1:]
                yield head, None
                continue
            start = self.buf.find(b"$")
            if start < 0:
                self.buf = b""
                return
            end = self.buf.find(b"#", start)
            if end < 0 or len(self.buf) < end + 3:
                self.buf = self.buf[start:]
                return
            content = self.buf[start + 1 : end]
            self.buf = self.buf[end + 3 :]
            yield b"$", content

    def send(self, data):
        if self.open:
            try:
                self.sock.sendall(data)
            except OSError:
                self.open = False


class Relay:
    def __init__(self, server, target_port, verbose):
        self.target_port = target_port
        self.verbose = verbose
        self.sel = selectors.DefaultSelector()
        self.server = server
        self.sel.register(self.server, selectors.EVENT_READ, None)
        self.upstream = None
        self.main = None
        self.live = []
        self.queue = []  # (client, content), main's requests first
        self.inflight = None  # client waiting for the reply to the request sent upstream
        self.inflight_req = b""
        self.running = False  # main gdb's continue/step is in progress
        self.last_up = b""
        listen_port = server.getsockname()[1]
        log("gdb relay: listening on :%d, Renode on :%d" % (listen_port, target_port))

    def trace(self, who, content):
        if self.verbose:
            log("  %-6s %s" % (who, content[:80].decode(errors="replace")))

    # --- connections -------------------------------------------------------------------------

    def accept(self):
        sock, _ = self.server.accept()
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        if self.main is None:
            self.connect_upstream()
            conn = self.main = Conn(sock, "main")
        else:
            conn = Conn(sock, "live%d" % (len(self.live) + 1))
            self.live.append(conn)
        self.sel.register(sock, selectors.EVENT_READ, conn)
        log("gdb relay: %s gdb connected" % conn.name)

    def connect_upstream(self):
        deadline = time.time() + 30
        while True:
            try:
                sock = socket.create_connection(("127.0.0.1", self.target_port), timeout=1)
                break
            except OSError:
                if time.time() > deadline:
                    raise SystemExit("gdb relay: Renode GDB server not reachable on :%d" % self.target_port)
                time.sleep(0.2)
        sock.settimeout(None)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.upstream = Conn(sock, "renode")
        self.sel.register(sock, selectors.EVENT_READ, self.upstream)

    def close(self, conn):
        conn.open = False
        self.sel.unregister(conn.sock)
        conn.sock.close()
        if conn is self.main or conn is self.upstream:
            log("gdb relay: %s disconnected, exiting" % conn.name)
            raise SystemExit(0)
        self.live.remove(conn)
        self.queue = [(c, p) for c, p in self.queue if c is not conn]
        log("gdb relay: %s gdb disconnected" % conn.name)

    # --- routing -----------------------------------------------------------------------------

    def from_client(self, conn, kind, content):
        if kind == b"\x03":
            self.trace(conn.name + ">", b"<interrupt 0x03>")
            if conn is self.main:
                self.upstream.send(b"\x03")  # interrupt: its stop reply arrives as an async stop
            return
        if kind != b"$":
            return  # client acks; the relay acks for everyone itself
        conn.send(b"+")
        self.trace(conn.name + ">", content)
        if content.startswith(b"QStartNoAckMode"):
            conn.send(frame(b""))  # keep acks on: the relay relies on them to frame both sides
            return
        if conn is not self.main:
            reply = self.live_reply(content)
            if reply is not None:
                conn.send(frame(reply))
                if content[:1] in (b"D", b"k"):
                    self.close(conn)
                return
            self.queue.append((conn, content))
        else:
            # Main goes ahead of any queued Live Watch reads.
            n = sum(1 for c, _ in self.queue if c is self.main)
            self.queue.insert(n, (conn, content))
        self.pump()

    @staticmethod
    def live_reply(content):
        """Answer a Live Watch request locally, or None to forward it to Renode."""
        if content == b"?":
            return b"S05"  # "stopped": lets the Live Watch gdb read memory without interrupting
        if content == b"!" or content.startswith(b"H"):
            return b"OK"
        if content[:1] in (b"D", b"k"):
            return b"OK"
        if is_run(content) or content.startswith(LIVE_REFUSED):
            return b"E01"
        return None

    def pump(self):
        while self.inflight is None and self.queue:
            conn, content = self.queue.pop(0)
            if not conn.open:
                continue
            self.last_up = frame(content)
            self.upstream.send(self.last_up)
            if conn is self.main and is_run(content):
                self.running = True
            else:
                self.inflight = conn
                self.inflight_req = content

    def from_upstream(self, kind, content):
        if kind == b"-":
            self.trace("renode", b"<nak, resending>")
            self.upstream.send(self.last_up)
            return
        if kind != b"$":
            return
        self.upstream.send(b"+")
        if ASYNC_STOP.match(content):
            if self.running:
                self.trace("<main", content)
                self.main.send(frame(content))
                if not content.startswith(b"O"):
                    self.running = False
                self.pump()
                return
            if not (self.inflight is self.main and self.inflight_req == b"?"):
                # Renode sends an unrequested S05 on connect and after every T05 breakpoint stop.
                # A direct gdb discards them; forwarded, they'd be taken as the next request's reply.
                self.trace("drop", content)
                return
        if self.inflight is None:
            self.trace("drop", content)
            return
        target, self.inflight = self.inflight, None
        self.trace("<" + target.name, content)
        target.send(frame(content))
        self.pump()

    def serve(self):
        while True:
            for key, _ in self.sel.select():
                conn = key.data
                if conn is None:
                    self.accept()
                    continue
                try:
                    data = conn.sock.recv(65536)
                except OSError:
                    data = b""
                if not data:
                    self.close(conn)
                    continue
                conn.buf += data
                for kind, content in list(conn.items()):
                    if conn is self.upstream:
                        self.from_upstream(kind, content)
                    elif conn.open:
                        self.from_client(conn, kind, content)


def log(msg):
    print(msg, file=sys.stderr, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--listen", type=int, default=3333, help="port VS Code connects to")
    parser.add_argument("--target", type=int, default=3334, help="Renode's GDB server port")
    parser.add_argument("--listen-fd", type=int, help="serve an already-listening socket (Board)")
    parser.add_argument("-v", "--verbose", action="store_true", help="print every packet")
    args = parser.parse_args()
    if args.listen_fd is not None:
        server = socket.socket(fileno=args.listen_fd)
    else:
        server = socket.create_server(("127.0.0.1", args.listen))
    Relay(server, args.target, args.verbose).serve()


if __name__ == "__main__":
    main()
