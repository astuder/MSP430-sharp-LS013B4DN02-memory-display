//***************************************************************************************
//  SHARP LS013B4DN02 Memory Display
//
//  Simple library for TI MSP430 LaunchPad to write characters to SHARP LS013B4DN02
//  Memory Display using hardware SPI.
//
//  This display is end-of-life. However it's successor LS013B4DN04 has the same pin-out
//  signals and should work too.
//
//  ACLK = n/a, MCLK = SMCLK = default DCO. Note that display specifies 1MHz max for SCLK
//
//                MSP430G2231
//             -----------------
//            |             P1.0|-->LED  (VCOM status display)
//			  |                 |
//            |             P1.4|-->DISP (display on/off)
//			  |                 |
//            |             P1.5|-->SCLK (SPI clock)
//			  |                 |
//            |             P1.6|-->SI   (SPI data)
//			  |                 |
//            |             P1.7|-->SCS  (SPI chip select)
//
//  Display VDD and VDDA connected to LaunchPad VCC
//  Display GND connected to LaunchPad GND
//
//  LS013B4DN02 is specified for 5V VDD/VDDA but seems to work well with 3V as provided by
//  the LaunchPad. LS013B4DN04 specifies VDD/VDDA at 3V.
//
//  Adrian Studer
//  November 2012
//***************************************************************************************

#include <msp430.h>				

#include "font.h"

#define _LED	0x01					// LED1 used to verify VCOM state
#define _DISP	0x10					// Turn display on/off
#define _SCLK	0x20					// SPI clock
#define _SDATA	0x40					// SPI data (sent to display)
#define _SCS	0x80					// SPI chip select

#define MLCD_WR 0x01					// MLCD write line command
#define MLCD_CM 0x04					// MLCD clear memory command
#define MLCD_SM 0x00					// MLCD static mode command
#define MLCD_VCOM 0x02					// MLCD VCOM bit

#define PIXELS_X 96						// display is 96x96
#define PIXELS_Y 96						// display is 96x96

#define DISP_INVERT 1					// INVERT text
#define DISP_WIDE 2						// double-width text
#define DISP_HIGH 4						// double-height text

unsigned char LineBuff[PIXELS_X/8];		// line buffer

volatile unsigned char VCOM;			// current state of VCOM (0x04 or 0x00)

volatile unsigned int timeMSec;			// clock milliseconds
volatile unsigned char timeSecond;		// clock seconds
volatile unsigned char timeMinute;		// clock minutes

char TextBuff[9];						// buffer to build line of text

void SPIWriteByte(unsigned char value);
void SPIWriteWord(unsigned int value);
void SPIWriteLine(unsigned char line);
void printSharp(const char* text, unsigned char line, unsigned char options);

extern void doubleWideAsm(unsigned char c, unsigned char* buff);

int main(void)
{
	// configure WDT
	WDTCTL = WDTPW | WDTHOLD;							// stop watch dog timer

	P1DIR |= _LED | _DISP | _SCLK | _SDATA | _SCS;		// set all pins used to output
	P1OUT &= ~(_LED | _DISP | _SCLK | _SDATA | _SCS);	// set all outputs low

	// setup USI for SPI
	USICTL0 = USIPE6 | USIPE5 | USILSB					// enable SDO and SCLK, LSB sent first
					 | USIMST | USIOE;					// master, enable data output
	USICTL1 = USICKPH;									// data captured on first edge
	USICKCTL = USIDIV_1 | USISSEL_2;					// /1 SMCLK (= default ~1MHz), clock low when inactive

	// setup timer A, to keep time and alternate VCOM at 1Hz
	timeMSec = 0;										// initialize variables used by "clock"
	timeSecond = 0;
	timeMinute = 0;

	VCOM = 0;											// initialize VCOM, this flag controls LCD polarity
														// and has to be toggled every second or so

	TA0CTL = TASSEL_2 + MC_1;							// SMCLK (default ~1MHz), up mode
	TACCR0 = 1000;										// trigger every millisecond
	TACCTL0 |= CCIE;									// timer 0 interrupt enabled

	P1OUT |= _DISP;										// turn  display on

	// initialize display
	P1OUT |= _SCS;										// SCS high, ready talking to display
	SPIWriteByte(MLCD_CM | VCOM);						// send clear display memory command
	SPIWriteByte(0);									// send command trailer
	P1OUT &= ~_SCS;										// SCS lo, finished talking to display

	// write some text to display to demonstrate options
	printSharp("HELLO,WORLD?",1,0);
	printSharp(" SHARP",16,DISP_WIDE);
	printSharp("   MEMORY   ",24,DISP_INVERT);
	printSharp("  DISPLAY!",32,DISP_HIGH);
	printSharp("123456789012",56,0);

	while(1)
	{
		// show VCOM state on LED1 for visual control
		if(VCOM == 0)
		{
			P1OUT &= ~_LED;
		}
		else
		{
			P1OUT |= _LED;
		}

		// write clock to display
		TextBuff[0] = ' ';
		TextBuff[1] = timeMinute / 10 + '0';
		TextBuff[2] = timeMinute % 10 + '0';
		TextBuff[3] = ':';
		TextBuff[4] = timeSecond / 10 + '0';
		TextBuff[5] = timeSecond % 10 + '0';
		TextBuff[6] = 0;
		printSharp(TextBuff,72,DISP_HIGH | DISP_WIDE);

		// put display into low-power static mode
		P1OUT |= _SCS;									// SCS high, ready talking to display
		SPIWriteByte(MLCD_SM | VCOM);					// send static mode command
		SPIWriteByte(0);								// send command trailer
		P1OUT &= ~_SCS;									// SCS lo, finished talking to display

		// sleep for a while
		_BIS_SR(LPM0_bits + GIE);						// enable interrupts and go to sleep
	};
}

