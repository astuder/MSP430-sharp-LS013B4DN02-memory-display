Text Library for SHARP Memory LCD
=================================

Created by Adrian Studer, April 2014.
Distributed under MIT License, see license.txt for details.

Basic, text-only library for SHARP Memory LCD. Developed with Energia and tested on MSP430G2553 and MSP430F5529 LaunchPads, 
with 43oh and TI SHARP Memory LCD BoosterPacks.

While untested, this library should work with other LaunchPads as well as Arduino. It should also work with
SHARP Memory LCD breakout boards sold by Adafruit, Kuzyatech and others.

Methods
-------

*SHARPMemLCDTxt(CS,DISP,VCOM)* Constructor of display class
- CS: SPI chip select pin, default 13, set to 6 for TI BoosterPack
- DISP: Pin to turn on/off display, default 8, set to 5 for TI BoosterPack
- VCOM: Pin for hardware LCD polarity inversion, default 0, set to 5 for 43oh and 19 for TI BoosterPack (see VCOM section below)

*begin()* Initialize display, including SPI library. Should be called within setup() section of your sketch.

*on()* Turns display on

*off()* Turns display off

*clear()* Clears display contents

*print(text,line,options)* Prints line of text
- text: Text string to be displayed, only ASCII 32 through 90 (0-9, uppercase A-Z, some punctuation)
- line: Vertical position of text
- options: Formatting options, combinable by adding them together. DISP_INVERT, DISP_HIGH, DISP_WIDE 

*pulse(force)* Manually toggle VCOM (see VCOM section below)
- force: If set to 0 (default) will only toggle VCOM if last call was at least 500ms ago. If set to 1, VCOM will toggled with each call.

Display size
------------

The library as provided is setup a screen size of 96x96 pixels. This is the dimension of the LCDs on the BoosterPacks and compatible with any 1.35" diagonal SHARP Memory LCD (LS013B4DN01, 02 and 04). You can adjust this library to different display sizes by editing PIXELS_X and PIXELS_Y in SHARPMemLCDTxt.h.

SPI
---

This library uses SPI to communicate with the display. If you want to use a different SPI module than the default, you will need to edit the SPI configuration inside the begin() method.

Also note that this library will reconfigure SPI bit order to LSB each time it is called. You may need to adjust other libraries that rely on SPI to be compatible with this one.


VCOM and LCD polarity inversion
-------------------------------

SHARP Memory LCDs require an alternating signal VCOM to avoid DC bias buildup. DC bias over time may cause stuck pixels.
The frequency of this singal varies by LCD model, but is typically specified as 0.5-30 Hz.

The *print* method of this library automatically alternates polarity approximately twice per second.
If your program updates the display only less often, it is recommended to call *pulse* yourself at least once a second.

SHARP Memory LCDs have two ways to provide the VCOM signal. One is by software with a special command, the other in hardware by
wiggling the EXTCOMM pin. The method to use is determined by the state of the EXTMODE pin. By default EXTMODE is set to software 
on both BoosterPacks, modifyable with solder bridges.

This library by default uses the software method to toggle VCOM. If your BoosterPack is configured for receiving VCOM by hardware,
the pin connected to EXTCOMM is set as 3rd parameter of the SHARPMemLCDTxt constructor. 

It would be convenient to setup a timer interrupt to toggle VCOM by calling *pulse*. This will work fine in hardware mode. However,
in software mode this can cause problems if the SPI library is using interrupts (not an issue with Energia and MSP430 Launchpads).
