"""
UBC Solar — Real-Time Cruise Control Simulator
===============================================
Calls your ACTUAL compiled C via ctypes. Runs in real time.

Setup:
    gcc -shared -fPIC -O2 -I. -include cruise_sim_stubs.h \\
        cruise_control_test.c accel_driver_test.c cruise_sim_exports.c \\
        -o cruise_test.so -lm

    python3 sim.py

Drag SETPOINT while running to see DAC spike then settle.
"""

import ctypes, os, sys, time
import numpy as np
import matplotlib
matplotlib.use("MacOSX")   # Mac: "MacOSX" | Linux/Win: "TkAgg" or "Qt5Agg"
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.animation as animation
from matplotlib.widgets import Slider, Button

# ── Load compiled C ───────────────────────────────────────────────────────────
SO_PATH = os.path.join(os.path.dirname(__file__), "cruise_test.so")
if not os.path.exists(SO_PATH):
    sys.exit(
        "cruise_test.so not found. Compile first:\n"
        "  gcc -shared -fPIC -O2 -I. -include cruise_sim_stubs.h \\\n"
        "      cruise_control_test.c accel_driver_test.c cruise_sim_exports.c \\\n"
        "      -o cruise_test.so -lm"
    )

lib = ctypes.CDLL(SO_PATH)
lib.sim_reset.argtypes           = [ctypes.c_float, ctypes.c_float]
lib.sim_reset.restype            = None
lib.sim_set_cruise_setpoint.argtypes = [ctypes.c_float]
lib.sim_set_cruise_setpoint.restype  = None
lib.sim_inject_imu.argtypes      = [ctypes.c_float, ctypes.c_float, ctypes.c_float]
lib.sim_inject_imu.restype       = None
lib.sim_velocity_set_ms.argtypes = [ctypes.c_float]
lib.sim_velocity_set_ms.restype  = ctypes.c_float
lib.sim_get_accel.restype        = ctypes.c_float
lib.sim_get_est_velocity.restype = ctypes.c_float
lib.sim_cruise_dac.restype       = ctypes.c_uint16
g_sim_velocity_ms = ctypes.c_float.in_dll(lib, "g_sim_velocity_ms")

# ── Timing ────────────────────────────────────────────────────────────────────
FSM_MS          = 25        # osDelay(DRIVE_STATE_FSM_DELAY)
PHYSICS_DT      = 0.1       # 1/CONTROL_FREQUENCY_HZ
TICK_DT         = FSM_MS / 1000.0
WINDOW_S        = 20.0      # rolling window shown on screen
WINDOW_TICKS    = int(WINDOW_S / TICK_DT)
TICKS_PER_FRAME = 2         # task ticks per animation frame
ANIM_MS         = 50        # ~20fps — reliable on all backends
CRUISE_MIN      = 6.94
CRUISE_MAX      = 22.22

# ── Buffers — pre-filled with real values so axes are never nan ───────────────
# t starts at -WINDOW_S so the first real data appears at the right edge
def make_buffers():
    t_init = np.linspace(-WINDOW_S, 0, WINDOW_TICKS)
    bufs = {
        "t":     list(t_init),
        "v":     [CRUISE_MIN]   * WINDOW_TICKS,
        "sp":    [CRUISE_MIN]   * WINDOW_TICKS,
        "dac":   [0]            * WINDOW_TICKS,
        "accel": [0.0]          * WINDOW_TICKS,
        "error": [0.0]          * WINDOW_TICKS,
        "imuax": [0.0]          * WINDOW_TICKS,
        "imuaz": [9.81]         * WINDOW_TICKS,
    }
    return bufs

buf = make_buffers()

# ── Task state ────────────────────────────────────────────────────────────────
S = dict(
    tick=0, mc_count=0, vel=CRUISE_MIN, elapsed=0.0,
    brake=False, paused=False, setpoint=11.11, hill=0.0,
    wall_start=time.time(),
)

def reset_task(sp):
    global buf
    buf = make_buffers()
    S.update(tick=0, mc_count=0, vel=CRUISE_MIN, elapsed=0.0,
             brake=False, setpoint=sp, wall_start=time.time())
    lib.sim_reset(ctypes.c_float(CRUISE_MIN), ctypes.c_float(sp))
    lib.sim_set_cruise_setpoint(ctypes.c_float(sp))
    g_sim_velocity_ms.value = CRUISE_MIN

def imu_sample():
    r  = np.deg2rad(S["hill"])
    ax = 9.81 * np.sin(r) + np.random.normal(0, sl_na.val)
    az = 9.81 * np.cos(r) + np.random.normal(0, sl_na.val)
    gy = np.random.normal(0, sl_ng.val)
    return float(ax), float(az), float(gy)

