#include <SystemLoader/SystemLoader.h>
#include <System/OSCompilerMacros.h>
#include <SystemLoader/SLLibrary.h>

typedef UInt32 SLUnicodePoint;
typedef UInt16 SLUTF16Char;
typedef UInt8 SLUTF8Char;

static const SLUnicodePoint kSLSurrogateHighBegin  = 0xD800;
static const SLUnicodePoint kSLSurrogateHighFinish = 0xDBFF;
static const SLUnicodePoint kSLSurrogateLowBegin   = 0xDC00;
static const SLUnicodePoint kSLSurrogateLowFinish  = 0xDFFF;
static const SLUnicodePoint kSLErrorCodePoint = 0xFFFFFFFF;

static const UInt8 kSLUTF8ExtraByteCount[0x100] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

static const UInt64 kSLUTF8Excess[6] = {
    0x0000000000,
    0x0000003040,
    0x00000E2080,
    0x00E2041040,
    0x00FA082080,
    0x3F82082080
};

static const SLUTF8Char kSLUTF8BitMasks[7] = {
    0,
    0b00000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100
};

OSPrivate OSCount SLUTF8FromCodePoint(SLUnicodePoint point, SLUTF8Char *output, OSCount count)
{
    #define next(o, b, p)           \
        b[o] = (p | 0x80) & 0xBF;   \
        p >>= 6;                    \

    #define finish(o, p, c) o[0] = p | kSLUTF8BitMasks[c]

    #define entry(b, c, p, o, entries)  \
        do {                            \
            if (c < b) return c;        \
                                        \
            entries                     \
            finish(o, p, b);            \
                                        \
            return (b - 1);             \
        } while (0)

    if (point < 0x80) {
        (*output) = (SLUTF8Char)point;
        return 0;
    } else if (point < 0x800) {
        entry(2, count, point, output, {
            next(1, output, point);
        });
    } else if (point < 0x10000) {
        entry(3, count, point, output, {
            next(2, output, point);
            next(1, output, point);
        });
    } else if (point < 0x200000) {
        entry(4, count, point, output, {
            next(3, output, point);
            next(2, output, point);
            next(1, output, point);
        });
    } else {
        entry(5, count, point, output, {
            next(4, output, point);
            next(3, output, point);
            next(2, output, point);
            next(1, output, point);
        });
    }

    #undef entry
    #undef finish
    #undef next
}

OSPrivate SLUnicodePoint SLUTF8ToCodePoint(SLUTF8Char *input, OSCount inCount, OSCount *usedCount)
{
    OSCount count = kSLUTF8ExtraByteCount[*input];
    SLUnicodePoint point = 0;

    if (inCount < count)
    {
        (*usedCount) = inCount;
        return kSLErrorCodePoint;
    }

    switch (count)
    {
        case 5:
            point += (*input++);
            point <<= 6;
        case 4:
            point += (*input++);
            point <<= 6;
        case 3:
            point += (*input++);
            point <<= 6;
        case 2:
            point += (*input++);
            point <<= 6;
        case 1:
            point += (*input++);
            point <<= 6;
        case 0:
            point += (*input++);
    }

    point -= kSLUTF8Excess[count];
    (*usedCount) = (count);

    return point;
}

OSPrivate OSCount SLUTF16FromCodePoint(SLUnicodePoint point, SLUTF16Char *output, OSCount count)
{
    if (point < 0x10000) {
        (*output) = point;
    } else if (count) {
        point -= 0x10000;

        output[0] = (point >> 10)    + kSLSurrogateHighBegin;
        output[1] = (point & 0x3FFF) + kSLSurrogateLowBegin;

        return 1;
    }

    return 0;
}

