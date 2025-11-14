#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "string.h"
#include "stdio.h"
#include "assert.h"
#include "stdlib.h"
#include "wasm.h"

typedef struct PutcharCtx
{
    int(*Function)(struct PutcharCtx* self, char c);
} PutcharCtx;

typedef enum _ALIGNMENT
{
    Left,
    Right
}ALIGNMENT, * PALIGNMENT;

typedef struct _FORMAT_INFO
{
    ALIGNMENT   Alignment;
    char        Prefix;
    char        Fill;
    int         Width;
    char        Type;
    size_t      IntSize;
    size_t      FloatingSize;
    bool        IntegerBaseNotation;
    bool        ForceDecimalPoint;
} FORMAT_INFO, * PFORMAT_INFO;

static
bool
ParseFormat(const char* formatPos,
            const char** pOutNewformatPos,
            PFORMAT_INFO pFormatInfo)
{
    bool done = false;
    char lastChar = 0;

    /* Initialize with default values. */
    pFormatInfo->Fill = ' ';
    pFormatInfo->Prefix = 0;
    pFormatInfo->Width = 0;
    pFormatInfo->Type = '?';
    pFormatInfo->Alignment = Right;
    pFormatInfo->IntSize = sizeof(int);
    pFormatInfo->FloatingSize = sizeof(float);
    pFormatInfo->IntegerBaseNotation = false;
    pFormatInfo->ForceDecimalPoint = false;

    while (*formatPos && !done)
    {
        switch (*formatPos)
        {
        case 'c':
        case 's':
        case 'i':
        case 'd':
        case 'u':
        case 'x':
        case 'X':
        case '?':
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
        case 'p':
        case 'o':
        case '%':
            pFormatInfo->Type = *formatPos;
            done = true;
            break;
        case '0':
            if (!(lastChar >= '0' && lastChar <= '9'))
            {
                pFormatInfo->Fill = *formatPos;
                break;
            }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            int digit = *formatPos - '0';
            pFormatInfo->Width = pFormatInfo->Width * 10 + digit;
            break;
        }
        case '#':
            pFormatInfo->IntegerBaseNotation = true;
            pFormatInfo->ForceDecimalPoint = true;
            break;
        case ' ':
        case '+':
            pFormatInfo->Prefix = *formatPos;
            break;
        case '-':
            pFormatInfo->Alignment = Left;
            break;
        case 'l':
            if (lastChar == 'l' && pFormatInfo->IntSize == sizeof(long))
            {
                pFormatInfo->IntSize = sizeof(long long);
            }
            else
            {
                pFormatInfo->IntSize = sizeof(long);
            }
            break;
        case 'h':
            if (lastChar == 'h' && pFormatInfo->IntSize == sizeof(short))
            {
                pFormatInfo->IntSize = sizeof(char);
            }
            else
            {
                pFormatInfo->IntSize = sizeof(short);
            }
            break;
        case 'L':
            pFormatInfo->FloatingSize = sizeof(double);
            break;
        case 'z':
            pFormatInfo->IntSize = sizeof(size_t);
            break;
        case 'j':
            pFormatInfo->IntSize = sizeof(intmax_t);
            break;
        case 't':
            pFormatInfo->IntSize = sizeof(ptrdiff_t);
            break;
        default:
            *pOutNewformatPos = formatPos;
            pFormatInfo->Type = *formatPos;
            return false;
        }
        lastChar = *formatPos;
        formatPos++;
    }

    if (!done)
    {
        *pOutNewformatPos = NULL;
    }
    else
    {
        *pOutNewformatPos = formatPos;
    }
    return done;
}

static
int
GetNumberOfDigits(intmax_t integer,
                  bool bSigned,
                  int base)
{
    uintmax_t uInteger = integer;
    int result = 0;

    if (bSigned && integer < 0)
    {
        uInteger = -integer;
    }

    while (uInteger)
    {
        result++;
        uInteger /= base;
    }

    return result;
}

