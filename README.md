# DC motor controller with Atmega2560

The following firmware was developed for the Atmega2560 µC and permits the control of a DC motor through two different pushbuttons (used to increment or decrement the rpm of the electro-mechanic device).
The methodology of management is by adopting a PWM wave with resolution 2^10bits; however the scaling was done in order to have a mapping
from % to bits.
A mosfet was placed as intermediate device between microcontroller and motor in order to deal with the current limitations of the output ports of the Atmega.

**Extra**
The implementation was successfully tested with SimulIde.

![alt text](PWM motor controller.png)