OSPrivate SLUnicodePoint SLUTF16ToCodePoint(SLUTF16Char *input, OSCount inCount, OSCount *usedCount)
{
    SLUTF16Char first = (*input++);
    (*usedCount) = 0;

    if (kSLSurrogateHighBegin <= first && first <= kSLSurrogateHighFinish) {
        if (!inCount) return kSLErrorCodePoint;

        SLUTF16Char second = (*input);
        (*usedCount) = 1;

        if (second <= kSLSurrogateLowBegin || kSLSurrogateLowFinish <= second)
            return kSLErrorCodePoint;

        return ((SLUnicodePoint)((first << 10) | second) + 0x10000);
    } else if (kSLSurrogateLowBegin <= first && first <= kSLSurrogateLowFinish) {
        return kSLErrorCodePoint;
    } else {
        return ((SLUnicodePoint)first);
    }
}

#if kCXBuildDev
    void SLSetBootScreenOutput(bool enabled)
    {
        gSLEnableScreenPrint = enabled;
    }

    bool SLRegisterOutputFunction(SLOutputUTF8 function)
    {
        for (UInt8 i = 0; i < 8; i++)
        {
            if (!gSLOutputFunctions[i])
            {
                gSLOutputFunctions[i] = function;
                return true;
            }
        }

        return false;
    }

    void SLDeleteCharacters(OSCount count)
    {
        while (count)
        {
            SLPrintString("\b");
            count--;
        }
    }
#endif /* kCXBuildDev */

SLOutputUTF8 gSLOutputFunctions[8] = {
    kOSNullPointer, kOSNullPointer,
    kOSNullPointer, kOSNullPointer,
    kOSNullPointer, kOSNullPointer,
    kOSNullPointer, kOSNullPointer
};

static OSLength SLSimpleStringLengthUTF8(SLUTF8Char *string)
{
    OSLength length = 0;

    while (*string)
    {
        string++;
        length++;
    }

    return length;
}

static OSLength SLSimpleStringLengthUTF16(SLUTF16Char *string)
{
    OSLength length = 0;

    while (*string)
    {
        string++;
        length++;
    }

    return length;
}

extern void SLLoaderSerial0OutputUTF8(UInt8 character);

OSInline void SLPrintSingle(SLUTF8Char utf8[7], SLUTF16Char utf16[3], bool enableUTF16)
{
    for (UInt8 i = 0; i < 8; i++)
    {
        if (gSLOutputFunctions[i]) {
            UInt8 n = 0;

            while (utf8[n])
            {
                #if kCXBuildDev
                    SLLoaderSerial0OutputUTF8(utf8[n]);
                #else
                    gSLOutputFunctions[i](utf8[n]);
                #endif

                n++;
            }
        } else {
            break;
        }
    }

    if (enableUTF16 && utf16[0])
    {
        if (utf16[0] == '\n' && !utf16[1])
        {
            utf16[0] = '\r';
            utf16[1] = '\n';
            utf16[2] = 0;
        }

        SLSimpleTextOutput *stdout = gSLLoaderSystemTable->stdout;
        stdout->printUTF16(stdout, utf16);
    }
}

#define SLLoadSingle(a)         \
    do {                        \
        utf8[0] = a;            \
        utf8[1] = 0;            \
                                \
        if (enableUTF16)        \
        {                       \
            utf16[0] = a;       \
            utf16[1] = 0;       \
        }                       \
    } while (0)

#define SLLoadDouble(a, b)      \
    do {                        \
        utf8[0] = a;            \
        utf8[1] = b;            \
        utf8[2] = 0;            \
                                \
        if (enableUTF16)        \
        {                       \
            utf16[0] = a;       \
            utf16[1] = b;       \
            utf16[2] = 0;       \
        }                       \
    } while (0)

#define kSLPrintSingleArgs      utf8, utf16, enableUTF16

