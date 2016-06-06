# Denso ECU adapter PCB

## License
All PCB layouts in this directory are licensed under [Creative Commons Attribution-Sharealike 4.0 (CC BY-SA 4.0)](http://creativecommons.org/licenses/by-sa/4.0/)
Copyright 2016 John Stäck and Vulkteamet

These are various PCB layouts for connecting a custom ECU (such as MegaSquirt or Speeduino) to the original car wiring harness, using a Nippon Denso connector.
These are found in various cars, but mostly Japanese cars from the 1990's. This includes at least some model years of Mazda MX-5/323 (Miata/Protege), Honda Civic/Prelude/Accord, Toyota Corolla, Ford Escort, Mitsubishi Lancer

## Simple adapter
The two "simple" layouts are just to mount the ECU connector header, and provide an easy way to solder cables to it. Comes in 48 and 64 pin versions.

## Breakout adapter
The "breakout" layout is mostly intended for testing and development purposes. It has the same ECU connector, but also provides solder points for
*Test hooks / pins
*Pigtail cable for ECU plugs (to connect in parallel with stock ECU)
*"Crossbar" to connect things together
*"Common rails" (for things like signal/chassis ground and +12V/+5V)
*Screw terminals (5.08mm pin spacing)

## Dimensions
Some applications will mess with the size of the image as it is displayed / printed. Make sure the final printout has these dimensions:
*ECU_48_simple: 92.5 x 34.6mm
*ECU_64_simple: 120.2 x 34.6mm
*ECU_64_breakout: 140.2 x 81.3mm
