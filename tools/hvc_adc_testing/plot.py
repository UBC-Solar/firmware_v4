"""
Plot time series of the first 6 ADC buffer values per line from UART output.

Data format per line:
  First 16 ADC Buffer Values: A, B, C, D, E, F, A, B, C, D, E, F, A, B, C, D

Channels (5 ADC channels, F is start of next sample group):
  0 (A) - dcdc_thermistor
  1 (B) - motor_precharge
  2 (C) - mppt_precharge
  3 (D) - supp_sense
  4 (E) - lv_curr_sense

Usage:
  python plot.py                  # reads plot.txt
  python plot.py my_data.txt      # reads my_data.txt
"""

import sys
import re
import matplotlib.pyplot as plt

CHANNEL_LABELS = [
    "A - dcdc_thermistor",
    "B - motor_precharge",
    "C - mppt_precharge",
    "D - supp_sense",
    "E - lv_curr_sense",
]

PREFIX = "Time:"

def parse_file(path):
    channels = [[] for _ in range(5)]
    times = []
    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line.startswith(PREFIX):
                continue
            numbers = list(map(int, re.findall(r"\d+", line)))
            # numbers[0] = time, numbers[1..5] = first 5 channel values
            if len(numbers) < 6:
                continue
            times.append(numbers[0])
            for i in range(5):
                channels[i].append(numbers[i + 2])
    return times, channels

def plot(times, channels):
    fig, axes = plt.subplots(5, 1, figsize=(12, 9), sharex=True)
    fig.suptitle("HVC ADC Buffer — First 5 Channels vs Time", fontsize=13)

    for i, (ax, data) in enumerate(zip(axes, channels)):
        ax.plot(times, data, linewidth=0.8)
        ax.set_ylabel(CHANNEL_LABELS[i], fontsize=8)
        ax.grid(True, alpha=0.3)

    axes[-1].set_xlabel("Time (ms)")
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "tools/hvc_adc_testing/plot.txt"
    times, channels = parse_file(path)
    if not channels[0]:
        print(f"No matching data found in '{path}'")
        sys.exit(1)
    print(f"Parsed {len(channels[0])} samples from '{path}'")
    plot(times, channels)
