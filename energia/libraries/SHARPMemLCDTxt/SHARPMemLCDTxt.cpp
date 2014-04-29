// Created by Adrian Studer, April 2014.
// Distributed under MIT License, see license.txt for details.

#include <Arduino.h>
#include <SPI.h>
#include "SHARPMemLCDTxt.h"
#include "font.h"

#define CMD_WR   0x01
#define CMD_CLR  0x04
#define CMD_NOP  0x00
#define CMD_VCOM 0x02

volatile char spi_busy = 0;

SHARPMemLCDTxt::SHARPMemLCDTxt(char pinCS, char pinDISP, char pinVCOM)
    : m_pinCS(pinCS), m_pinDISP(pinDISP), m_pinVCOM(pinVCOM), m_stateVCOM(0)
{
}

SHARPMemLCDTxt::~SHARPMemLCDTxt()
{
}

void SHARPMemLCDTxt::begin()
{
    pinMode(m_pinCS, OUTPUT);
    pinMode(m_pinDISP, OUTPUT);

    digitalWrite(m_pinCS, LOW);
    digitalWrite(m_pinDISP, LOW);

    if (m_pinVCOM != 0) {
        pinMode(m_pinVCOM, OUTPUT);
    } else {
        m_millis = millis();
    }

    // configure SPI
    SPI.begin();
    SPI.setBitOrder(0);			// bit order LSB first
    SPI.setClockDivider(F_CPU/1000000);	// run SPI at 1MHz
}

void SHARPMemLCDTxt::on()
{
    digitalWrite(m_pinDISP, HIGH);
}

void SHARPMemLCDTxt::off()
{
    digitalWrite(m_pinDISP, LOW);
}

void SHARPMemLCDTxt::clear()
{
    spi_busy = 1;

    SPI.setBitOrder(0);			// bit order LSB first

    digitalWrite(m_pinCS, HIGH);
    SPI.transfer(CMD_CLR | m_stateVCOM);
    SPI.transfer(0);
    digitalWrite(m_pinCS, LOW);

    spi_busy = 0;
}

void SHARPMemLCDTxt::print(const char* text, char line, char options)
{
    pulse(0);

    SPI.setBitOrder(0);			// bit order LSB first

    // c = char
    // b = bitmap
    // i = text index
    // j = line buffer index
    // k = char line
    unsigned char c, b, i, j, k;

    // rendering happens line-by-line because this display can only be written by line
    k = 0;
    while (k < 8 && line < PIXELS_Y) { // loop for 8 character lines while within display
        i = 0;
        j = 0;
        while (j < (PIXELS_X/8) && (c = text[i]) != 0) {     // while we did not reach end of line or string
            if (c < ' ' || c > 'Z') {                        // invalid characters are replace with SPACE
                c = ' ';
            }

            c = c - 32;                                      // convert character to index in font table
            b = font8x8[(c*8)+k];                            // retrieve byte defining one line of character

            if (!(options & DISP_INVERT)) {                  // invert bits if DISP_INVERT is _NOT_ selected
                b = ~b;// pixels are LOW active
            }

            if ((options & DISP_WIDE) && (c != 0)) {         // double width rendering if DISP_WIDE and character is not SPACE
                doubleWide(b, j);		             
                j += 2;                                      // we've written two bytes to buffer
            } else {                                         // else regular rendering
                m_buffer[j] = b;                             // store pixels in line buffer
                j++;                                         // we've written one byte to buffer
            }

            i++;                                             // next character
        }

        while (j < (PIXELS_X/8)) {                           // pad line for empty characters
            m_buffer[j] = 0xff;
            j++;
        }

        writeBuffer(line++);                                 // write line buffer to display

        if (options & DISP_HIGH && line < PIXELS_Y) {        // repeat line if DISP_HIGH is selected
            writeBuffer(line++);
        }

        k++;                                                 // next pixel line
    }
}

void SHARPMemLCDTxt::pulse(int force)
{
    int update = 1;

    if (!force) {
        unsigned long time = millis();
        if (time - m_millis > 500) {
            m_millis = time;
        } else {
            update = 0;
        }
    }

    if (update) {
        if (m_pinVCOM != 0) {
            digitalWrite(m_pinVCOM, HIGH);
            delayMicroseconds(1);
            digitalWrite(m_pinVCOM, LOW);
        } else {
            m_stateVCOM ^= CMD_VCOM;
            if (!spi_busy) {
                spi_busy = 1;
                SPI.setBitOrder(0);			// bit order LSB first
                digitalWrite(m_pinCS, HIGH);
                SPI.transfer(CMD_NOP | m_stateVCOM);
                SPI.transfer(0);
                digitalWrite(m_pinCS, LOW);
                spi_busy = 0;
            }
        }
    }
}

void SHARPMemLCDTxt::doubleWide(char b, char j)
{
    int i = 8;
    unsigned int c = 0;

    do {
        c <<= 2;
        if (b & 0x80) {
            c |= 3;
        }
        b <<= 1;
        i--;
    } while (i != 0);

    m_buffer[j] = c >> 8;
    m_buffer[j+1] = c & 0xff;
}

void SHARPMemLCDTxt::writeBuffer(char line)
{
    if (line > PIXELS_Y) return;         // ignore writing to invalid lines

    spi_busy = 1;

    digitalWrite(m_pinCS, HIGH);

    SPI.transfer(CMD_WR | m_stateVCOM);  // send command to write line(s)

    SPI.transfer(line+1);                // send line address

    SPI.setBitOrder(1);                  // switch SPI to MSB first for proper bitmap orientation

    char j = 0;
    while (j < (PIXELS_X/8)) {           // write pixels / 8 bytes
        SPI.transfer(m_buffer[j++]);
    }

    SPI.transfer(0);                     // transfer 16 bit to latch buffers and end transmission
    SPI.transfer(0);

    SPI.setBitOrder(0);                  // switch SPI back to LSB first for commands

    digitalWrite(m_pinCS, LOW);

    spi_busy = 0;
}
