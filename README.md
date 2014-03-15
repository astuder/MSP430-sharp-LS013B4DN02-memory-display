Example of using SHARP Memory LCD LS013B4DN04 with MSP430G2553
==============================================================

This is a simple library for TI MSP430G2553 to write characters to SHARP LS013B4DN04
Memory Display using USCI for hardware SPI.

SHARP LS013B4DN04 is a very low power 1.35 inch, 96x96 pixel LCD display. It uses
less than 15 uW when displaying a static image.

http://www.sharpmemorylcd.com/1-35-inch-memory-lcd.html

This code also works with the predecessor LS013B4DN02 and should work with any
display of that series with compatible pinouts.

The project also includes a matching BoosterPack to mount the display on a MSP430 LaunchPad.
More information about the BoosterPack can be found on 43oh:
http://forum.43oh.com/topic/4979-sharp-memory-display-booster-pack/

Pinout MSP430 -> Display
* P1.0:	LED  (VCOM status display)
* P1.5:	SCLK (SPI clock)
* P1.7:	SI   (SPI data)
* P2.0:	DISP (display on/off)
* P2.5:	SCS  (SPI chip select)
* GND:	GND
* VCC:	VDD and VDDA

This code should also work with the Adafruit SHARP Memory Display Breakout
http://www.adafruit.com/products/1393

