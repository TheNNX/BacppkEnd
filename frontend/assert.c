#include "string.h"
#include "wasm.h"

void itoa(char* buffer, int i)
{
    int pow = 1;
    int j = i;
    int idx = 0;

    if (i == 0)
    {
        buffer[0] = '0';
        buffer[1] = 0;
        return;
    }
    else if (i < 0)
    {
        buffer[idx++] = '-';
        i = -i;
    }

    while (j / 10)
    {
        pow *= 10;
        j /= 10; 
    }

    while (pow)
    {
        buffer[idx++] = '0' + ((i / pow) % 10);
        pow /= 10;
    }

    buffer[idx] = 0;
}

void _assert(const char* message, const char* file, int line)
{
    char buffer[4096] = { 0 };
    char lineStr[16] = { 0 };

    itoa(lineStr, line);

    strcpy(buffer, "alert('Assertion failed: (");
    strcat(buffer, file);
    strcat(buffer, ":");
    strcat(buffer, lineStr);
    strcat(buffer, "): ");
    strcat(buffer, message);
    strcat(buffer, "');");

    Eval(buffer);
}