#!/usr/bin/env python

for v in range(0,720,5):
    n = v / 5

    vd = v % 180

    #Pin 1 is high for 70 degrees every 180 degrees, goes off 5 degrees before each TDC
    pin1 = int(vd >= 105 and vd < 175)

    #pin2 goes high before two of the pin1 pulses (2nd and 4th). One longer, one shorter than the pin1 pulses
    pin2 = int( (v>=270 and v<410) or (v>=630 and v<705) )

    #pin3 is a scope trigger signal, just goes high for a bit at start of each cycle
    pin3 = int(v < 90)

    #pin4 goes as fast as it can!
    pin4 = n % 2

    print "  {%d, %d, %d, %d}, //%3d" % (pin1, pin2, pin3, pin4, v)
