#define UART_TX_ADDR 0xC02723

// isalpha
STATIC
word isalpha(word argument)
{
  if (argument >= 'A' && argument <= 'Z')
    return 2;
  if (argument >= 'a' && argument <= 'z')
    return 1;
  return 0;
}

// isdigit
STATIC
word isdigit(word argument)
{
  if (argument >= '0' && argument <= '9')
    return 1;
  return 0;
}

// isalnum
STATIC
word isalnum(word argument)
{
  if (isdigit(argument) || isalpha(argument))
    return 1;
  return 0;
}



STATIC
void* memcpy(void *dest, const void *src, size_t len)
{
   // Typecast src and dest addresses to (char *)
   char *csrc = (char *)src;
   char *cdest = (char *)dest;
  
   // Copy contents of src[] to dest[]
   word i;
   for (i=0; i<len; i++)
       cdest[i] = csrc[i];
}

STATIC
void* memmove(void* dest, const void* src, size_t n)
{
  unsigned char* from = (unsigned char*) src;
  unsigned char* to = (unsigned char*) dest;

  if (from == to || n == 0)
    return dest;
  if (to > from && to-from < (word)n) {
    /* to overlaps with from */
    /*  <from......>         */
    /*         <to........>  */
    /* copy in reverse, to avoid overwriting from */
    word i;
    for(i=n-1; i>=0; i--)
      to[i] = from[i];
    return dest;
  }
  if (from > to && from-to < (word)n) {
    /* to overlaps with from */
    /*        <from......>   */
    /*  <to........>         */
    /* copy forwards, to avoid overwriting from */
    size_t i;
    for(i=0; i<n; i++)
      to[i] = from[i];
    return dest;
  }
  memcpy(dest, src, n);
  return dest;
}

// Function to implement `strcpy()` function
STATIC
char* strcpy(char* destination, const char* source)
{
    // return if no memory is allocated to the destination
    if (destination == NULL) {
        return NULL;
    }
 
    // take a pointer pointing to the beginning of the destination string
    char *ptr = destination;
 
    // copy the C-string pointed by source into the array
    // pointed by destination
    while (*source != '\0')
    {
        *destination = *source;
        destination++;
        source++;
    }
 
    // include the terminating null character
    *destination = '\0';
 
    // the destination is returned by standard `strcpy()`
    return ptr;
}


STATIC
size_t strlen(const char *str)
{
        const char *s;
        for (s = str; *s; ++s);
        return (s - str);
}

STATIC
char* strcat (char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}

STATIC
char* strchr (const char *s, word c)
{
  do {
    if (*s == c)
      {
        return (char*)s;
      }
  } while (*s++);
  return (0);
}


STATIC
word strcmp(const char* s1, const char* s2)
{
  while(*s1 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }
  return *(unsigned char*)s1 - *(unsigned char*)s2;
}

//STATIC

word strncmp(const char * s1, const char * s2, size_t n )
{
  while ( n && *s1 && ( *s1 == *s2 ) )
  {
    ++s1;
    ++s2;
    --n;
  }
  if ( n == 0 )
  {
    return 0;
  }
  else
  {
    return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
  }
}



/*
Recursive helper function for itoa
Eventually returns the number of digits in n
s is the output buffer
*/
word itoar(word n, char *s)
{
    word digit = MATH_modU(n, 10);
    word i = 0;

    n = MATH_divU(n,10);
    if ((unsigned int) n > 0)
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
void itoa(word n, char *s)
{
    // compute and fill the buffer
    word i = itoar(n, s);

    // end with terminator
    s[i] = 0;
} 





/*
Recursive helper function for itoa
Eventually returns the number of digits in n
s is the output buffer
*/
word itoahr(word n, char *s)
{
    word digit = MATH_modU(n, 16);
    word i = 0;

    n = MATH_divU(n,16);
    if ((unsigned int) n > 0)
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
void itoah(word n, char *s)
{
    // add prefix
    s[0] = '0';
    s[1] = 'x';
    s+=2;

    // compute and fill the buffer
    word i = itoahr(n, s);

    // end with terminator
    s[i] = 0;
} 


/*
Converts string into int.
Assumes the string is valid.
*/
word strToInt(char* str)
{
    word retval = 0;
    word multiplier = 1;
    word i = 0;
    while (str[i] != 0)
    {
        i++;
    }
    if (i == 0)
        return 0;

    i--;

    while (i > 0)
    {
        // Return 0 if not a digit
        if (str[i] < '0' || str[i] > '9')
            return 0;
        
        word currentDigit = str[i] - '0';
        word toAdd = multiplier * currentDigit;
        retval += toAdd;
        multiplier = multiplier * 10;
        i--;
    }

    word currentDigit = str[i] - '0';
    word toAdd = multiplier * currentDigit;
    retval += toAdd;

    return retval;
}


/*
Prints a single char c by writing it to UART_TX_ADDR
*/
void uprintc(char c) 
{
    word *p = (word *)UART_TX_ADDR; // address of UART TX
    *p = (word)c;                   // write char over UART
}


/*
Sends each character from str over UART
by writing them to UART_TX_ADDR
until a 0 value is found.
Does not send a newline afterwards.
*/
void uprint(char* str) 
{
    word *p = (word *)UART_TX_ADDR; // address of UART TX
    char chr = *str;                // first character of str

    while (chr != 0)                // continue until null value
    {
        *p = (word)chr;             // write char over UART
        str++;                      // go to next character address
        chr = *str;                 // get character from address
    }
}


/*
Same as uprint(char* str),
except it sends a newline afterwards.
*/
void uprintln(char* str) 
{
    uprint(str);
    uprintc('\n');
}


/*
Prints decimal integer over UART
*/
void uprintDec(word i) 
{
    char buffer[11];
    itoa(i, buffer);
    uprint(buffer);
    uprintc('\n');
}

/*
Prints hex integer over UART
*/
void uprintHex(word i) 
{
    char buffer[11];
    itoah(i, buffer);
    uprint(buffer);
    uprintc('\n');
}


/*
Prints decimal integer over UART, with newline
*/
void uprintlnDec(word i) 
{
    char buffer[11];
    itoa(i, buffer);
    uprint(buffer);
    uprintc('\n');
}

/*
Prints hex integer over UART, with newline
*/
void uprintlnHex(word i) 
{
    char buffer[11];
    itoah(i, buffer);
    uprint(buffer);
    uprintc('\n');
}