OSInline void SLPrintNumber(UInt64 number, UInt8 base, UInt8 hexBegin, UInt8 padding, bool enableUTF16)
{
    SLUTF8Char  buffer[32];
    SLUTF16Char utf16[3];
    SLUTF8Char  utf8[7];
    SInt8 i = 0;

    if (!number)
    {
        if (padding)
        {
            for ( ; i < padding; i++)
            {
                SLLoadSingle('0');
                SLPrintSingle(kSLPrintSingleArgs);
            }
        }

        return;
    }

    do {
        UInt8 next = number % base;
        number /= base;

        if (next < 10) {
            buffer[i++] = next + '0';
        } else if (next < 37) {
            buffer[i++] = (next - 10) + hexBegin;
        } else {
            buffer[i++] = '#';
        }
    } while (number);

    if (i < padding)
    {
        UInt8 zeroCount = padding - i;

        for (UInt8 j = 0; j < zeroCount; j++)
        {
            SLLoadSingle('0');
            SLPrintSingle(kSLPrintSingleArgs);
        }
    }

    for (i--; i >= 0; i--)
    {
        if (i) {
            SLLoadDouble(buffer[i], buffer[i - 1]);
            SLPrintString(kSLPrintSingleArgs);

            i--;
        } else {
            SLLoadSingle(buffer[i]);
            SLPrintSingle(kSLPrintSingleArgs);
        }
    }
}

#if !kCXBuildDev
    #undef SLPrintString
#endif /* !kCXBuildDev */

