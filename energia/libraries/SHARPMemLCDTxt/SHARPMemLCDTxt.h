// Created by Adrian Studer, April 2014.
// Distributed under MIT License, see license.txt for details.

#ifndef __SHARPMEMLCDTXT_H__
#define __SHARPMEMLCDTXT_H__

#define DISP_INVERT 1
#define DISP_WIDE 2
#define DISP_HIGH 4

#ifndef PIXELS_X
#define PIXELS_X 96
#endif

#ifndef PIXELS_Y
#define PIXELS_Y 96
#endif

class SHARPMemLCDTxt
{
private:
    char m_pinCS;
    char m_pinDISP;
    char m_pinVCOM;
    char m_stateVCOM;
    unsigned long m_millis;
    char m_buffer[PIXELS_X/8];

    void writeBuffer(char line);
    void doubleWide(char b, char j);

public:
    SHARPMemLCDTxt(char pinCS = 13,
                   char pinDISP = 8,
                   char pinVCOM = 0);
    ~SHARPMemLCDTxt();
    void begin();
    void clear();
    void on();
    void off();
    void print(const char* text, char line, char options = 0);
    void pulse(int force = 0);
    void bitmap(const unsigned char* bitmap, int width, int height, char line, char options = 0);
};

#endif
