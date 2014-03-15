//***************************************************************************************
//  SHARP LS013B4DN04 Memory Display
//
//  Simple library for TI MSP430G2 LaunchPad to write characters to SHARP LS013B4DN04
//  Memory Display using hardware SPI.
//
//  This code also works with the predecessor LS013B4DN02 and should work with any
//  display of that series with compatible pinouts.
//
//  ACLK = n/a, MCLK = SMCLK = default DCO. Note that display specifies 1MHz max for SCLK
//
//                MSP430G2553
//             -----------------
//            |             P1.0|-->LED  (VCOM status display)
//			  |                 |
//            |             P1.5|-->SCLK (SPI clock)
//			  |                 |
//            |             P1.7|-->SI   (SPI data)
//			  |                 |
//            |             P2.0|-->DISP (display on/off)
//			  |                 |
//            |             P2.5|-->SCS  (SPI chip select)
//
//  Display VDD and VDDA connected to LaunchPad VCC
//  Display GND connected to LaunchPad GND
//
//  LS013B4DN02 is specified for 5V VDD/VDDA but seems to work well with 3V as provided by
//  the LaunchPad. LS013B4DN04 specifies VDD/VDDA at 3V.
//
//  This code works with the 43oh SHARP Memory LCD BoosterPack
//  http://forum.43oh.com/topic/4979-sharp-memory-display-booster-pack/
//
//  Adrian Studer
//  March 2014
//***************************************************************************************

#include <msp430.h>				

#include "font.h"

#define _LED	BIT0					// LED1 used to verify VCOM state
#define _SCLK	BIT5					// SPI clock
#define _SDATA	BIT7					// SPI data (sent to display)
#define _SCS	BIT5					// SPI chip select
#define _DISP	BIT0					// Turn display on/off

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

	P1DIR |= _LED | _SCLK | _SDATA;						// set all pins used to output
	P1OUT &= ~(_LED | _SCLK | _SDATA);					// set all outputs low

	P1SEL |= _SCLK | _SDATA;							// connect pins to USCI (secondary peripheral)
	P1SEL2 |= _SCLK | _SDATA;							// connect pins to USCI (secondary peripheral)

	P2DIR |= _DISP | _SCS;								// set all pins used to output
	P2OUT &= ~(_DISP | _SCS);							// set all outputs low

	// configure UCSI B0 for SPI
	UCB0CTL1 |= UCSWRST;								// reset USCI B0
	UCB0CTL0 = UCCKPH | UCMST | UCMODE_0 | UCSYNC;		// read on rising edge, inactive clk low, lsb, 8 bit, master mode, 3 pin SPI, synchronous
	UCB0BR0 = 8; UCB0BR1 = 0;							// clock scaler = 8, i.e 2 MHz SPI clock
	UCB0CTL1 = UCSSEL_2;								// clock source SMCLK, clear UCSWRST to enable USCI B0
	UCB0CTL1 &= ~UCSWRST;								// enable USCI B0

	// setup timer A, to keep time and alternate VCOM at 1Hz
	timeMSec = 0;										// initialize variables used by "clock"
	timeSecond = 0;
	timeMinute = 0;

	VCOM = 0;											// initialize VCOM, this flag controls LCD polarity
														// and has to be toggled every second or so

	TA0CTL = TASSEL_2 + MC_1;							// SMCLK (default ~1MHz), up mode
	TACCR0 = 1000;										// trigger every millisecond
	TACCTL0 |= CCIE;									// timer 0 interrupt enabled

	P2OUT |= _DISP;										// turn  display on

	// initialize display
	P2OUT |= _SCS;										// SCS high, ready talking to display
	SPIWriteByte(MLCD_CM | VCOM);						// send clear display memory command
	SPIWriteByte(0);									// send command trailer
	P2OUT &= ~_SCS;										// SCS lo, finished talking to display

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
		P2OUT |= _SCS;									// SCS high, ready talking to display
		SPIWriteByte(MLCD_SM | VCOM);					// send static mode command
		SPIWriteByte(0);								// send command trailer
		P2OUT &= ~_SCS;									// SCS lo, finished talking to display

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
	P2OUT |= _SCS;										// SCS high, ready talking to display

	SPIWriteByte(MLCD_WR | VCOM);						// send command to write line(s)
	SPIWriteByte(line+1);								// send line address

	UCB0CTL0 |= UCMSB;									// switch SPI to MSB first for proper bitmap orientation

	unsigned char j = 0;
	while(j < (PIXELS_X/8))								// write pixels / 8 bytes
	{
		UCB0TXBUF = LineBuff[j++];						// transfer byte
		while (UCB0STAT & UCBUSY);						// wait for transfer to complete
 	}

	UCB0CTL0 &= ~UCMSB;									// switch SPI back to LSB first for commands

	SPIWriteWord(0);									// send 16 bit to latch buffers and end transfer
	P2OUT &= ~_SCS;										// SCS low, finished talking to display
}

// send one byte over SPI, does not handle SCS
// input: value		byte to be sent
void SPIWriteByte(unsigned char value)
{
	UCB0TXBUF = value;
	while (UCB0STAT & UCBUSY);
}

// send one byte over SPI, does not handle SCS
// input: value		word to be sent
void SPIWriteWord(unsigned int value)
{
	UCB0TXBUF = value & 0xff;
	while (UCB0STAT & UCBUSY);

	UCB0TXBUF = value >> 8;
	while (UCB0STAT & UCBUSY);
}

// interrupt service routine to handle timer A
#pragma vector=TIMER0_A0_VECTOR
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

