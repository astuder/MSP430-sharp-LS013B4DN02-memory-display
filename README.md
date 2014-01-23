Example of using SHARP LS013B4DN02 with MSP430 LaunchPad
========================================================

SHARP LS013B4DN02 is a very low power 1.35 inch, 96x96 pixel LCD display. It uses
less than 15 uW when displaying a static image.

http://www.sharpmemorylcd.com/1-35-inch-memory-lcd.html

Mouser indicates this model as end-of-life. However LS013B4DN04 has very similar
specifications and should also work with this code and breakout board.

As the display comes with 0.5mm pitch FPC cable I designed a basic breakout board
with a matching connector and the recommended decoupling capacitors.

Wiring on MSP430 LaunchPad to LCD breakout:
* P1.0:	LED  (VCOM status display)
* P1.4:	DISP (display on/off)
* P1.5:	SCLK (SPI clock)
* P1.6:	SI   (SPI data)
* P1.7:	SCS  (SPI chip select)
* GND:	GND
* VCC:	VDD and VDDA	(even though specified 5V, the display works fine at 3V)

This code should also work with the Adafruit SHARP Memory Display Breakout
http://www.adafruit.com/products/1393

