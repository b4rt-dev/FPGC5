/*
* Library for handling PS/2 Scancodes
* Reads scancodes and translates them to ASCII values
* For certain extended keys like the arrow keys,
*  a virtual code > 256 is used.
* Writes resulting code to hid FIFO.
*
* TODO:
* - implement alt, control
* - implement caps
* - implement F-keys
* - implement ps, sl, pb
* - implement windows key
*/

#define PS2_ADDR 0xC02740

#include "../data/PS2SCANCODES.h"
#include "hidfifo.h"

int ps2caps = 0;
int ps2shifted = 0;
int ps2controlled = 0;
int ps2alted = 0;
int ps2extended = 0;
int ps2released = 0;


void PS2_HandleInterrupt() 
{
    int *scanCode = (int *) PS2_ADDR;

    // last key was a break code
    if (ps2released)  
    {
        // left or right shift
        if (*scanCode == 0x59 || *scanCode == 0x12)
            ps2shifted = 0;

        // extended
        if (ps2extended)
            ps2extended = 0;

        if (*scanCode == 0x14)
            ps2controlled = 0;

        ps2released = 0;
    }
    else
    {
        // extended
        if (*scanCode == 0xE0)
        {
            ps2extended = 1;
        }
        else
        {
            if (*scanCode == 0x59 || *scanCode == 0x12) // left or right shift
                ps2shifted = 1;
            else if (*scanCode == 0xF0) // break code
                ps2released = 1;
            else if (*scanCode == 0x14) // control
                ps2controlled = 1;
            else
            {
                if (ps2extended)
                {
                    int *tableExtended = (int*) &DATA_PS2SCANCODE_EXTENDED;
                    tableExtended += 4; // set offset to data in function
                    HID_FifoWrite(*(tableExtended+*scanCode));
                }
                else if (ps2shifted)
                {
                    int *tableShifted = (int*) &DATA_PS2SCANCODE_SHIFTED;
                    tableShifted += 4; // set offset to data in function
                    HID_FifoWrite(*(tableShifted+*scanCode));
                }
                else
                {
                    int *tableNomal = (int*) &DATA_PS2SCANCODE_NORMAL;
                    tableNomal += 4; // set offset to data in function
                    HID_FifoWrite(*(tableNomal+*scanCode));
                }
            }
        }
    }
}

