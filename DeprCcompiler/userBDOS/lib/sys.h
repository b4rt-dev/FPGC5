/*
Contains System Call functions
*/

#define SYSCALL_RETVAL_ADDR 0x200000

// Interrupt IDs for extended interrupt handler
#define INTID_TIMER2 0x0
#define INTID_TIMER3 0x1
#define INTID_PS2 0x2
#define INTID_UART1 0x3
#define INTID_UART2 0x4


// executes system call to BDOS
// ID is written to the same location as the output of the system call
//  at address SYSCALL_RETVAL_ADDR
// This address is also returned
int* syscall(int ID)
{
    int* p = (int*) SYSCALL_RETVAL_ADDR;
    *p = ID;

    ASM("\
    push r1 ;\
    push r2 ;\
    push r3 ;\
    push r4 ;\
    push r5 ;\
    push r6 ;\
    push r7 ;\
    push r8 ;\
    push r9 ;\
    push r10 ;\
    push r11 ;\
    push r12 ;\
    push r13 ;\
    push rbp ;\
    push rsp ;\
    savpc r1 ;\
    push r1 ;\
    jump 6 ;\
    pop rsp ;\
    pop rbp ;\
    pop r13 ;\
    pop r12 ;\
    pop r11 ;\
    pop r10 ;\
    pop r9 ;\
    pop r8 ;\
    pop r7 ;\
    pop r6 ;\
    pop r5 ;\
    pop r4 ;\
    pop r3 ;\
    pop r2 ;\
    pop r1 ;\
    ");

    return p;
}


int HID_FifoAvailable()
{
    int* p = syscall(1);
    return p[0];
}


int HID_FifoRead()
{
    int* p = syscall(2);
    return p[0];
}


void BDOS_PrintcConsole(char c)
{
    int* p = (int*) SYSCALL_RETVAL_ADDR;
    p[1] = c;
    syscall(3);
}

// Prints string on BDOS console untill terminator
// Does not add newline at end
void BDOS_PrintConsole(char* str)
{
    char chr = *str;            // first character of str

    while (chr != 0)            // continue until null value
    {
        BDOS_PrintcConsole(chr);
        str++;                  // go to next character address
        chr = *str;             // get character from address
    }
}

void BDOS_PrintDecConsole(int i)
{
    char buffer[11];
    itoa(i, &buffer[0]);
    BDOS_PrintConsole(&buffer[0]);
}


void BDOS_PrintHexConsole(int i)
{
    char buffer[11];
    itoah(i, &buffer[0]);
    BDOS_PrintConsole(&buffer[0]);
}


// Returns command line args
char* BDOS_GetArgs()
{
    int* p = syscall(4);
    return (char*) p[0];
}


// Writes command line argument n into buf
// Arg 0 is the command itself
void BDOS_GetArgN(int n, char* buf)
{
    char* args = BDOS_GetArgs();

    int i = 0;
    int bufi = 0;
    int currentArg = 0;
    char prevChar = 0;
    buf[0] = 0;
    while (args[i] != 0)
    {
        // new argument
        if (args[i] == ' ' && prevChar != ' ')
        {
            currentArg++;
        }

        if (args[i] != ' ')
        {
            if (currentArg == n)
            {
                buf[bufi] = args[i];
                bufi++;
            }
        }

        prevChar = args[i];
        i++;
    }

    buf[bufi] = 0; // terminate
}


// Returns BDOS current path
char* BDOS_GetPath()
{
    int* p = syscall(5);
    return (char*) p[0];
}