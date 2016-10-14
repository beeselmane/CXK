#include <SystemLoader/SystemLoader.h>

#if 0
static CXKBootOptions *gSLKernelOptions = kOSNullPointer;

#if kCXDebug || kCXDevelopment

static CXKBasicSerialPort *gSLSerialPort0;

SLEFIABI void SLSetupSerial0(SLSerialOptions *options)
{
    gSLSerialPort0 = SLInitSerial(options->offset);

    if (gSLSerialPort0) {
        CXKBasicSerialSetupLineControl(gSLSerialPort0, options->wordLength, options->stopBits, options->parity);
        CXKBasicSerialSetBaudDivisor(gSLSerialPort0, options->baudDivisor);

        UInt8 startText[kSLLoaderStartTextSize] = kSLLoaderStartText;
        SLPrintSerialString(gSLSerialPort0, startText, kSLLoaderStartTextSize);
        SLPrintString(SLS("Loaded serial[0] at 0x%X\r\n"), options->offset);
    } else {
        SLPrintString(SLS("Error: No Serial Port Found!\r\n"));
        SLPrintString(SLS("Info: Searched at 0x%X\r\n"), options->offset);
    }
}

#endif /* kCXDebug || kCXDevelopment */

SLEFIABI CXKBootOptions *SLGetKernelInitialOptions(SLConfigOptions *config)
{
    if (!gSLKernelOptions)
    {
        gSLKernelOptions = SLAllocateBuffer(sizeof(CXKBootOptions)).address;
        if (!gSLKernelOptions) return kOSNullPointer;

        gSLKernelOptions->loaderBase = SLLibraryGetMainImageHandle();

        #if kCXDebug || kCXDevelopment
            gSLKernelOptions->devOptions.serialPort0      = gSLSerialPort0;
            gSLKernelOptions->devOptions.setupCode        = config->devOptions.setupCode;
            gSLKernelOptions->devOptions.kernelCode       = config->devOptions.kernelCode;
            gSLKernelOptions->devOptions.kernelAddress    = config->devOptions.kernelAddress;
            gSLKernelOptions->devOptions.enableRelocation = config->devOptions.enableKernelRelocation;
        #endif /* debug || development */
    }

    return gSLKernelOptions;
}

SLEFIABI SLStatus CXSystemLoaderMain(input SLImageHandle imageHandle, input SLSystemTable *systemTable)
{
    SLLibrarySetMainImageHandle(imageHandle);
    SLLibrarySetSystemTable(systemTable);

    SLConfigOptions *config = SLGetConfigOptions();
    SLLoadedImage *loadedImage = SLGetLoadedImage(SLLibraryGetMainImageHandle());

    #if kCXDebug || kCXDevelopment

    SLPrintString(kSLLoaderWelcomeString);
    SLLibraryCheckStatus(kSLSuccess);

    if (!loadedImage) goto fail;

    SLString path = SLGetLoadedImagePath(loadedImage);
    if (path) SLPrintString(SLS("Loader Path: %s\r\n"), path);

    SLSetupSerial0(&config->devOptions.serial);
    SLPrintString(SLS("Press Any Key to Continue (or q to quit)..."));
    SLWaitForKeyPress(true);
    SLPrintString(SLS("\r\n\r\n"));

    #endif /* debug || development */

    if (!loadedImage) goto fail;
    SLFileHandle root = SLLoadedImageGetRoot(loadedImage);
    if (!root) goto fail;

    SLFileHandle dataDirectory = SLOpenChild(root, kSLLoaderDataDirectory);
    if (!dataDirectory) goto fail;

    SLPrintString(SLS("Opened Data Directory '%s'\r\n"), kSLLoaderDataDirectory);
    SLFileHandle setupFile = SLOpenChild(dataDirectory, config->setupFile);
    if (!setupFile) goto fail;

    SLPrintString(SLS("Opened Setup File '%s'\r\n"), config->setupFile);
    CXKBootOptions *kernelOptions = SLGetKernelInitialOptions(config);
    if (!kernelOptions) goto fail;

    SLStartSetup(setupFile, kernelOptions);

    if (!systemTable->bootServices)
        return kSLLoadError;

fail:
    if (kCXDebug || kCXDevelopment)
    {
        SLPrintString(SLS("Loading Failed! Press any key to exit...\r\n"));
        SLWaitForKeyPress(false);
    }

    return kSLLoadError;
}

