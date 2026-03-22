"""
UBC Solar — Real-Time Cruise Control Simulator
===============================================
Calls ACTUAL compiled cruise_control.c + accel_driver.c via ctypes.

Setup:
    gcc -shared -fPIC -O2 -I. -include cruise_sim_stubs.h \\
        cruise_control_test.c accel_driver_test.c cruise_sim_exports.c \\
        -o cruise_test.so -lm

    python3 cruise_sim.py
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
lib.sim_reset.argtypes            = [ctypes.c_float, ctypes.c_float];  lib.sim_reset.restype = None
lib.sim_set_cruise_setpoint.argtypes = [ctypes.c_float];               lib.sim_set_cruise_setpoint.restype = None
lib.sim_inject_imu.argtypes       = [ctypes.c_float]*3;                lib.sim_inject_imu.restype = None
lib.sim_velocity_set_ms.argtypes  = [ctypes.c_float];                  lib.sim_velocity_set_ms.restype = ctypes.c_float
lib.sim_set_params.argtypes       = [ctypes.c_float]*7;                lib.sim_set_params.restype = None
lib.sim_get_accel.restype         = ctypes.c_float
lib.sim_get_est_velocity.restype  = ctypes.c_float
lib.sim_cruise_dac.restype        = ctypes.c_uint16
g_sim_velocity_ms = ctypes.c_float.in_dll(lib, "g_sim_velocity_ms")

# ── Timing ────────────────────────────────────────────────────────────────────
FSM_MS          = 25
PHYSICS_DT      = 0.1
TICK_DT         = FSM_MS / 1000.0
WINDOW_S        = 20.0
WINDOW_TICKS    = int(WINDOW_S / TICK_DT)
TICKS_PER_FRAME = 2
ANIM_MS         = 50
CRUISE_MIN      = 6.94
CRUISE_MAX      = 22.22

# ── Buffers ───────────────────────────────────────────────────────────────────
def make_buffers():
    t_init = list(np.linspace(-WINDOW_S, 0, WINDOW_TICKS))
    return {
        "t":     t_init,
        "v":     [CRUISE_MIN] * WINDOW_TICKS,
        "sp":    [CRUISE_MIN] * WINDOW_TICKS,
        "dac":   [0]          * WINDOW_TICKS,
        "accel": [0.0]        * WINDOW_TICKS,
        "error": [0.0]        * WINDOW_TICKS,
        "imuax": [0.0]        * WINDOW_TICKS,
        "imuaz": [9.81]       * WINDOW_TICKS,
        "imugy": [0.0]        * WINDOW_TICKS,
        "regen": [0]          * WINDOW_TICKS,
    }

buf = make_buffers()

# ── Task state ────────────────────────────────────────────────────────────────
S = dict(tick=0, mc_count=0, vel=CRUISE_MIN, elapsed=0.0,
         brake=False, paused=False, setpoint=11.11, hill=0.0,
         wall_start=time.time())

def push_params():
    lib.sim_set_params(
        ctypes.c_float(sl_mass.val),
        ctypes.c_float(sl_power.val),
        ctypes.c_float(sl_eff.val),
        ctypes.c_float(sl_cd.val),
        ctypes.c_float(sl_rolling.val),
        ctypes.c_float(sl_frontal.val),
        ctypes.c_float(1.26714),
    )

def reset_task(sp):
    global buf
    buf = make_buffers()
    S.update(tick=0, mc_count=0, vel=CRUISE_MIN, elapsed=0.0,
             brake=False, setpoint=sp, wall_start=time.time())
    push_params()
    lib.sim_reset(ctypes.c_float(CRUISE_MIN), ctypes.c_float(sp))
    lib.sim_set_cruise_setpoint(ctypes.c_float(sp))
    g_sim_velocity_ms.value = CRUISE_MIN

def imu_sample():
    r  = np.deg2rad(S["hill"])
    ax = 9.81 * np.sin(r) + np.random.normal(0, 0.05)
    az = 9.81 * np.cos(r) + np.random.normal(0, 0.05)
    gy = np.random.normal(0, 2.0)
    return float(ax), float(az), float(gy)

def task_tick():
    push_params()
    ax, az, gy = imu_sample()
    lib.sim_inject_imu(ctypes.c_float(ax), ctypes.c_float(az), ctypes.c_float(gy))

    if (S["mc_count"] % 4) == 0:
        g_sim_velocity_ms.value = S["vel"]
        S["vel"] = lib.sim_velocity_set_ms(ctypes.c_float(PHYSICS_DT))
        g_sim_velocity_ms.value = S["vel"]

    # Mirror ComputeNextCommand cruise regen logic with deadzone
    ACCEL_DEADZONE = 0.1   # m/s² — ignore noise near zero
    raw_dac      = int(lib.sim_cruise_dac())
    cruise_accel = lib.sim_get_accel()
    if S["brake"]:
        dac   = 0
        regen = 0
    elif cruise_accel < -ACCEL_DEADZONE:
        # Clearly decelerating — regen only
        dac   = 0
        regen = raw_dac
    else:
        # Accelerating or within deadzone — throttle + regen mirrored
        dac   = raw_dac
        regen = raw_dac

    buf["t"].append(S["elapsed"])
    buf["v"].append(S["vel"])
    buf["sp"].append(S["setpoint"])
    buf["dac"].append(dac)
    buf["regen"].append(regen)
    buf["accel"].append(cruise_accel)
    buf["error"].append(S["setpoint"] - lib.sim_get_est_velocity())
    buf["imuax"].append(ax)
    buf["imuaz"].append(az)
    buf["imugy"].append(gy)

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

fig = plt.figure(figsize=(17, 9))
fig.patch.set_facecolor("#000")

outer = gridspec.GridSpec(1, 2, figure=fig, width_ratios=[2.3, 1],
                          left=0.03, right=0.99,
                          top=0.93, bottom=0.04, wspace=0.04)
cgs   = gridspec.GridSpecFromSubplotSpec(4, 2, subplot_spec=outer[0],
                                         hspace=0.60, wspace=0.28)

ax_vel  = fig.add_subplot(cgs[0, :])
ax_dac  = fig.add_subplot(cgs[1, :])
ax_err  = fig.add_subplot(cgs[2, 0])
ax_imu  = fig.add_subplot(cgs[2, 1])
ax_pitch= fig.add_subplot(cgs[3, 0])
ax_gyro = fig.add_subplot(cgs[3, 1])

for ax in [ax_vel, ax_dac, ax_err, ax_imu, ax_pitch, ax_gyro]:
    ax.grid(True); ax.set_facecolor("#080808")
    for sp_ in ax.spines.values(): sp_.set_edgecolor("#1a1a1a")

ax_vel.set_title("VELOCITY  vs  SETPOINT", color="#555", pad=3)
ax_dac.set_title(
    "DAC  ·  568 = holding speed  ·  spikes to ~1023 when accelerating  ·  drops when decelerating",
    color="#555", pad=3)
ax_err.set_title("PI ERROR  ( m/s )", color="#555", pad=3)
ax_imu.set_title("IMU  accel_x / accel_z  ( m/s² )", color="#555", pad=3)
ax_pitch.set_title("PITCH ESTIMATE  ( ° )", color="#555", pad=3)
ax_gyro.set_title("GYRO_Y  raw  ( mdps )", color="#555", pad=3)

ax_dac.set_ylim(-30, 1060)
ax_dac.axhline(568, color="#1e1e1e", lw=1.2, zorder=0)
# Clearer DAC explanation labels
ax_dac.text(0.01, 0.56, "568 — accel=0  motor balancing drag+rolling  (NORMAL at cruise)",
            transform=ax_dac.transAxes, color="#2a2a2a", fontsize=7)
ax_dac.text(0.01, 0.92, "1023 — max accel  (climbing to new setpoint)",
            transform=ax_dac.transAxes, color="#1a2a1a", fontsize=7)
ax_dac.text(0.01, 0.06, "0 — brake / no throttle",
            transform=ax_dac.transAxes, color="#2a1a1a", fontsize=7)

t_init = np.array(buf["t"])
ln_vel, = ax_vel.plot(t_init, buf["v"],     color=CG,    lw=2,  label="velocity (m/s)")
ln_sp,  = ax_vel.plot(t_init, buf["sp"],    color="#444",lw=1,  ls="--", label="setpoint")
ax_vel.legend(loc="lower right", framealpha=0.3)
ln_dac,  = ax_dac.plot(t_init, buf["dac"],  color=CY, lw=2,   label="accel DAC")
ln_regen,= ax_dac.plot(t_init, [0]*WINDOW_TICKS, color=CB, lw=1.5, ls="--", label="regen DAC")
ax_dac.legend(loc="upper right", framealpha=0.3)
dac_lbl  = ax_dac.text(0.01, 0.74, "DAC = 0",
                        transform=ax_dac.transAxes, color=CY, fontsize=11, fontweight="bold")
ln_err, = ax_err.plot(t_init, buf["error"], color=CR, lw=1.5)
ax_err.axhline(0, color="#252525", lw=1, ls="--")
ln_ax,  = ax_imu.plot(t_init, buf["imuax"], color=CO, lw=1.2, label="accel_x")
ln_az,  = ax_imu.plot(t_init, buf["imuaz"], color=CB, lw=1.2, label="accel_z")
ax_imu.legend(loc="upper right", framealpha=0.3)
ln_pitch,= ax_pitch.plot(t_init, [0.0]*WINDOW_TICKS, color=CP, lw=1.5)
ax_pitch.axhline(0, color="#252525", lw=1, ls="--")
ln_gyro, = ax_gyro.plot(t_init, buf["imugy"], color=CY, lw=1.0, alpha=0.8)
ax_gyro.axhline(0, color="#252525", lw=1, ls="--")

for ax in [ax_vel, ax_dac, ax_err, ax_imu, ax_pitch, ax_gyro]:
    ax.set_xlim(-WINDOW_S, 0)

hdr = fig.text(0.01, 0.97,
               "UBC Solar  ·  cruise_control.c compiled C  ·  t=0.00s",
               color="#333", fontsize=8, family="monospace", va="top")

# =============================================================================
# SLIDER PANEL
# =============================================================================
# Values shown INSIDE the slider bar (bottom-left of bar) — never clips
SL_L = 0.700; SL_W = 0.285; SL_H = 0.020; SL_G = 0.031

def mka(r):
    return fig.add_axes([SL_L, 0.93 - r*SL_G, SL_W, SL_H], facecolor="#0a0a0a")

def lbl(r, t, c="#3a3a3a"):
    fig.text(SL_L, 0.93 - r*SL_G + SL_H + 0.003, t,
             color=c, fontsize=7, family="monospace")

def sec(r, t):
    fig.text(SL_L, 0.93 - r*SL_G + SL_H + 0.004,
             f"── {t} ──", color="#222", fontsize=7, family="monospace")

# Slider value labels — positioned inside the bar at bottom-left
# We store them as a dict keyed by slider so animate() can update them
val_labels = {}

def make_slider(row, lo, hi, init, color, step=None):
    """Create a slider with value text drawn INSIDE the bar, not to the right."""
    ax_ = mka(row)
    kw  = dict(color=color)
    if step: kw["valstep"] = step
    sl = Slider(ax_, "", lo, hi, valinit=init, **kw)
    # Hide the default valtext that matplotlib puts to the right
    sl.valtext.set_visible(False)
    sl.label.set_visible(False)
    # Draw our own value label inside the axes at left edge
    dec = 5 if (hi - lo) < 0.01 else 3 if (hi - lo) < 1 else 2 if (hi - lo) < 10 else 0
    txt = ax_.text(0.01, 0.10, f"{init:.{dec}f}",
                   transform=ax_.transAxes,
                   color="#ffffff", fontsize=7, fontweight="bold", family="monospace", va="bottom", zorder=10)
    val_labels[id(sl)] = (txt, dec)
    # Update label on change
    def on_change(v):
        d = val_labels[id(sl)][1]
        val_labels[id(sl)][0].set_text(f"{v:.{d}f}")
    sl.on_changed(on_change)
    return sl

R = 0
sec(R, "SCENARIO");                                          R += 1
lbl(R, "setpoint  m/s  ← drag live", CG)
sl_sp      = make_slider(R, CRUISE_MIN, CRUISE_MAX, 11.11, CG);       R += 1
lbl(R, "hill angle  °", CP)
sl_hill    = make_slider(R, -15, 15, 0.0, CP);                         R += 1

sec(R, "VEHICLE  (live)");                                   R += 1
lbl(R, "mass  kg", CO)
sl_mass    = make_slider(R, 100, 600, 300, CO, step=10);               R += 1
lbl(R, "motor power  W", CO)
sl_power   = make_slider(R, 500, 5000, 2000, CO, step=100);            R += 1
lbl(R, "efficiency", CO)
sl_eff     = make_slider(R, 0.5, 1.0, 1.0, CO);                       R += 1

sec(R, "AERODYNAMICS  (live)");                              R += 1
lbl(R, "drag coeff  Cd", CB)
sl_cd      = make_slider(R, 0.05, 0.5, 0.116, CB);                    R += 1
lbl(R, "frontal area  m²", CB)
sl_frontal = make_slider(R, 0.5, 3.0, 1.18, CB);                      R += 1
lbl(R, "rolling resistance", CB)
sl_rolling = make_slider(R, 0.001, 0.05, 0.01, CB);                   R += 1

sec(R, "PI CONTROLLER  (RESET to apply)");                   R += 1
lbl(R, "Kp factor    KP = factor × mass", CY)
sl_kp      = make_slider(R, 0.05, 2.0, 0.5, CY);                      R += 1
lbl(R, "Ki factor    KI = (val × 1e-5) × mass", CY)
sl_ki      = make_slider(R, 0.5, 100.0, 9.0, CY);           # displayed ×1e-5, so 9.0 = 0.00009
R += 1

for sl in [sl_sp, sl_hill, sl_mass, sl_power, sl_eff,
           sl_cd, sl_frontal, sl_rolling, sl_kp, sl_ki]:
    sl.ax.set_facecolor("#0a0a0a")
    for sp_ in sl.ax.spines.values(): sp_.set_edgecolor("#1a1a1a")

def make_btn(row, label, c):
    a = fig.add_axes([SL_L, 0.93 - row*SL_G, SL_W, SL_H*1.6], facecolor="#0a0a0a")
    b = Button(a, label, color="#0a0a0a", hovercolor="#111")
    b.label.set_color(c); b.label.set_fontsize(8); b.label.set_family("monospace")
    for sp_ in a.spines.values(): sp_.set_edgecolor("#1a1a1a")
    return b

sec(R, "TASK CONTROLS");                                     R += 1
btn_play  = make_btn(R, "⏸  PAUSE",  CG);                   R += 1
btn_brake = make_btn(R, "●  BRAKE",  CR);                    R += 1
btn_reset = make_btn(R, "⏮  RESET", "#444");                 R += 1

# Stats box — tall enough for all content
# Calculate actual remaining space above stats box
stats_top = 0.93 - R * SL_G - 0.005
stats_h   = max(0.18, stats_top - 0.04)
stats_ax  = fig.add_axes([SL_L, 0.04, SL_W, stats_h], facecolor="#000")
stats_ax.set_xticks([]); stats_ax.set_yticks([])
for sp_ in stats_ax.spines.values(): sp_.set_edgecolor("#1a1a1a")
stats_txt  = stats_ax.text(0.03, 0.97, "starting…",
                             transform=stats_ax.transAxes,
                             color="#555", fontsize=7, family="monospace",
                             va="top", linespacing=1.7)
stats_txt2 = stats_ax.text(0.55, 0.97, "",
                             transform=stats_ax.transAxes,
                             color="#555", fontsize=7, family="monospace",
                             va="top", linespacing=1.7)

# ── Callbacks ─────────────────────────────────────────────────────────────────
def on_sp(v):
    S["setpoint"] = v
    lib.sim_set_cruise_setpoint(ctypes.c_float(v))

def on_hill(v): S["hill"] = v

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

sl_sp.on_changed(on_sp); sl_hill.on_changed(on_hill)
btn_play.on_clicked(on_play); btn_brake.on_clicked(on_brake); btn_reset.on_clicked(on_reset)

# ── Animation ─────────────────────────────────────────────────────────────────
def animate(frame):
    if S["paused"]: return

    for _ in range(TICKS_PER_FRAME):
        task_tick()

    t   = np.array(buf["t"])
    t0  = t[-1] - WINDOW_S
    t1  = t[-1]

    # Velocity
    v_arr  = np.array(buf["v"])
    sp_arr = np.array(buf["sp"])
    ln_vel.set_data(t, v_arr); ln_sp.set_data(t, sp_arr)
    ax_vel.set_xlim(t0, t1)
    lo = min(v_arr.min(), sp_arr.min()) - 0.5
    hi = max(v_arr.max(), sp_arr.max()) + 0.5
    ax_vel.set_ylim(max(0, lo), hi)

    # DAC
    dac_arr   = np.array(buf["dac"])
    regen_arr = np.array(buf["regen"])
    ln_dac.set_data(t,   dac_arr)
    ln_regen.set_data(t, regen_arr)
    ax_dac.set_xlim(t0, t1)
    cur_dac = int(dac_arr[-1])
    if   cur_dac > 590: dac_status = "ACCELERATING ↑  (climbing to setpoint)"
    elif cur_dac < 545: dac_status = "DECELERATING ↓  (setpoint dropped)"
    else:               dac_status = "HOLDING ─  (568 = drag+rolling balanced)"
    dac_lbl.set_text(f"DAC = {cur_dac:4d} / 1023    {dac_status}")

    # PI error
    err_arr = np.array(buf["error"])
    ln_err.set_data(t, err_arr)
    ax_err.set_xlim(t0, t1)
    ax_err.set_ylim(min(-0.5, err_arr.min() - 0.1), max(0.5, err_arr.max() + 0.1))

    # IMU
    imuax_arr = np.array(buf["imuax"])
    imuaz_arr = np.array(buf["imuaz"])
    imugy_arr = np.array(buf["imugy"])
    ln_ax.set_data(t, imuax_arr); ln_az.set_data(t, imuaz_arr)
    ax_imu.set_xlim(t0, t1); ax_imu.set_ylim(-2, 12)

    # Pitch estimate  (accel-only: atan2(ax, az))
    pitch_arr = np.rad2deg(np.arctan2(imuax_arr, imuaz_arr))
    ln_pitch.set_data(t, pitch_arr)
    ax_pitch.set_xlim(t0, t1)
    ax_pitch.set_ylim(min(-5, pitch_arr.min()-1), max(5, pitch_arr.max()+1))

    # Gyro
    ln_gyro.set_data(t, imugy_arr)
    ax_gyro.set_xlim(t0, t1)
    rng = max(5.0, abs(imugy_arr).max() + 2)
    ax_gyro.set_ylim(-rng, rng)

    # Header
    sim_t  = S["elapsed"]
    wall_t = time.time() - S["wall_start"]
    run_str = "PAUSED" if S["paused"] else ("BRAKE" if S["brake"] else "RUNNING")
    hdr.set_text(
        f"UBC Solar  ·  cruise_control.c compiled C  ·  "
        f"sim = {sim_t:7.2f}s    wall = {wall_t:6.1f}s    "
        f"tick = {S['tick']:6d}    {run_str}"
    )

    # Stats — compute forces
    cur_v   = float(v_arr[-1])
    cur_sp  = S["setpoint"]
    cur_acc = float(buf["accel"][-1])
    err_val = abs(cur_v - cur_sp)
    kp_val  = sl_kp.val * sl_mass.val
    ki_val  = (sl_ki.val * 1e-5) * sl_mass.val
    v       = max(cur_v, 0.01)
    f_drag  = 0.5 * 1.26714 * sl_cd.val * sl_frontal.val * v**2
    f_roll  = sl_rolling.val * sl_mass.val * 9.81
    f_nom   = (sl_eff.val * sl_power.val) / v
    f_hill  = sl_mass.val * 9.81 * np.sin(np.deg2rad(S["hill"]))
    f_net   = f_nom - f_drag - f_roll - f_hill
    cur_pitch = float(pitch_arr[-1])
    cur_gyro  = float(imugy_arr[-1])

    # Left column
    stats_txt.set_text(
        f"── real C output ──\n"
        f"sim:  {sim_t:.1f}s  wall: {wall_t:.1f}s\n"
        f"tick: {S['tick']}\n"
        f"\n"
        f"setpoint: {cur_sp:.2f} m/s\n"
        f"          {cur_sp*3.6:.1f} km/h\n"
        f"velocity: {cur_v:.3f} m/s\n"
        f"          {cur_v*3.6:.1f} km/h\n"
        f"error:    {err_val:.4f} m/s\n"
        f"accel:    {cur_acc:+.4f} m/s²\n"
        f"accel DAC:{cur_dac:5d} / 1023\n"
        f"regen DAC:{int(regen_arr[-1]):5d} / 1023\n"
        f"\n"
        f"── forces (N) ──\n"
        f"F_nom:  {f_nom:7.2f}\n"
        f"F_drag: {f_drag:7.2f}\n"
        f"F_roll: {f_roll:7.2f}\n"
        f"F_hill: {f_hill:7.2f}\n"
        f"F_net:  {f_net:7.2f}\n"
        f"\n"
        f"── IMU ──\n"
        f"pitch:  {cur_pitch:+.3f} °\n"
        f"gyro_y: {cur_gyro:+.2f} mdps"
    )
    # Right column
    stats_txt2.set_text(
        f"── gains ──\n"
        f"KP: {kp_val:.1f}\n"
        f"KI: {ki_val:.5f}\n"
        f"    = {sl_ki.val:.1f}e-5 × mass\n"
        f"\n"
        f"── vehicle ──\n"
        f"mass:  {sl_mass.val:.0f} kg\n"
        f"power: {sl_power.val:.0f} W\n"
        f"eff:   {sl_eff.val:.2f}\n"
        f"\n"
        f"── aero ──\n"
        f"Cd:   {sl_cd.val:.3f}\n"
        f"area: {sl_frontal.val:.2f} m²\n"
        f"roll: {sl_rolling.val:.4f}\n"
        f"\n"
        f"hill:  {S['hill']:.1f}°\n"
        f"brake: {'ON' if S['brake'] else 'off'}"
    )

    fig.canvas.draw_idle()

# ── Start ─────────────────────────────────────────────────────────────────────
reset_task(sl_sp.val)
ani = animation.FuncAnimation(fig, animate, interval=ANIM_MS, blit=False, cache_frame_data=False)
plt.show()