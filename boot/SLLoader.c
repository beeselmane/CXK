#include <SystemLoader/SystemLoader.h>
#include <Kernel/CXKProcessorState.h>

#if kCXBuildDev

#include <SystemLoader/SLLibrary.h>

static SLSerialPort gSLSerialPort0;

void SLLoaderSerial0OutputUTF8(UInt8 character)
{
    if (character == '\b')
    {
        SLSerialWriteString(gSLSerialPort0, (const char *)"\b \b");
        return;
    }

    SLSerialWriteCharacter(gSLSerialPort0, character, true);
}

void SLLoaderSetupSerial(void)
{
    OSAddress portAddress = 0x03F8;
    SLSerialPort port = SLSerialPortInit(portAddress);

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
}

#endif /* kCXBuildDev */

SLStatus CXSystemLoaderMain(OSAddress imageHandle, SLSystemTable *systemTable)
{
    #if kCXBuildDev
        SLPrintError(kSLLoaderWelcomeString);
        SLLoaderSetupSerial();

        if (gSLSerialPort0)
            SLPrintString(kSLLoaderWelcomeString);

        #if kCXDebug
            __SLLibraryInitialize();
        #endif /* kCXDebug */

        bool runRequested = SLPromptUser("Run Tests", gSLSerialPort0);

        if (runRequested)
        {
            SLShowDelay("Running", 2);
            SLRunTests();
        }

        SLPrintString("Starting Loader. CPU States:\n");
        SLDumpProcessorState(true, true, true);

        SLWaitForKeyPress();
    #endif /* kCXBuildDev */

    return kSLStatusSuccess;
}