static
int
PrintInteger(intmax_t integer,
             bool bSigned,
             int base,
             const char* digits,
             PutcharCtx* putchar)
{
    uintmax_t uInteger = integer;
    int numberOfDigits;
    uintmax_t divisor = 1;
    int i, result = 0;

    if (bSigned && integer < 0)
    {
        uInteger = -integer;
    }

    numberOfDigits = GetNumberOfDigits(uInteger, bSigned, base);
    for (i = 0; i < numberOfDigits - 1; i++)
    {
        divisor *= base;
    }

    for (i = 0; i < numberOfDigits; i++)
    {
        assert(divisor != 0);
        uintmax_t mod = (uInteger / divisor) % base;
        divisor /= base;
        result += putchar->Function(putchar, digits[mod]);
    }

    return result;
}

static
int
PrintPrefixAndNotation(char prefix,
                       const char* baseNotation,
                       PutcharCtx* putchar)
{
    int result = 0;

    if (prefix != 0)
    {
        result += putchar->Function(putchar, prefix);
    }
    if (baseNotation != NULL)
    {
        while (*baseNotation)
        {
            result += putchar->Function(putchar, *baseNotation++);
        }
    }

    return result;
}

static
bool
GetIntegerFromVaArg(va_list* pArgs,
                    PFORMAT_INFO formatInfo,
                    uintmax_t* pOutInteger)
{
    uintmax_t integer;

    /* Load the integer from the variadic arguments. */
    if (formatInfo->IntSize == sizeof(long long))
    {
        integer = va_arg(*pArgs, long long);
    }
    else if (formatInfo->IntSize == sizeof(long))
    {
        integer = va_arg(*pArgs, long);
    }
    else if (formatInfo->IntSize == sizeof(int))
    {
        integer = va_arg(*pArgs, int);
    }
    else if (formatInfo->IntSize == sizeof(short) ||
             formatInfo->IntSize == sizeof(char))
    {
        /* Short and chars are apparently promoted in vaargs to ints
         * per the C standart. */
        integer = va_arg(*pArgs, int);
    }
    else if (formatInfo->IntSize == sizeof(size_t))
    {
        integer = va_arg(*pArgs, size_t);
    }
    else if (formatInfo->IntSize == sizeof(intmax_t))
    {
        integer = va_arg(*pArgs, intmax_t);
    }
    else if (formatInfo->IntSize == sizeof(ptrdiff_t))
    {
        integer = va_arg(*pArgs, ptrdiff_t);
    }
    else
    {
        return false;
    }

    *pOutInteger = integer;
    return true;
}

static
uintmax_t
GetInteger(va_list* pArgs, 
           PFORMAT_INFO formatInfo,
           uintmax_t* pOutInteger,
           int* pOutBase,
           char* pOutPrefix,
           bool* pOutSigned,
           bool* pOutCapital,
           const char** pOutBaseNotation)
{
    uintmax_t integer;
    int base = 10;
    bool capital = false;
    bool bSigned = true;
    char prefix = 0;
    const char* baseNotation = "";
    bool msb;

    switch (formatInfo->Type)
    {
    case 'x':
        base = 16;
        bSigned = false;
        if (formatInfo->IntegerBaseNotation)
        {
            baseNotation = "0x";
        }
        break;
    case 'X':
    case 'p':
        base = 16;
        capital = true;
        bSigned = false;
        if (formatInfo->IntegerBaseNotation)
        {
            baseNotation = "0X";
        }
        break;
    case 'i':
    case 'd':
        base = 10;
        break;
    case 'o':
        base = 8;
        if (formatInfo->IntegerBaseNotation)
        {
            baseNotation = "0";
        }
        break;
    case 'u':
        base = 10;
        bSigned = false;
        break;
    }

    if (bSigned)
    {
        prefix = formatInfo->Prefix;
    }

    if (!GetIntegerFromVaArg(pArgs, formatInfo, &integer))
    {
        return false;
    }

    /* Extend the sign if neccessary. */
    msb = (integer & ((intmax_t)1 << (8 * formatInfo->IntSize - 1))) != 0;
    if (bSigned && msb)
    {
        uintmax_t signExtension = ~(((intmax_t)1 << (8 * formatInfo->IntSize - 1)) - 1);
        integer |= signExtension;
    }

    if (bSigned && ((intmax_t)integer) < 0)
    {
        prefix = '-';
    }

    *pOutBase = base;
    *pOutInteger = integer;
    *pOutPrefix = prefix;
    *pOutSigned = bSigned;
    *pOutCapital = capital;
    *pOutBaseNotation = baseNotation;
    return true;
}