def task_tick():
    """One 25ms TasksDriveState iteration — calls real compiled C."""
    ax, az, gy = imu_sample()
    lib.sim_inject_imu(ctypes.c_float(ax), ctypes.c_float(az), ctypes.c_float(gy))

    # Every 4th tick → VelocitySetMs(dt_s)
    if (S["mc_count"] % 4) == 0:
        g_sim_velocity_ms.value = S["vel"]
        S["vel"] = lib.sim_velocity_set_ms(ctypes.c_float(PHYSICS_DT))
        g_sim_velocity_ms.value = S["vel"]

    dac = 0 if S["brake"] else int(lib.sim_cruise_dac())

    # Append and trim to window size
    buf["t"].append(S["elapsed"])
    buf["v"].append(S["vel"])
    buf["sp"].append(S["setpoint"])
    buf["dac"].append(dac)
    buf["accel"].append(lib.sim_get_accel())
    buf["error"].append(S["setpoint"] - lib.sim_get_est_velocity())
    buf["imuax"].append(ax)
    buf["imuaz"].append(az)

    for k in buf:
        if len(buf[k]) > WINDOW_TICKS:
            buf[k].pop(0)

    S["mc_count"] += 1
    S["elapsed"]  += TICK_DT
    S["tick"]     += 1

# =============================================================================
# PLOT
# =============================================================================
plt.rcParams.update({
    "figure.facecolor": "#000", "axes.facecolor": "#080808",
    "axes.edgecolor":   "#1a1a1a", "axes.labelcolor": "#444",
    "axes.titlecolor":  "#555", "axes.titlesize": 8,
    "xtick.color":      "#2a2a2a", "ytick.color": "#2a2a2a",
    "xtick.labelsize":  7, "ytick.labelsize": 7,
    "grid.color":       "#0f0f0f", "grid.linewidth": 0.6,
    "text.color":       "#bbb", "font.family": "monospace", "font.size": 8,
    "lines.linewidth":  1.8,
    "legend.facecolor": "#0a0a0a", "legend.edgecolor": "#1a1a1a",
    "legend.fontsize":  7,
})
CG = "#00ff6a"; CB = "#3b9eff"; CO = "#ff6a00"
CY = "#ffd000"; CP = "#a855f7"; CR = "#ff3b3b"

fig = plt.figure(figsize=(14, 8))
fig.patch.set_facecolor("#000")

outer = gridspec.GridSpec(1, 2, figure=fig,
                          width_ratios=[3, 1],
                          left=0.05, right=0.99,
                          top=0.92, bottom=0.06, wspace=0.06)
cgs   = gridspec.GridSpecFromSubplotSpec(3, 2, subplot_spec=outer[0],
                                         hspace=0.55, wspace=0.3)

ax_vel  = fig.add_subplot(cgs[0, :])
ax_dac  = fig.add_subplot(cgs[1, :])
ax_err  = fig.add_subplot(cgs[2, 0])
ax_imu  = fig.add_subplot(cgs[2, 1])

for ax in [ax_vel, ax_dac, ax_err, ax_imu]:
    ax.grid(True)
    ax.set_facecolor("#080808")
    for sp_ in ax.spines.values():
        sp_.set_edgecolor("#1a1a1a")

ax_vel.set_title("VELOCITY  vs  SETPOINT", color="#555", pad=4)
ax_dac.set_title("DAC OUTPUT  ( 0 – 1023 )  ·  drag setpoint to see spike", color="#555", pad=4)
ax_err.set_title("PI ERROR  ( m/s )", color="#555", pad=4)
ax_imu.set_title("IMU  accel_x / accel_z  ( m/s² )", color="#555", pad=4)

# Fixed y limits — never autoscale these two so blit is clean
ax_vel.set_ylim(CRUISE_MIN - 1, CRUISE_MAX + 1)
ax_dac.set_ylim(-30, 1060)
ax_dac.axhline(568, color="#1e1e1e", lw=1, ls="-", zorder=0)
ax_dac.text(0.01, 0.50, "568 = holding speed  (0 m/s² net accel)",
            transform=ax_dac.transAxes, color="#252525", fontsize=7)

# Lines — initialised with the pre-filled buffers so they show immediately
t_init = np.array(buf["t"])

ln_vel, = ax_vel.plot(t_init, buf["v"],     color=CG,    lw=2,  label="velocity (m/s)")
ln_sp,  = ax_vel.plot(t_init, buf["sp"],    color="#444",lw=1,  ls="--", label="setpoint")
ax_vel.legend(loc="lower right", framealpha=0.3)

