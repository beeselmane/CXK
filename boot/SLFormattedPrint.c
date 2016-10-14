#include <SystemLoader/SystemLoader.h>

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

//
