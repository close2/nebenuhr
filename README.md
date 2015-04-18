# nebenuhr
atmega code to control a secondary clock which would normally get its signal from a master clock.


as of 2015-04-18

untested and probably incomplete code.


## Idea

I reuse the board of a broken radio controlled clock.

An atmega is connected to the 4 relevant pins of this board (see images in the doc directory).

The atmega will then power down until a low on INT1 triggers an IRQ and wakes the atmega.
We will then turn on an h-bridge for a short period and power down again.

Because the clock is pretty loud, setting a pin high disables the h-bridge output.
The atmega keeps count of the missed minute-arm movements and outputs them when the pin is set
low again.


As we have 3 power sources now:
* 1 AA battery for the old rc clock
* 1 battery (4.5V?) for the atmega
* 3 4.5V in series for the H-Bridge

i wanted to make it easy for the owner of the clock to find out, why the clock stopped working.
Pushing a button (INT0) will measure those voltages (or rather the divided voltage) and turn on
corresponding LEDs if the voltage seems high enough.


## libcl

This project uses my still incomplete libcl library.
Hopefully I will complete this libary soon and remove the header files from this project.
(And reference my libcl repository instead).


# PLEASE NOTE

I haven't even tested this code on an atmega.  I know it compiles, but that's about it.
If this code does something unexpected it's YOUR problem.
Don't complain.  (actually please open an issue).