ln_dac, = ax_dac.plot(t_init, buf["dac"],   color=CY, lw=2)
dac_lbl = ax_dac.text(0.01, 0.83, "DAC = 0",
                       transform=ax_dac.transAxes,
                       color=CY, fontsize=10, fontweight="bold")

ln_err, = ax_err.plot(t_init, buf["error"], color=CR, lw=1.5)
ax_err.axhline(0, color="#252525", lw=1, ls="--")

ln_ax,  = ax_imu.plot(t_init, buf["imuax"], color=CO, lw=1.2, label="accel_x")
ln_az,  = ax_imu.plot(t_init, buf["imuaz"], color=CB, lw=1.2, label="accel_z")
ax_imu.legend(loc="upper right", framealpha=0.3)

# Set initial x limits
for ax in [ax_vel, ax_dac, ax_err, ax_imu]:
    ax.set_xlim(-WINDOW_S, 0)

# Header
hdr = fig.text(0.01, 0.96,
               "UBC Solar  ·  cruise_control.c compiled C  ·  t=0.00s",
               color="#333", fontsize=8, family="monospace", va="top")

# ── Sliders ───────────────────────────────────────────────────────────────────
SL_L = 0.768; SL_W = 0.215; SL_H = 0.020; SL_G = 0.034

def mka(r):
    return fig.add_axes([SL_L, 0.93 - r*SL_G, SL_W, SL_H], facecolor="#0a0a0a")
def lbl(r, t, c="#3a3a3a"):
    fig.text(SL_L, 0.93 - r*SL_G + SL_H + 0.003, t, color=c, fontsize=7, family="monospace")
def sec(r, t):
    fig.text(SL_L, 0.93 - r*SL_G + SL_H + 0.005,
             f"── {t} ──", color="#222", fontsize=7, family="monospace")

R = 0
sec(R, "LIVE CONTROLS");              R += 1
lbl(R, "setpoint  m/s  ← drag me", CG)
sl_sp   = Slider(mka(R), "", CRUISE_MIN, CRUISE_MAX, valinit=11.11, color=CG);   R += 1
lbl(R, "hill angle  °", CP)
sl_hill = Slider(mka(R), "", -15, 15, valinit=0.0, color=CP);                    R += 1
sec(R, "IMU NOISE");                   R += 1
lbl(R, "accel σ  m/s²", CB)
sl_na   = Slider(mka(R), "", 0.0, 0.5, valinit=0.05, color=CB);                  R += 1
lbl(R, "gyro σ  mdps", CP)
sl_ng   = Slider(mka(R), "", 0.0, 20.0, valinit=2.0, color=CP);                  R += 1

for sl in [sl_sp, sl_hill, sl_na, sl_ng]:
    sl.ax.set_facecolor("#0a0a0a")
    sl.valtext.set_color("#888")
    sl.valtext.set_fontsize(7)
    sl.label.set_color("#1f1f1f")
    for sp_ in sl.ax.spines.values():
        sp_.set_edgecolor("#1a1a1a")

def make_btn(row, label, c):
    a = fig.add_axes([SL_L, 0.93 - row*SL_G, SL_W, SL_H*1.6], facecolor="#0a0a0a")
    b = Button(a, label, color="#0a0a0a", hovercolor="#111")
    b.label.set_color(c); b.label.set_fontsize(8); b.label.set_family("monospace")
    for sp_ in a.spines.values():
        sp_.set_edgecolor("#1a1a1a")
    return b

sec(R, "TASK CONTROLS");               R += 1
btn_play  = make_btn(R, "⏸  PAUSE",   CG); R += 1
btn_brake = make_btn(R, "●  BRAKE",   CR); R += 1
btn_reset = make_btn(R, "⏮  RESET",  "#444"); R += 1

# Stats
stats_ax = fig.add_axes([SL_L, 0.04, SL_W, 0.30], facecolor="#000")
stats_ax.set_xticks([]); stats_ax.set_yticks([])
for sp_ in stats_ax.spines.values():
    sp_.set_edgecolor("#151515")
stats_txt = stats_ax.text(0.05, 0.97, "starting…",
                            transform=stats_ax.transAxes,
                            color="#555", fontsize=7.5, family="monospace",
                            va="top", linespacing=2.0)

# ── Callbacks ─────────────────────────────────────────────────────────────────
def on_sp(v):
    S["setpoint"] = v
    lib.sim_set_cruise_setpoint(ctypes.c_float(v))  # live — no reset needed

