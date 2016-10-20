#include <SystemLoader/SystemLoader.h>
#include <SystemLoader/SLSerial.h>
#include <Kernel/CXKMemory.h>

typedef struct {
    SLConfigInteger port;
    SLConfigInteger baudDivisor;
    SLConfigInteger wordLength;
    SLConfigInteger stopBits;
    SLConfigInteger parity;
} SLConfigPlaneSerial;

typedef struct {
    SLConfigInteger postCode;
    bool enableRelocaction;
    SLConfigInteger loadAddress;
} SLConfigPlaneKernelDev;

typedef struct {
    bool enableScreenConsole;
    bool enableSerialConsole;
} SLConfigPlaneConsoleDev;

typedef struct {
    SLConfigString bootFile;
} SLConfigPlaneLoaderDev;

typedef struct {
    SLConfigPlaneLoaderDev  loader;
    SLConfigPlaneConsoleDev console;
    SLConfigPlaneKernelDev  kernel;
    SLConfigPlaneSerial     serial;
} SLConfigPlaneDev;

typedef struct {
    //
} SLConfigPlaneKernel;

typedef struct {
    //
} SLConfigPlaneLoader;

struct __SLConfigFile {
    SLConfigPlaneLoader loader;
    SLConfigPlaneKernel kernel;
    SLConfigPlaneDev    dev;
};

SLConfigFile gSLMainConfig;

SLConfigFile *SLConfigFileLoad(SLString path)
{
    #if kCXBuildDev
        gSLMainConfig.dev.loader.bootFile = "/boot.car";

        gSLMainConfig.dev.console.enableScreenConsole = false;
        gSLMainConfig.dev.console.enableSerialConsole = true;

        gSLMainConfig.dev.kernel.postCode = 0xCC;
        gSLMainConfig.dev.kernel.enableRelocaction = true;
        gSLMainConfig.dev.kernel.loadAddress = 0xFFFFFFFFFFFFFFFF;

        gSLMainConfig.dev.serial.port = 0x03F8;
        gSLMainConfig.dev.serial.baudDivisor = 2;
        gSLMainConfig.dev.serial.wordLength = kSLSerialWordLength8Bits;
        gSLMainConfig.dev.serial.stopBits = kSLSerial1StopBit;
        gSLMainConfig.dev.serial.parity = kSLSerialNoParity;
    #endif /* kCXBuildDev */

    return SLConfigGet();
}

SLConfigFile *SLConfigGet(void)
{
    return &gSLMainConfig;
}

void SLConfigSet(SLConfigFile *config)
{
    CXKMemoryCopy(config, &gSLMainConfig, sizeof(SLConfigFile));
}

bool SLSimpleStringCompare(const char *restrict s1, const char *restrict s2)
{
    for ( ; (*s2) && (*s1) == (*s2); s1++, s2++);
    return !((*s1) ^ (*s2));
}

SLConfigString SLConfigGetString(SLConfigPath path, SLStringEncoding encoding)
{
    SLConfigString string = kOSNullPointer;

    if (kCXBuildDev)
    {
        if (SLSimpleStringCompare(path, "dev.loader.bootFile"))
            string = gSLMainConfig.dev.loader.bootFile;
    }

    if (!string) return kOSNullPointer;

    if (encoding == kSLStringEncodingUTF8) {
        return string;
    } else if (encoding == kSLStringEncodingUTF16) {
        // Need to convert from UTF-8
        return kOSNullPointer;
    } else {
        return kOSNullPointer;
    }
}

SLConfigInteger SLConfigGetInteger(SLConfigPath path)
{
    if (kCXBuildDev)
    {
        if (SLSimpleStringCompare(path, "dev.kernel.postCode"))
            return gSLMainConfig.dev.kernel.postCode;

        if (SLSimpleStringCompare(path, "dev.kernel.loadAddress"))
            return gSLMainConfig.dev.kernel.loadAddress;

        if (SLSimpleStringCompare(path, "dev.serial.port"))
            return gSLMainConfig.dev.serial.port;

        if (SLSimpleStringCompare(path, "dev.serial.baudDivisor"))
            return gSLMainConfig.dev.serial.baudDivisor;

        if (SLSimpleStringCompare(path, "dev.serial.wordLength"))
            return gSLMainConfig.dev.serial.wordLength;

        if (SLSimpleStringCompare(path, "dev.serial.stopBits"))
            return gSLMainConfig.dev.serial.stopBits;

        if (SLSimpleStringCompare(path, "dev.serial.parity"))
            return gSLMainConfig.dev.serial.parity;
    }

    return 0;
}

bool SLConfigGetBoolean(SLConfigPath path)
{
    if (kCXBuildDev)
    {
        if (SLSimpleStringCompare(path, "dev.console.enableScreenConsole"))
            return gSLMainConfig.dev.console.enableScreenConsole;

        if (SLSimpleStringCompare(path, "dev.console.enableSerialConsole"))
            return gSLMainConfig.dev.console.enableSerialConsole;

        if (SLSimpleStringCompare(path, "dev.kernel.enableRelocaction"))
            return gSLMainConfig.dev.kernel.enableRelocaction;
    }

    return false;
}
