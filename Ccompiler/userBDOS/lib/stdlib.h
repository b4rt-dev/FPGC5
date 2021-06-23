#include "math.h" 

#define UART_TX_ADDR 0xC02723

// Timer I/O Addresses
#define TIMER1_VAL 0xC02739
#define TIMER1_CTRL 0xC0273A
#define TIMER2_VAL 0xC0273B
#define TIMER2_CTRL 0xC0273C
#define TIMER3_VAL 0xC0273D
#define TIMER3_CTRL 0xC0273E

int timer1Value = 0;
int timer2Value = 0;
int timer3Value = 0;

/*
* TODO:
* - Convert most of these functions to assembly
*/

/*
Copies n characters from src to dest
*/
void memcpy(char* dest, char* src, int n)
{
    for (int i = 0; i < n; i++)
    {
        dest[i] = src[i];
    }
}


/*
Copies string from src to dest
Returns number of characters copied
*/
int strcpy(char* dest, char* src)
{
    // write to buffer
    int i = 0;
    while (src[i] != 0)
    {
        dest[i] = src[i];
        i += 1;
    }

    dest[i] = 0;

    return i;
}


/*
appends string from src to dest
Returns number of characters appended
*/
int strcat(char* dest, char* src)
{
    int endOfDest = 0;
    while (dest[endOfDest] != 0)
        endOfDest += 1;

    return strcpy(dest+endOfDest, src);
}


/*
Prints a single char c by writing it to 0xC02723
*/
void uprintc(char c) 
{
    int *p = (int *)UART_TX_ADDR;   // address of UART TX
    *p = (int)c;            // write char over UART
}


/*
Sends each character from str over UART
by writing them to 0xC02723
until a 0 value is found.
Does not send a newline afterwards.
*/
void uprint(char* str) 
{
    int *p = (int *)UART_TX_ADDR;   // address of UART TX
    char chr = *str;            // first character of str

    while (chr != 0)            // continue until null value
    {
        *p = (int)chr;          // write char over UART
        str++;                  // go to next character address
        chr = *str;             // get character from address
    }
}


/*
Same as uprint(char* str),
except it sends a newline afterwards.
*/
void uprintln(char* str) 
{
    uprint(str);
    uprint("\n");
}



/*
Recursive helper function for itoa
Eventually returns the number of digits in n
*/
int itoar(int n, char *s)
{
    int digit = MATH_mod(n, 10);
    int i = 0;

    n = MATH_div(n,10);
    if (n > 0)
        i += itoar(n, s);

    s[i++] = digit + '0';

    return i;
}


/*
Converts integer n to characters.
The characters are placed in the buffer s.
The buffer is terminated with a 0 value.
Uses recursion, division and mod to compute.
*/
void itoa(int n, char *s)
{
    // compute and fill the buffer
    int i = itoar(n, s);

    // end with terminator
    s[i] = 0;
} 





/*
Recursive helper function for itoa
Eventually returns the number of digits in n
*/
int itoahr(int n, char *s)
{
    int digit = MATH_mod(n, 16);
    int i = 0;

    n = MATH_div(n,16);
    if (n > 0)
        i += itoahr(n, s);

    char c;
    if (digit > 9)
    {
        c = digit + 'A' - 10;
    }
    else
    {
        c = digit + '0';
    }
    s[i++] = c;

    return i;
}


/*
Converts integer n to hex string characters.
The characters are placed in the buffer s.
A prefix of 0x is added.
The buffer is terminated with a 0 value.
Uses recursion, division and mod to compute.
*/
void itoah(int n, char *s)
{
    // add prefix
    s[0] = '0';
    s[1] = 'x';
    s+=2;

    // compute and fill the buffer
    int i = itoahr(n, s);

    // end with terminator
    s[i] = 0;
} 



// sleeps ms using timer1.
// blocking.
// requires int1() to set timer1Value to 1:
/*
    timer1Value = 1; // notify ending of timer1
*/
void delay(int ms)
{

    // clear result
    timer1Value = 0;

    // set timer
    int *p = (int *) TIMER1_VAL;
    *p = ms;
    // start timer
    int *q = (int *) TIMER1_CTRL;
    *q = 1;

    // wait until timer done
    while (timer1Value == 0);
}


// returns interrupt ID by using the readintid asm instruction
// OUTPUT:
//   r1: read value
int getIntID()
{
    ASM("\
    readintid r1            // reads interrupt id to r1 ;\
    ");
}



//Converts char c to uppercase if possible
char toUpper(char c)
{
    if (c>96 && c<123) 
        c = c ^ 0x20;

    return c;
}
