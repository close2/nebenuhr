# nebenuhr
atmega code to control a secondary clock which would normally get its signal from a master clock.


## Idea

I reuse the board of a broken radio controlled clock.

See images in the doc directory.

The pins A and B of the clock powered a coil for a short amount of time (30ms) every minute.
As you can see in the diagram the polarity is reversed every minute.  (This is exactly the same
for the Nebenuhr.)  The signal on pins A and B are called timeSignal in my code.

When both arms (minute and hour) were at 12 o'clock Pin D was connected to ground (Pin C).

In my code I call this the noonSensor.  This was originally done with a light-sensor and 2 holes
in the hour/minute arms.


An Atmega is connected to the 3 (A, B and D) relevant pins of this board.


## Atmega

In order to use as little power as possible we power down the Atmega between timerSignals.

Only a low can wake an atmega when powered off,

                           VCC
                            |
                            1MΩ
                            |
                            ----------- Atmega TimeSignal Pin (INT1)
                            /
     Pin A -- 1MΩ -- Base |<
                            ↘
                             |
                            GND
                          
    
And again the same for B.  Note that both A and B finally connect to the same Atmega pin.
(One resistor to VCC would be enough).

When A goes high (1.5V) the transistor pulls the Atmega pin to ground, waking the Atmega.


I considered connecting Pin D directly to the atmega and switching between HiZ and low,
but decided to play it safe.  A Mosfet connects Pin D to ground, when an Atmega pin
goes high:


    Pin D --
           |
           | Drain
           |
           Mosfet  Gate ---- 1kΩ ----- Atmega NoonSignal Pin
           |
           | Source
           |
          GND



An HBridge will power the coils of the nebenuhr.


                                 VCC
                                  |
                                  1MΩ
                                  |
       to P Channel left side  ----
                                   \
                                    >| 1kΩ -- Atmega HBridge1 Pin
                                   ↙        |
                                  |         |
                                 500kΩ     1kΩ
                                  |         |
                                 GND        to N Channel right side
                          


Similar for P Channel right side, N Channel left side and HBridge2 Pin.


## Pause pin

Because the clock is pretty loud, setting a pin high disables the h-bridge output.
The atmega keeps count of the missed minute-arm movements and outputs them when the pin is set
low again.

The pin is simply either connected to VCC or GND:

              VCC
               |
                \
    
                     -- 1MΩ -- Atmega Pause Pin
    
                /
               |
              GND


## Battery check

As we have 3 power sources now:
* 1 AA battery for the old rc clock
* 1 battery (4.5V?) for the atmega
* 3 4.5V in series for the H-Bridge

i wanted to make it easy for the owner of the clock to find out, why the clock stopped working.
Pushing a button (INT0) will compare those voltages (or rather the divided voltages) to 1.1V and
turn on corresponding LEDs if the voltage seems high enough.

This was by far the most work.  *Don't do it.*

Either really remove the code, or connect 1MΩ to VCC on the check battery pin.

The difficulties:

- the voltage divider for the voltages should only be powered when the button is pressed.  
  I added Mosfets to only connect the voltage dividers to ground during measurements.
- the HBridge/ Nebenuhr voltage is to high for the atmega → simply not connecting to GND
  would result in 15+V to one of the atmega pins.  I finally simply added an solid state
  relay.
- Pressing the check battery button should wake the atmega and the button therefore connects
  the atmega pin to GND when pressed.  
  This means that I can not use the button to power the voltage divider. To measure the
  atmega voltage I used another pin to power the atmega voltage divider.


## libcl

This project uses my still incomplete libcl library.
Hopefully I will complete this libary soon and remove the header files from this project.
(And reference my libcl repository instead).


# Usage

- Power the atmega 4.5V
- Power the Nebenuhr ~15V
- move the Nebenuhr arms to 2 minutes before 12:00
- Power the rc-clock.

The rc-clock will now send 2 timer signals until the atmega sends the noon-signal.

Then the rc-clock will wait for an rc-signal.  In my case this could take several hours.


# Feel free to contact me.

I have the impression that writing this documentation costs me far more time than simply
answering questions... :(

