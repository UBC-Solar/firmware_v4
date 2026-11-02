# Digi XBee source archive

Upstream: https://github.com/digidotcom/xbee_ansic_library
Pinned revision: `66603bd42157a9de73899c00d30bfdd5e968d661`.

`xbee_ansic.tar.gz` contains unmodified upstream headers, README (including license
notice), and the three sources TEL builds: `xbee_device.c`, `xbee_ipv4.c`, and
`swapbytes.c`. Source copyright/license notices are preserved. Digi code is MPL
2.0; `include/jslong.h` carries its own MPL 1.1 notice. See the notices in each file.

CMake verifies SHA-256 and extracts into the build directory. No network or git
submodules are needed. The archive keeps library files out of the application
source tree. It is a subset archive, not the original GitHub download.
