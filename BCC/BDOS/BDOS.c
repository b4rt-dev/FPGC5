// Bart's Drive Operating System(BDOS)
/* TODO:
- Reimplement (and optimize?!) using BCC
- Implement more Syscalls (network?)
- More shell functionality (folders, copy, move, etc.)
*/

// As of writing, BCC assigns 4 memory addresses to ints, so we should use chars instead
// However, this is confusing, so we typedef it to word, since that is what it basically is
#define word char


// Defines (also might be used by included libraries below)

// Address of loaded user program
#define RUN_ADDR 0x400000

// Backup of current path
#define FS_PATH_MAX_LENGHT 256

// Temp address for (potentially) large temporary outputs/buffers
// eg: output of listDir or print chunks. 
#define TEMP_ADDR 0x200000

// Syscalls use the same tmp address for input and output
#define SYSCALL_RETVAL_ADDR 0x200000

// Interrupt IDs for extended interrupt handler
#define INTID_TIMER2 0x0
#define INTID_TIMER3 0x1
#define INTID_PS2 0x2
#define INTID_UART1 0x3
#define INTID_UART2 0x4


// Flag that indicates whether a user program is running
// Defined above the defines, so netloader and shell can also access it
word UserprogramRunning = 0;

// These functions are used by some of the other libraries
void BDOS_Backup();
void BDOS_Restore();

// Path variable and its backup variable
char SHELL_path[FS_PATH_MAX_LENGHT];
char SHELL_pathBackup[FS_PATH_MAX_LENGHT];

// Data includes
#include "data/ASCII_BW.c"
#include "data/PS2SCANCODES.c"
#include "data/USBSCANCODES.c"

// Code includes
// Note that these directories are relative to the directory from this file
#include "lib/stdlib.c"
#include "lib/math.c"
#include "lib/gfx.c"
#include "lib/hidfifo.c"
#include "lib/ps2.c"
#include "lib/usbkeyboard.c"
#include "lib/fs.c"
#include "lib/wiz5500.c"
#include "lib/netloader.c"
#include "lib/shell.c"


// Initializes CH376 and mounts drive
// returns 1 on success
word BDOS_Init_FS()
{
    if (FS_init() != FS_ANSW_RET_SUCCESS)
    {
        GFX_PrintConsole("Error initializing CH376 for FS");
        return 0;
    }

    delay(10);

    if (FS_connectDrive() != FS_ANSW_USB_INT_SUCCESS)
    {
        GFX_PrintConsole("Could not mount drive");
        return 0;
    }
    return 1;
}

void BDOS_Reinit_VRAM()
{
    GFX_initVram(); // clear all VRAM
    GFX_copyPaletteTable((word)DATA_PALETTE_DEFAULT);
    GFX_copyPatternTable((word)DATA_ASCII_DEFAULT);

    GFX_cursor = 0;
}

void BDOS_Backup()
{
    // TODO look into what to backup
}

void BDOS_Restore()
{
    // Restore graphics (trying to keep text in window plane)
    GFX_copyPaletteTable((word)DATA_PALETTE_DEFAULT);
    GFX_copyPatternTable((word)DATA_ASCII_DEFAULT);
    GFX_clearBGtileTable();
    GFX_clearBGpaletteTable();
    GFX_clearWindowpaletteTable();
    GFX_clearSprites();

    // Restore netloader
    NETLOADER_init(NETLOADER_SOCKET);
}