static
int
HandlePreAlignment(PFORMAT_INFO formatInfo,
                   size_t dataLength,
                   char prefix,
                   const char* baseNotation,
                   PutcharCtx* putchar)
{
    intmax_t i;
    int result = 0;
    intmax_t fillLength = formatInfo->Width - dataLength;

    if (formatInfo->Alignment == Right && formatInfo->Width != 0)
    {
        /* If the fill is not to be empty, place the sign before it. */
        if (formatInfo->Fill != ' ')
        {
            result += PrintPrefixAndNotation(prefix, baseNotation, putchar);
        }

        for (i = 0; i < fillLength; i++)
        {
            result += putchar->Function(putchar, formatInfo->Fill);
        }

        /* If the fill was done using spaces, place the prefix character after
         * the fill. */
        if (formatInfo->Fill == ' ')
        {
            result += PrintPrefixAndNotation(prefix, baseNotation, putchar);
        }
    }
    else
    {
        /* Print the prefix. In case of negative integers, it is the '-' sign.
         * If the '+' or ' ' flag was specified, its respective character is used
         * for non-negative numbers. */
        result += PrintPrefixAndNotation(prefix, baseNotation, putchar);
    }

    return result;
}

static
int
HandlePostAlignment(PFORMAT_INFO formatInfo,
                    size_t dataLength,
                    PutcharCtx* putchar)
{
    int result = 0;
    intmax_t i;
    intmax_t fillLength = formatInfo->Width - dataLength;

    /* The second possible alignment. */
    if (formatInfo->Alignment == Left && formatInfo->Width != 0)
    {
        for (i = 0; i < fillLength; i++)
        {
            /* The trailing fill is always ' ' */
            result += putchar->Function(putchar, ' ');
        }
    }

    return result;
}

static
int
HandleIntFormat(PFORMAT_INFO formatInfo,
                va_list* pArgs,
                PutcharCtx* putchar)
{
    int numberOfDigits = 0;
    uintmax_t integer;
    char prefix = 0;
    const char* baseNotation = "";
    int base = 10;
    bool capital = false;
    bool bSigned = true;
    size_t dataLength;
    int result = 0;

    if (!GetInteger(
        pArgs,
        formatInfo,
        &integer,
        &base,
        &prefix,
        &bSigned,
        &capital,
        &baseNotation))
    {
        return 0;
    }

    numberOfDigits = GetNumberOfDigits(integer, bSigned, base);
    /* At least formatInfo->Width characters have to be printed.
     * If the sign prefix/space is non-zero, it takes one character.
     * digits of the integer take characters too, obviously.
     * base notation prefix length also has to be subtracted. */
    dataLength = numberOfDigits + (prefix != 0) + strlen(baseNotation);
    result += HandlePreAlignment(formatInfo, 
                                 dataLength, 
                                 prefix, 
                                 baseNotation, 
                                 putchar);

    /* Print the number itself. */
    result += PrintInteger(integer,
                           bSigned,
                           base,
                           capital ?
                               "0123456789ABCDEF" :
                               "0123456789abcdef", 
                           putchar);

    result += HandlePostAlignment(formatInfo,
                                  dataLength, 
                                  putchar);
    return result;
}

static
int
HandleStrFormat(PFORMAT_INFO formatInfo,
                va_list* pArgs,
                PutcharCtx* putchar)
{
    int result = 0;
    size_t length;
    const char* Str = va_arg(*pArgs, const char*);
    length = strlen(Str);

    result += HandlePreAlignment(formatInfo, length, 0, NULL, putchar);
    while (*Str)
    {
        result += putchar->Function(putchar, *Str++);
    }
    result += HandlePostAlignment(formatInfo, length, putchar);

    return result;
}

static
int
HandleCharFormat(PFORMAT_INFO formatInfo,
                 va_list* pArgs,
                 PutcharCtx* putchar)
{
    char c = (char)va_arg(*pArgs, int);
    int result = 0;

    result += HandlePreAlignment(formatInfo, 1, 0, NULL, putchar);
    result += putchar->Function(putchar, c);
    result += HandlePostAlignment(formatInfo, 1, putchar);

    return true;
}

