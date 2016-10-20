#include <SystemLoader/SystemLoader.h>
#include <Kernel/CXKProcessorState.h>

#if kCXBuildDev

#include <SystemLoader/SLLibrary.h>

static SLSerialPort gSLSerialPort0;

void SLLoaderSerial0OutputUTF8(UInt8 character)
{
    if (character == '\b')
    {
        SLSerialWriteString(gSLSerialPort0, "\b \b");
        return;
    }

    SLSerialWriteCharacter(gSLSerialPort0, character, true);
}

void SLLoaderSetupSerial(void)
{
    OSAddress portAddress = (OSAddress)0x03F8;
    SLSerialPort port = SLSerialPortInit((OSAddress)portAddress);

    if (port == 0xFFFF)
    {
        SLPrintError("Error Loading Serial Port! (Tried port %p)\n", portAddress);
        gSLSerialPort0 = 0;

        return;
    }

    SLSerialPortSetupLineControl(port, 3, 0, 0);
    SLSerialPortSetBaudDivisor(port, 2);

    gSLSerialPort0 = port;
    bool registered = SLRegisterOutputFunction(&SLLoaderSerial0OutputUTF8);

    if (!registered)
    {
        SLPrintError("Error Registering Serial Port 0 (at %p) to receive output!\n", portAddress);
        return;
    }

    if (false) SLSetBootScreenOutput(false);
}

#endif /* kCXBuildDev */

SLStatus CXSystemLoaderMain(OSAddress imageHandle, SLSystemTable *systemTable)
{
    #if kCXBuildDev
        SLPrintError(kSLLoaderWelcomeString);
        SLLoaderSetupSerial();

        if (gSLSerialPort0)
        {
            SLSetBootScreenOutput(false);
            SLPrintString(kSLLoaderWelcomeString);
            SLSetBootScreenOutput(true);
        }

        bool runRequested = SLPromptUser("Run Tests", gSLSerialPort0);

        if (runRequested)
        {
            SLShowDelay("Running", 3);
            SLRunTests();
        }

        SLPrintString("Starting Loader. CPU States:\n");
        SLDumpProcessorState(false, false, false);

        SLWaitForKeyPress();
    #endif /* kCXBuildDev */

    return kSLStatusSuccess;
}