// write a string to display, truncated after 12 characters
// input: text		0-terminated string
//        line		vertical position of text, 0-95
//        options	can be combined using OR
//					DISP_INVERT	inverted text
//					DISP_WIDE double-width text (except for SPACE)
//					DISP_HIGH double-height text
void printSharp(const char* text, unsigned char line, unsigned char options)
{
	// c = char
	// b = bitmap
	// i = text index
	// j = line buffer index
	// k = char line
	unsigned char c, b, i, j, k;

	// rendering happens line-by-line because this display can only be written by line
	k = 0;
	while(k < 8 && line < PIXELS_Y)						// loop for 8 character lines while within display
	{
		i = 0;
		j = 0;
		while(j < (PIXELS_X/8) && (c = text[i]) != 0)	// while we did not reach end of line or string
		{
			if(c < ' ' || c > 'Z')						// invalid characters are replace with SPACE
			{
				c = ' ';
			}

			c = c - 32;									// convert character to index in font table
			b = font8x8[(c*8)+k];						// retrieve byte defining one line of character

			if(!(options & DISP_INVERT))				// invert bits if DISP_INVERT is _NOT_ selected
			{											// pixels are LOW active
				b = ~b;
			}

			if((options & DISP_WIDE) && (c != 0))		// double width rendering if DISP_WIDE and character is not SPACE
			{
				doubleWideAsm(b, &LineBuff[j]);			// implemented in assembly for efficiency/space reasons
				j += 2;									// we've written two bytes to buffer
			}
			else										// else regular rendering
			{
				LineBuff[j] = b;						// store pixels in line buffer
				j++;									// we've written one byte to buffer
			}

			i++;										// next character
		}

		while(j < (PIXELS_X/8))							// pad line for empty characters
		{
			LineBuff[j] = 0xff;
			j++;
		}

		SPIWriteLine(line++);							// write line buffer

		if(options & DISP_HIGH && line < PIXELS_Y)	// repeat line if DISP_HIGH is selected
		{
			SPIWriteLine(line++);
		}

		k++;											// next pixel line
	}
}

// transfer line buffer to display using SPI
// input: line	position where line buffer is rendered
void SPIWriteLine(unsigned char line)
{
	P1OUT |= _SCS;										// SCS high, ready talking to display

	SPIWriteByte(MLCD_WR | VCOM);						// send command to write line(s)
	SPIWriteByte(line+1);								// send line address

	USICTL0 &= ~USILSB;									// switch SPI to MSB first for proper bitmap orientation

	unsigned char j = 0;
	while(j < (PIXELS_X/8))								// write pixels / 8 bytes
	{
		USISRH = LineBuff[j++];							// store low byte
		USISRL = LineBuff[j++];							// store high byte
		USICNT |= USISCLREL | USI16B | USICNT4;			// release SCLK when done, transfer 16 bits
		while(!(USICTL1 & USIIFG));  					// wait for transfer to complete
	}

	USICTL0 |= USILSB;									// switch SPI back to LSB first for commands

	SPIWriteWord(0);									// send 16 bit to latch buffers and end transfer
	P1OUT &= ~_SCS;										// SCS low, finished talking to display
}

// send one byte over SPI, does not handle SCS
// input: value		byte to be sent
void SPIWriteByte(unsigned char value)
{
	USISRL = value;										// store byte
	USICNT |= USISCLREL | USICNT3;						// release SCLK when done, transfer 8 bits
	while(!(USICTL1 & USIIFG));  						// wait for transfer to complete
}

// send one byte over SPI, does not handle SCS
// input: value		word to be sent
void SPIWriteWord(unsigned int value)
{
	USISRL = value & 0xff;								// store low byte
	USISRH = value >> 8;								// store high byte
	USICNT |= USISCLREL | USI16B | USICNT4;				// release SCLK when done, transfer 16 bits
	while(!(USICTL1 & USIIFG));							// wait for transfer to complete
}

// interrupt service routine to handle timer A
#pragma vector=TIMERA0_VECTOR
__interrupt void handleTimerA(void)
{
	timeMSec++;											// count milliseconds

	if(timeMSec == 1000)								// if we reached 1 second
	{
		timeMSec = 0;									// reset milliseconds
		timeSecond++;									// increase seconds
		if(timeSecond == 60)							// if we reached 1 minute
		{
			timeSecond = 0;								// reset seconds
			timeMinute++;								// increase minutes
			if(timeMinute == 60)						// if we reached 1 hour
			{
				timeMinute = 0;							// reset minutes
			}
		}

		VCOM ^= MLCD_VCOM;								// invert polarity bit every second

		_bic_SR_register_on_exit(LPM0_bits);			// wake up main loop every second
	}
}
