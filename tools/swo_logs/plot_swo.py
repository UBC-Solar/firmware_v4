import matplotlib.pyplot as plt
import sys
import os

FILE  = "swo_shunt_r=99500_nohms.log"
TITLE = "Shunt Current - R=99.500 mΩ"

STEP_SIZE    = 100   # mA — input current increment size
AVG_WINDOW   = 50    # samples to average before snapping to nearest step
PLOT_ERROR   = True  # show error chart (measured current - input current)
PLOT_VOLTAGE = True  # show shunt voltage chart with input current on second axis

AVG_RANGE_MS = None, None  # (start_ms, end_ms) — print averages over this window, or None for disabled

testing="""
    Averages from 49000 ms to 51000 ms (201 samples):
    Current: 2467.65 mA
    Voltage: 248038.68 nV

    Resistance = 0.10051 mΩ // maybe 1 too many sig figs here
    """

if len(sys.argv) >= 2:
    FILE = sys.argv[1]
if len(sys.argv) >= 3:
    TITLE = " ".join(sys.argv[2:])

path = os.path.join(os.path.dirname(os.path.abspath(__file__)), FILE)

times    = []
currents = []
voltages = []

with open(path, 'r') as f:
    for line in f:
        parts = line.strip().split(',')
        if len(parts) == 3:
            try:
                times.append(int(parts[0].strip()))
                currents.append(int(parts[1].strip()))
                voltages.append(int(parts[2].strip()))
            except ValueError:
                pass

def moving_avg(data):
    return [
        sum(data[max(0, i - AVG_WINDOW // 2):i + AVG_WINDOW // 2]) /
        len(data[max(0, i - AVG_WINDOW // 2):i + AVG_WINDOW // 2])
        for i in range(len(data))
    ]

if AVG_RANGE_MS != (None, None):
    t0, t1 = AVG_RANGE_MS
    window_i = [currents[i] for i in range(len(times)) if t0 <= times[i] <= t1]
    window_v = [voltages[i] for i in range(len(times)) if t0 <= times[i] <= t1]
    if window_i:
        print(f"Averages from {t0} ms to {t1} ms ({len(window_i)} samples):")
        print(f"  Current: {sum(window_i)/len(window_i):.2f} mA")
        print(f"  Voltage: {sum(window_v)/len(window_v):.2f} nV")
    else:
        print(f"No samples found between {t0} ms and {t1} ms")

averaged      = moving_avg(currents)
input_current = [round(v / STEP_SIZE) * STEP_SIZE for v in averaged]
error         = [currents[i] - input_current[i] for i in range(len(currents))]

# Chart 1: current
fig1, ax = plt.subplots(figsize=(12, 5))
ax.plot(times, currents, linewidth=0.8, alpha=0.4, label="Raw")
ax.plot(times, averaged, linewidth=1.5, label=f"{AVG_WINDOW}-sample avg")
ax.plot(times, input_current, linewidth=1.5, linestyle='--', label="Input current (detected)")
if AVG_RANGE_MS != (None, None):
    ax.axvspan(AVG_RANGE_MS[0], AVG_RANGE_MS[1], alpha=0.15, color='green', label="Avg range")
ax.legend()
ax.set_xlabel("Time (ms)")
ax.set_ylabel("Current (mA)")
ax.set_title(TITLE)
ax.grid(True, alpha=0.3)
ax.axhline(0, color='black', linewidth=0.5)
fig1.tight_layout()
img_path = os.path.splitext(path)[0] + '.png'
fig1.savefig(img_path, dpi=150)
print(f"Saved to {img_path}")

# Chart 2: error
if PLOT_ERROR:
    error_avg = moving_avg(error)
    fig2, ax2 = plt.subplots(figsize=(12, 5))
    ax2.plot(times, error, linewidth=0.8, color='red', alpha=0.3, label="Error (raw - input)")
    ax2.plot(times, error_avg, linewidth=1.5, color='red', label=f"Error ({AVG_WINDOW}-sample avg)")
    ax2.axhline(0, color='black', linewidth=0.5)
    ax2.set_xlabel("Time (ms)")
    ax2.set_ylabel("Error (mA)")
    ax2.set_ylim(-100, 100)
    ax2.set_title("Measurement Error")
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    fig2.tight_layout()
    err_path = os.path.splitext(path)[0] + '_error.png'
    fig2.savefig(err_path, dpi=150)
    print(f"Saved to {err_path}")

# Chart 3: shunt voltage with input current on secondary axis
if PLOT_VOLTAGE:
    fig3, ax3 = plt.subplots(figsize=(12, 5))
    ax3.plot(times, voltages, linewidth=0.8, alpha=0.5, color='steelblue', label="Shunt Voltage (nV)")
    ax3.set_xlabel("Time (ms)")
    ax3.set_ylabel("Shunt Voltage (nV)", color='steelblue')
    ax3.tick_params(axis='y', labelcolor='steelblue')
    ax3.grid(True, alpha=0.3)
    ax3.axhline(0, color='black', linewidth=0.5)

    ax3b = ax3.twinx()
    ax3b.plot(times, input_current, linewidth=1.5, linestyle='--', color='darkorange', label="Input current (detected)")
    ax3b.set_ylabel("Input Current (mA)", color='darkorange')
    ax3b.tick_params(axis='y', labelcolor='darkorange')

    if AVG_RANGE_MS != (None, None):
        ax3.axvspan(AVG_RANGE_MS[0], AVG_RANGE_MS[1], alpha=0.15, color='green', label="Avg range")
    lines1, labels1 = ax3.get_legend_handles_labels()
    lines2, labels2 = ax3b.get_legend_handles_labels()
    ax3.legend(lines1 + lines2, labels1 + labels2)
    ax3.set_title(f"{TITLE} - Shunt Voltage")
    fig3.tight_layout()
    volt_path = os.path.splitext(path)[0] + '_voltage.png'
    fig3.savefig(volt_path, dpi=150)
    print(f"Saved to {volt_path}")

plt.show()
