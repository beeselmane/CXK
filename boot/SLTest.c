#include <SystemLoader/SystemLoader.h>
#include <SystemLoader/SLLibrary.h>

#if kCXBuildDev

void SLTestFormattedPrint(void)
{
    SLPrintString("0x00324BCA\n0x%08lX\n",           0x00324BCA);
    SLPrintString("07654321\n%08o\n",                07654321);
    SLPrintString("0x0abc\n0x%04hx\n",               0x0abc);
    SLPrintString("0x0F\n0x%02hhX\n",                0x0F);
    SLPrintString("0\n%hhu\n",                       0);
    SLPrintString("-12\n%hd\n",                      -12);

    SLDelayProcessor(1000000, true);

    SLPrintString("011001\n%06b\n",                  0b011001);
    SLPrintString("c\n%hhc\n",                       'c');
    SLPrintString(" \n%0d\n",                        0);
    SLPrintString("6547898\n%d\n",                   6547898);
    SLPrintString("57\n%zu\n",                       (UInt64)57);
    SLPrintString("0x0BCDEF9876543210\n0x%016llX\n", 0x0BCDEF9876543210);

    SLDelayProcessor(1000000, true);

    SLPrintString("UTF-16 Test\n%S\n",               u"UTF-16 Test");
    SLPrintString("UTF-8  Test\n%s\n",                "UTF-8  Test");
    SLPrintString("%%\n%%\n");
    SLPrintString("\n\n");
}

void SLRunTests(void)
{
    SLPrintString("Running Formatted Print Test...\n\n");
    SLTestFormattedPrint();
}

#endif /* kCXBuildDev */