int main() 
{
    // Indicate that no user program is running
    UserprogramRunning = 0;

    // Start with loading ASCII table and set palette
    BDOS_Reinit_VRAM();
    
    // Print welcome message
    GFX_PrintConsole("Starting BDOS\n");

    GFX_PrintConsole("Init network...");
    NETLOADER_init(NETLOADER_SOCKET);
    GFX_PrintConsole("DONE\n");


    // Init file system
    GFX_PrintConsole("Init filesystem...");
    if (!BDOS_Init_FS())
        return 0;
    GFX_PrintConsole("DONE\n");

    // Init USB keyboard driver
    GFX_PrintConsole("Init USB keyboard...");
    USBkeyboard_init();
    GFX_PrintConsole("DONE\n");


    // Init shell
    SHELL_init();

    // Main loop
    while (1)
    {
        // Block when downloading file
        if (NETLOADER_transferState == NETLOADER_STATE_USB_DATA)
        {
            // New shell line
            GFX_PrintcConsole('\n');

            // clear buffer
            SHELL_clearCommand();

            // print shell prompt
            SHELL_print_prompt();

            GFX_PrintConsole("Downloading");
            word loopCount = 0; // counter for animation
            word smallLoopCount = 0; // to slow down the animation

            while (NETLOADER_transferState == NETLOADER_STATE_USB_DATA)
            {
                NETLOADER_loop(NETLOADER_SOCKET);

                // indicate progress
                if (loopCount == 3 && smallLoopCount == 4)
                {
                    GFX_PrintcConsole(0x8); // backspace
                    GFX_PrintcConsole(0x8); // backspace
                    GFX_PrintcConsole(0x8); // backspace
                    loopCount = 0;
                }
                else
                {
                    if (smallLoopCount == 4)
                    {
                        GFX_PrintcConsole('.');
                        loopCount++;
                    }
                }

                if (smallLoopCount == 4)
                {
                    smallLoopCount = 0;
                }
                else
                {
                    smallLoopCount++;
                }
            }
            
            // remove the dots
            for (loopCount; loopCount > 0; loopCount--)
            {
                GFX_PrintcConsole(0x8); // backspace
            }

            // remove the loading text
            word i;
            for (i = 0; i < 11; i++)
            {
                GFX_PrintcConsole(0x8); // backspace
            }

        }

        SHELL_loop();
        NETLOADER_loop(NETLOADER_SOCKET);

        // If we received a program, run it and print shell prompt afterwards
        if (NETLOADER_checkDone())
        {
            BDOS_Restore();
            SHELL_print_prompt();
        }
    }

    return 'f';
}


// System call handler
/* Syscall table:
0 - Nothing
1 - HID_FifoAvailable
2 - HID_FifoRead
3 - GFX_PrintcConsole
4 - Get arguments
5 - Get path (backup)
6 - Get USB keyboard buffer (8 words)
*/
void syscall()
{
    word* p = (word*) SYSCALL_RETVAL_ADDR;
    word ID = p[0];

    switch(ID)
    {
        case 1: // HID_FifoAvailable()
            p[0] = HID_FifoAvailable();
            break;
        case 2: // HID_FifoRead()
            p[0] = HID_FifoRead();
            break;
        case 3: // GFX_PrintcConsole()
            GFX_PrintcConsole(p[1]);
            p[0] = 0;
            break;
        case 4: // Get arguments
            p[0] = SHELL_command;
            break;
        case 5: // Get path (backup)
            p[0] = SHELL_pathBackup;
            break;
        case 6: // Get usb keyboard buffer
            p[0] = USBkeyboard_buffer_parsed;
            break;
        default:
            p[0] = 0;
            break;
    }
}

// timer1 interrupt handler
void int1()
{
    
    timer1Value = 1; // notify ending of timer1 (in BDOS)

    // Check if a user program is running
    if (UserprogramRunning)
    {
        // Call int1() of user program
        asm(
            "; backup registers\n"
            "push r1\n"
            "push r2\n"
            "push r3\n"
            "push r4\n"
            "push r5\n"
            "push r6\n"
            "push r7\n"
            "push r8\n"
            "push r9\n"
            "push r10\n"
            "push r11\n"
            "push r12\n"
            "push r13\n"
            "push r14\n"
            "push r15\n"

            "savpc r1\n"
            "push r1\n"
            "jump 0x400001\n"

            "; restore registers\n"
            "pop r15\n"
            "pop r14\n"
            "pop r13\n"
            "pop r12\n"
            "pop r11\n"
            "pop r10\n"
            "pop r9\n"
            "pop r8\n"
            "pop r7\n"
            "pop r6\n"
            "pop r5\n"
            "pop r4\n"
            "pop r3\n"
            "pop r2\n"
            "pop r1\n"
            );
        return;
    }
    else
    {
        
    }
    
}