def on_hill(v):  S["hill"] = v

def on_play(e):
    S["paused"] = not S["paused"]
    btn_play.label.set_text("▶  PLAY" if S["paused"] else "⏸  PAUSE")

def on_brake(e):
    S["brake"] = not S["brake"]
    btn_brake.label.set_text("○  RELEASE" if S["brake"] else "●  BRAKE")
    btn_brake.label.set_color("#555" if S["brake"] else CR)

def on_reset(e):
    reset_task(sl_sp.val)
    S["hill"] = sl_hill.val

sl_sp.on_changed(on_sp)
sl_hill.on_changed(on_hill)
btn_play.on_clicked(on_play)
btn_brake.on_clicked(on_brake)
btn_reset.on_clicked(on_reset)

# ── Animation ─────────────────────────────────────────────────────────────────
def animate(frame):
    if S["paused"]:
        return

    # Run N task ticks (each 25ms of sim time)
    for _ in range(TICKS_PER_FRAME):
        task_tick()

    t   = np.array(buf["t"])
    t0  = t[-1] - WINDOW_S
    t1  = t[-1]

    # Velocity vs setpoint
    ln_vel.set_data(t, np.array(buf["v"]))
    ln_sp.set_data(t,  np.array(buf["sp"]))
    ax_vel.set_xlim(t0, t1)

    # DAC
    dac_arr = np.array(buf["dac"])
    ln_dac.set_data(t, dac_arr)
    ax_dac.set_xlim(t0, t1)
    cur_dac = int(dac_arr[-1])
    if   cur_dac > 590: status = "accelerating ↑"
    elif cur_dac < 545: status = "decelerating ↓"
    else:               status = "holding ─"
    dac_lbl.set_text(f"DAC = {cur_dac:4d} / 1023    {status}")

    # PI error
    err_arr = np.array(buf["error"])
    ln_err.set_data(t, err_arr)
    ax_err.set_xlim(t0, t1)
    ax_err.set_ylim(min(-0.5, err_arr.min() - 0.1),
                     max(0.5,  err_arr.max() + 0.1))

    # IMU
    ln_ax.set_data(t, np.array(buf["imuax"]))
    ln_az.set_data(t, np.array(buf["imuaz"]))
    ax_imu.set_xlim(t0, t1)
    ax_imu.set_ylim(-2, 12)

    # Header — sim time + wall clock
    sim_t  = S["elapsed"]
    wall_t = time.time() - S["wall_start"]
    status_str = "PAUSED" if S["paused"] else ("BRAKE" if S["brake"] else "RUNNING")
    hdr.set_text(
        f"UBC Solar  ·  cruise_control.c  compiled C  ·  "
        f"sim = {sim_t:7.2f}s    wall = {wall_t:6.1f}s    "
        f"tick = {S['tick']:6d}    {status_str}"
    )

    # Stats panel
    cur_v   = float(buf["v"][-1])
    cur_sp  = S["setpoint"]
    cur_acc = float(buf["accel"][-1])
    if   cur_dac > 590: dac_state = "ACCEL ↑  (PI pushing hard)"
    elif cur_dac < 545: dac_state = "DECEL ↓"
    else:               dac_state = "HOLD  (balanced)"

    stats_txt.set_text(
        f"── real C output ──\n"
        f"\n"
        f"sim time:   {sim_t:.2f} s\n"
        f"wall time:  {wall_t:.1f} s\n"
        f"\n"
        f"setpoint:   {cur_sp:.2f} m/s\n"
        f"            {cur_sp*3.6:.1f} km/h\n"
        f"\n"
        f"velocity:   {cur_v:.3f} m/s\n"
        f"            {cur_v*3.6:.1f} km/h\n"
        f"\n"
        f"error:      {abs(cur_v - cur_sp):.4f} m/s\n"
        f"accel:      {cur_acc:+.4f} m/s²\n"
        f"DAC:        {cur_dac} / 1023\n"
        f"            {dac_state}\n"
        f"\n"
        f"hill:       {S['hill']:.1f}°\n"
        f"brake:      {'ON' if S['brake'] else 'off'}\n"
        f"tick #:     {S['tick']}"
    )

    fig.canvas.draw_idle()   # lighter than draw() — only redraws stale regions

# ── Start ─────────────────────────────────────────────────────────────────────
reset_task(sl_sp.val)

ani = animation.FuncAnimation(
    fig, animate,
    interval=ANIM_MS,
    blit=False,              # blit=True breaks on MacOSX/TkAgg with text objects
    cache_frame_data=False,
)

plt.show()