static
int
HandleFormat(PFORMAT_INFO formatInfo,
             va_list* pArgs,
             PutcharCtx* putchar)
{
    switch (formatInfo->Type)
    {
    case 'i':
    case 'd':
    case 'x':
    case 'X':
    case 'u':
    case 'p':
    case 'o':
        return HandleIntFormat(formatInfo, pArgs, putchar);
    case 's':
        return HandleStrFormat(formatInfo, pArgs, putchar);
    case 'c':
        return HandleCharFormat(formatInfo, pArgs, putchar);
    case '?':
    case '%':
        return putchar->Function(putchar, formatInfo->Type);
    default:
        assert(false);
    }

    return 0;
}

int vxprintf(const char* format, 
             va_list args, 
             PutcharCtx* putchar)
{
    int result = 0;
    FORMAT_INFO formatInfo;

    while (format && *format)
    {
        if (*format == '%')
        {
            format++;
            if (ParseFormat(format, &format, &formatInfo))
            {
                result += HandleFormat(&formatInfo, &args, putchar);
            }
        }
        else
        {
            putchar->Function(putchar, *format);
            result++;
            format++;
        }
    }

    return result;
}

int sprintf(char* buffer, const char* format, ...)
{
    int result;

    va_list list;
    va_start(list, format);

    result = vsprintf(buffer, format, list);

    va_end(list);
    return result;
}

int snprintf(char* buffer, size_t size, const char* format, ...)
{
    int result;

    va_list list;
    va_start(list, format);

    result = vsnprintf(buffer, size, format, list);

    va_end(list);
    return result;
}

int printf(const char* format, ...)
{
    int result;

    va_list list;
    va_start(list, format);

    result = vprintf(format, list);

    va_end(list);
    return result;
}

struct PutcharSprintf
{
    struct PutcharCtx;
    char* Buffer;
    size_t Index;
    size_t Size;
};

int StrBufferPutchar(PutcharCtx* putchar, char c)
{
    struct PutcharSprintf* putcharSprintf = (struct PutcharSprintf*)putchar;

    if (putcharSprintf->Index < putcharSprintf->Size)
    {
        putcharSprintf->Buffer[putcharSprintf->Index++] = c;
        return 1;
    }

    return 0;
}

int vsprintf(char* buffer, const char* format, va_list args)
{
    struct PutcharSprintf context;
    context.Buffer = buffer;
    context.Index = 0;
    context.Size = __SIZE_MAX__;
    context.Function = StrBufferPutchar;

    return vxprintf(format, args, (PutcharCtx*)&context);
}

int vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
    struct PutcharSprintf context;
    context.Buffer = buffer;
    context.Index = 0;
    context.Size = size;
    context.Function = StrBufferPutchar;

    return vxprintf(format, args, (PutcharCtx*)&context);
}

#define CONOUT_BUFFER_SIZE 128
struct PutcharBufferedConLog
{
    struct PutcharCtx;
    char Buffer[CONOUT_BUFFER_SIZE];
    size_t Index;
};

static 
void 
Flush(struct PutcharBufferedConLog* putchar)
{
    Log(putchar->Buffer);

    memset(putchar->Buffer, 0, CONOUT_BUFFER_SIZE);
    putchar->Index = 0;
}

int BufferedConLogPutchar(PutcharCtx* putchar, char c)
{
    struct PutcharBufferedConLog* putcharConLog = (struct PutcharBufferedConLog*)putchar;

    if (putcharConLog->Index + 1 < CONOUT_BUFFER_SIZE &&
        c != '\n')
    {
        putcharConLog->Buffer[putcharConLog->Index++] = c;
    }
    else if (c == '\n')
    {
        Flush(putcharConLog);
    }
    else 
    {
        Flush(putcharConLog);
        putcharConLog->Buffer[putcharConLog->Index++] = c;
    }
    
    return 1;
}

struct PutcharBufferedConLog putchar =
{
    .Index = 0,
    .Buffer = { 0 },
    .Function = BufferedConLogPutchar
};

int vprintf(const char* format, va_list args)
{
    return vxprintf(format, args, (PutcharCtx*)&putchar);
}