// extended interrupt handler
void int2()
{
    
    int i = getIntID();
    if (i == INTID_PS2)
    {
        // handle PS2 interrupt
        PS2_HandleInterrupt();
    }
    else if (i == INTID_TIMER2)
    {
        // handle USB keyboard interrupt
        USBkeyboard_HandleInterrupt();
    }

    // Check if a user program is running
    if (UserprogramRunning)
    {
        // Call int2() of user program
        asm(
            "; backup registers\n"
            "push r1\n"
            "push r2\n"
            "push r3\n"
            "push r4\n"
            "push r5\n"
            "push r6\n"
            "push r7\n"
            "push r8\n"
            "push r9\n"
            "push r10\n"
            "push r11\n"
            "push r12\n"
            "push r13\n"
            "push r14\n"
            "push r15\n"

            "savpc r1\n"
            "push r1\n"
            "jump 0x400002\n"

            "; restore registers\n"
            "pop r15\n"
            "pop r14\n"
            "pop r13\n"
            "pop r12\n"
            "pop r11\n"
            "pop r10\n"
            "pop r9\n"
            "pop r8\n"
            "pop r7\n"
            "pop r6\n"
            "pop r5\n"
            "pop r4\n"
            "pop r3\n"
            "pop r2\n"
            "pop r1\n"
            );
        return;
    }
    else
    {
        
    }

    
}

// UART0 interrupt handler
void int3()
{

    // Check if a user program is running
    if (UserprogramRunning)
    {
        // Call int3() of user program
        asm(
            "; backup registers\n"
            "push r1\n"
            "push r2\n"
            "push r3\n"
            "push r4\n"
            "push r5\n"
            "push r6\n"
            "push r7\n"
            "push r8\n"
            "push r9\n"
            "push r10\n"
            "push r11\n"
            "push r12\n"
            "push r13\n"
            "push r14\n"
            "push r15\n"

            "savpc r1\n"
            "push r1\n"
            "jump 0x400003\n"

            "; restore registers\n"
            "pop r15\n"
            "pop r14\n"
            "pop r13\n"
            "pop r12\n"
            "pop r11\n"
            "pop r10\n"
            "pop r9\n"
            "pop r8\n"
            "pop r7\n"
            "pop r6\n"
            "pop r5\n"
            "pop r4\n"
            "pop r3\n"
            "pop r2\n"
            "pop r1\n"
            );
        return;
    }
    else
    {
        
    }

}

// GFX framedrawn interrupt handler
void int4()
{

    // Check if a user program is running
    if (UserprogramRunning)
    {
        // Call int4() of user program
        asm(
            "; backup registers\n"
            "push r1\n"
            "push r2\n"
            "push r3\n"
            "push r4\n"
            "push r5\n"
            "push r6\n"
            "push r7\n"
            "push r8\n"
            "push r9\n"
            "push r10\n"
            "push r11\n"
            "push r12\n"
            "push r13\n"
            "push r14\n"
            "push r15\n"

            "savpc r1\n"
            "push r1\n"
            "jump 0x400004\n"

            "; restore registers\n"
            "pop r15\n"
            "pop r14\n"
            "pop r13\n"
            "pop r12\n"
            "pop r11\n"
            "pop r10\n"
            "pop r9\n"
            "pop r8\n"
            "pop r7\n"
            "pop r6\n"
            "pop r5\n"
            "pop r4\n"
            "pop r3\n"
            "pop r2\n"
            "pop r1\n"
            );
        return;
    }
    else
    {
        
    }

}