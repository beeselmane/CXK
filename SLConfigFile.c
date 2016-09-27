#include <SystemLoader/SystemLoader.h>

static SLConfigOptions gSLConfigOptions = {
    .setupFile                  = kSLDefaultKernelLoader,
    .setupAddress               = (OSAddress)kSLDefaultSetupAddr,
    #if kCXDebug || kCXDevelopment
    .devOptions = {
        .serial = {
            .offset             = kSLDefaultSerialAddr,
            .baudDivisor        = kSLDefaultSerialBaudDiv,
            .wordLength         = kSLDefaultSerialWordLen,
            .stopBits           = kSLDefaultSerialStopBits,
            .parity             = kSLDefaultSerialParity
        },
        .setupCode              = kSLDefaultSetupPOSTCode,
        .kernelCode             = kSLDefaultKernelPOSTCode,
        .kernelAddress          = (OSAddress)kSLDefaultKernelAddr,
        .enableKernelRelocation = 1
    }
    #endif /* debug || development */
};

static bool gSLConfigOptionsLoaded = false;

SLEFIABI SLConfigOptions *SLGetConfigOptions(void)
{
    if (!gSLConfigOptionsLoaded)
    {
        /// TODO: Load Options from kSLLoaderConfigFile

        gSLConfigOptionsLoaded = true;
    }

    return &gSLConfigOptions;
}