OSPrivate void SLPrintString(const char *s, ...)
{
    #if kCXBuildDev
        bool enableUTF16 = (gSLBootServicesEnabled && gSLEnableScreenPrint);
    #else /* !kCXBuildDev */
        bool enableUTF16 = (gSLBootServicesEnabled);
    #endif /* kCXBuildDev */

    bool justEnteredEscapeCode = false;
    bool inEscapeCode = false;
    bool printNumber = false;
    bool printSign = false;
    bool inPadding = false;

    UInt8 hexBase = 'A';
    UInt8 lastFlag = 0;
    UInt8 argSize = 4;
    UInt8 padding = 1;
    UInt8 base = 10;

    SLUTF16Char utf16[3];
    SLUTF8Char utf8[7];

    OSVAList args;
    OSVAStart(args, s);

    while (*s)
    {
        UInt8 c = *s++;

        if (!inEscapeCode && c != '%')
        {
            SLLoadSingle(c);
            SLPrintSingle(kSLPrintSingleArgs);

            continue;
        }

        if (!inEscapeCode)
        {
            justEnteredEscapeCode = true;
            inEscapeCode = true;

            continue;
        }

        if (justEnteredEscapeCode)
        {
            justEnteredEscapeCode = false;

            if (c == '%')
            {
                SLLoadSingle(c);
                SLPrintSingle(kSLPrintSingleArgs);
                inEscapeCode = false;

                continue;
            }
        }

        // Yes, I realize that this will allow an
        // infinite number of size flags to be
        // used, and that the last size flag given
        // will take precedence over all others.
        // This doesn't matter, however, it doesn't
        // actually expose anything that could be
        // exploited.
        switch (c)
        {
            case '0': {
                inPadding = true;
                padding = 0;
            } break;
            case 'h': {
                if (lastFlag == c) {
                    argSize = 1;
                } else {
                    lastFlag = c;
                    argSize = 2;
                }
            } break;
            case 'l': {
                if (lastFlag == c) {
                    argSize = 8;
                } else {
                    lastFlag = c;
                    argSize = 4;
                }
            } break;
            case 'z': {
                argSize = 8;
            } break;
            // Extension to print binary
            // numbers of length n
            case 'b': {
                inEscapeCode = false;
                printNumber = true;
                base = 2;
            } break;
            case 'o': {
                inEscapeCode = false;
                printNumber = true;
                base = 8;
            } break;
            case 'u': {
                inEscapeCode = false;
                printNumber = true;
                printSign = false;
                base = 10;
            } break;
            case 'd': {
                inEscapeCode = false;
                printNumber = true;
                printSign = true;
                base = 10;
            } break;
            case 'x': {
                inEscapeCode = false;
                printNumber = true;
                hexBase = 'a';
                base = 16;
            } break;
            case 'X': {
                inEscapeCode = false;
                printNumber = true;
                hexBase = 'A';
                base = 16;
            } break;
            case 'p': {
                SLLoadDouble('0', 'x');
                SLPrintSingle(kSLPrintSingleArgs);

                inEscapeCode = false;
                printNumber = true;
                hexBase = 'A';
                argSize = 8;
                base = 16;
            }
            case 'c': {
                inEscapeCode = false;
                UInt8 character = OSVAGetNext(args, UInt32);

                SLLoadSingle(character);
                SLPrintSingle(kSLPrintSingleArgs);
            } break;
            case 's': {
                inEscapeCode = false;
                SLUTF8Char *utf8string = OSVAGetNext(args, SLUTF8Char *);
                OSCount charsLeft = SLSimpleStringLengthUTF8(utf8string);
                OSCount used, used16;
                UInt8 i;

                while (charsLeft)
                {
                    if (enableUTF16) {
                        SLUnicodePoint point = SLUTF8ToCodePoint(utf8string, charsLeft, &used);
                        if (point == kSLErrorCodePoint) break;
                        used++;

                        used16 = SLUTF16FromCodePoint(point, utf16, 1);
                        utf16[used16 + 1] = 0;

                        //CXKMemoryCopy(utf8string, utf8, used);
                        for (i = 0; i < used; i++)
                            utf8[i] = utf8string[i];

                        utf8string += used;
                        charsLeft -= used;
                        utf8[used] = 0;
                    } else {
                        for (i = 0; i <= charsLeft && i < 7; i++)
                            utf8[i] = (*utf8string++);

                        charsLeft -= (i - 1);
                        utf8[i] = 0;
                    }

                    SLPrintSingle(kSLPrintSingleArgs);
                }
            } break;
            case 'S': {
                inEscapeCode = false;
                SLUTF16Char *utf16string = OSVAGetNext(args, SLUTF16Char *);
                OSCount charsLeft = SLSimpleStringLengthUTF16(utf16string);
                OSCount used, used8;

                while (charsLeft)
                {
                    SLUnicodePoint point = SLUTF16ToCodePoint(utf16string, charsLeft, &used);
                    if (point == kSLErrorCodePoint) break;
                    used++;

                    used8 = SLUTF8FromCodePoint(point, utf8, 6);
                    utf8[used8 + 1] = 0;

                    for (UInt8 i = 0; i < used; i++)
                        utf16[i] = utf16string[i];

                    utf16string += used;
                    charsLeft -= used;
                    utf16[used] = 0;

                    SLPrintSingle(kSLPrintSingleArgs);
                }
            } break;
        }

        if (inPadding)
        {
            if ('0' <= c && c <= '9') {
                padding *= 10;
                padding += (c - '0');
            } else {
                inPadding = false;
            }
        }

        if (printNumber)
        {
            UInt64 number;

            if (!printSign) {
                switch (argSize)
                {
                    case 1:
                    case 2:
                    case 4:
                        number = OSVAGetNext(args, UInt32);
                    break;
                    case 8:
                        number = OSVAGetNext(args, UInt64);
                    break;
                }
            } else {
                SInt64 signedValue;

                switch (argSize)
                {
                    case 1:
                    case 2:
                    case 4:
                        signedValue = OSVAGetNext(args, SInt32);
                    break;
                    case 8:
                        signedValue = OSVAGetNext(args, SInt64);
                    break;
                }

                if (signedValue < 0)
                {
                    signedValue *= -1;

                    SLLoadSingle('-');
                    SLPrintSingle(kSLPrintSingleArgs);
                }

                printSign = false;
                number = signedValue;
            }

            SLPrintNumber(number, base, hexBase, padding, enableUTF16);
            printNumber = false;
            padding = 1;
        }
    }

    OSVAFinish(args);
}

void SLPrintError(const char *s, ...) OSAlias(SLPrintString);