#endif

void SLPrint(SLSystemTable *systemTable, SLString string)
{
    systemTable->stdout->printUTF16(systemTable->stdout, string);
}

void SLSerialPrintNumber(SLSerialPort port, UInt64 number, UInt8 base)
{
    if (!number)
    {
        SLSerialWriteCharacter(port, '0', true);
        return;
    }

    UInt8 characters[32];
    SInt8 i = 0;

    do {
        UInt8 next = number % base;
        number /= base;

        if (next < 10) {
            characters[i++] = next + '0';
        } else if (next <= 36) {
            characters[i++] = (next + 'A') - 10;
        } else {
            characters[i++] = '#';
        }
    } while (number);

    for (i--; i >= 0; i--)
        SLSerialWriteCharacter(port, characters[i], true);
}

OSBuffer bfunc(OSBuffer buffer)
{
    buffer.address = 0;
    buffer.size = 0;
    return buffer;
}

void SLSRS(SLSerialPort port, UInt8 terminator, OSBuffer *buffer, bool print)
{
    if (terminator == 0x0A) terminator = 0x0D;

    UInt8 *string = (UInt8 *)buffer->address;
    OSSize size = buffer->size;

    while (size)
    {
        UInt8 character = SLSerialReadCharacter(port, true);
        SLSerialWriteCharacter(port, character, true);
        if (character == terminator) return;
        (*string++) = character;
        size--;
    }

    (*string) = 0;
}

extern OSAddress gSLFirmwareReturnAddress;
extern OSSize gSLLoaderImageSize;
extern OSAddress gSLLoaderBase;
extern OSAddress gSLLoaderEnd;

extern SLSystemTable *gSLLoaderSystemTable;
extern OSAddress gSLLoaderImageHandle;

void dumpInfo(SLSerialPort port)
{
    SLSerialWriteString(port, "Firmware Return Address: 0x");
    SLSerialPrintNumber(port, (UInt64)gSLFirmwareReturnAddress, 16);
    SLSerialWriteString(port, "\nLoader Base: 0x");
    SLSerialPrintNumber(port, (UInt64)&gSLLoaderBase, 16);
    SLSerialWriteString(port, "\nLoader End: 0x");
    SLSerialPrintNumber(port, (UInt64)&gSLLoaderEnd, 16);
    SLSerialWriteString(port, "\nLoader Size: ");
    SLSerialPrintNumber(port, (UInt64)gSLLoaderImageSize, 16);
    SLSerialWriteString(port, " (0x");
    SLSerialPrintNumber(port, (UInt64)gSLLoaderImageSize, 16);
    SLSerialWriteString(port, ")\nSystem Table: 0x");
    SLSerialPrintNumber(port, (UInt64)gSLLoaderSystemTable, 16);
    SLSerialWriteString(port, "\nImage Handle: 0x");
    SLSerialPrintNumber(port, (UInt64)gSLLoaderImageHandle, 16);
    SLSerialWriteString(port, "\nBoot Services: 0x");
    SLSerialPrintNumber(port, (UInt64)gSLLoaderSystemTable->bootServices, 16);
    SLSerialWriteString(port, "\n\n");
}

SLStatus CXSystemLoaderMain(OSAddress imageHandle, SLSystemTable *systemTable)
{
    SLSerialPort port = SLSerialPortInit((OSAddress)0x03F8);

    if (port == 0xFFFF)
    {
        SLPrint(systemTable, u"Error: Could not open Serial Port 0!\r\n");
        return kSLStatusSuccess;
    }

    SLSerialPortSetBaudDivisor(port, 2);
    SLSerialPortSetupLineControl(port, 3, 0, 0);
    SLSerialWriteString(port, kSLLoaderStartText);

    UInt8 cmdBuffer[256];
    OSBuffer buffer;
    buffer.address = cmdBuffer;
    buffer.size = 256;

    SLSerialReadString(port, '\n', &buffer, true);
    SLSerialWriteString(port, "\nRead: ");
    SLSerialWriteString(port, cmdBuffer);
    SLSerialWriteString(port, "\n\n");

    dumpInfo(port);

    return kSLStatusSuccess;
}
