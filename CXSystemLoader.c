#include <SystemLoader/SystemLoader.h>

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

    #if kCXDebug || kCXDevelopment

    SLPrintString(kSLLoaderWelcomeString);
    SLLibraryCheckStatus(kSLSuccess);

    SLLoadedImage *loadedImage = SLGetLoadedImage(SLLibraryGetMainImageHandle());

    if (loadedImage)
    {
        SLString path = SLGetLoadedImagePath(loadedImage);
        if (path) SLPrintString(SLS("Loader Path: %s\r\n"), path);
    }

    SLSetupSerial0(&config->devOptions.serial);
    SLPrintString(SLS("Press Any Key to Continue (or q to quit)..."));
    SLWaitForKeyPress(true);
    SLPrintString(SLS("\r\n\r\n"));

    #endif /* debug || development */

    SLFileHandle dataDirectory = SLOpenPath("\\" kSLLoaderDataDirectory);
    if (!dataDirectory) goto fail;

    SLPrintString(SLS("Opened Data Directory '%s'\r\n"), dataDirectory);
    SLFileHandle setupFile = SLOpenChild(dataDirectory, config->setupFile);
    if (!setupFile) goto fail;

    SLPrintString(SLS("Opened Setup File '%s'\r\n"), config->setupFile);
    CXKBootOptions *kernelOptions = SLGetKernelInitialOptions(config);
    if (!kernelOptions) goto fail;

    SLStartSetup(setupFile, kernelOptions);

fail:
    if (kCXDebug || kCXDevelopment)
    {
        SLPrintString(SLS("Loading Failed! Press any key to exit...\r\n"));
        SLWaitForKeyPress(false);
    }

    return kSLLoadError;
}
