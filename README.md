# **DRD PCBWay Showcase**

The Driver Dashboard (DRD) is a PCB designed to act as the central hub of the interface between the driver and the solar racing car featuring LEDs, switches, and an LCD to display critical information of the car. This repository will go into details about the hardware design of this PCB from schematic to layout design.

![alt text](image.png)
This project was sponsored by PCBWay, who provided PCB manufacturing support and quick design review for the DRD. During the review process, they clarified aspects of the layout and identified a design fault, which was confirmed and resolved prior to the board being produced.
![alt text](image.png)
![alt text](image-6.png)
![alt text](image-7.png)

The DRD is controlled by an STM32 microcontroller responsible for managing low-voltage vehicle functions, including exterior lighting control and drive-state selection. The board interfaces with the vehicle CAN bus to receive real-time telemetry such as fault conditions, motor controller status signals, and the supplemental battery voltage, which are processed and displayed to the driver.

# Layout 

The DRD layout was driven by mechanical constraints and dashboard integration requirements, defining the placement of connectors, switches, and the display. Functional partitioning was used to group subsystems such as the Microcontroller Unit (MCU), CAN transceiver, and N-channel MOSFET LED drivers, to maximize the space usage of the board.

# Routing

With the various subsystems involving 30+ GPIOs, analog signals, and most importantly communication lines, various routing practices were used during this design process.

![alt text](image-2.png)

In order to prevent the GND plane being cut off by traces, 0 ohm resistors were used for some digital signals to bypass routes to create a continuous place for short return paths.

![alt text](image-3.png)

Curved traces were used in place of 90-degree corners to avoid sudden changes in the geometry. An abrupt change will create a small area where the impedance will be decreased due to a larger area being present in the bend that causes slight impedance discontinuites in addition to minor reflections in fast signals.

![alt text](image-4.png)

Stiching vias were put in place as some traces switched between the planes due to hte large number of signals coming from the MCU. Stiching vias act as a stable reference for signals when they cross planes by having a continuous GND trace going from the top to bottom layer of the board that prevents magnetic coupling from the outside going to the inside.

![alt text](image-5.png)

CAN communication uses differential pair signaling, where a signal and its inverse are transmitted on a matched pair of traces such that external interference couples similarly into both lines. The receiver measures the voltage difference between the two signals, improving noise immunity by rejecting common-mode interference in the electrically noisy vehicle environment.

## Future Improvements

While the DRD integrates a large number of signals, future revisions would further refine component placement and routing to minimize signals crossing plane splits. Board area could also be optimized by tightening component spacing and reorganizing functional blocks, allowing the overall board length to be reduced.
