#include <SystemLoader/SystemLoader.h>

static SLImageHandle gSLMainImageHandle = kOSNullPointer;
static SLSystemTable *gSLSystemTable = kOSNullPointer;

#if kCXDebug || kCXDevelopment
    #define kSLInitialErrorCode       0x80
    #define kSLInitialInfoCode        0x00

    static CXKPOSTCode gSLErrorCode = kSLInitialErrorCode;
    static CXKPOSTCode gSLInfoCode  = kSLInitialInfoCode;
#endif /* debug || development */

SLEFIABI void SLLibrarySetSystemTable(SLSystemTable *systemTable)
{
    gSLSystemTable = systemTable;
}

SLEFIABI void SLLibrarySetMainImageHandle(SLImageHandle imageHandle)
{
    gSLMainImageHandle = imageHandle;
}

SLEFIABI SLSystemTable *SLLibraryGetSystemTable(void)
{
    return gSLSystemTable;
}

SLEFIABI SLImageHandle SLLibraryGetMainImageHandle(void)
{
    return gSLMainImageHandle;
}

SLEFIABI bool SLLibraryCheckStatus(SLStatus status)
{
    bool isError = SLStatusIsError(status);

    #if kCXDebug || kCXDevelopment
        CXKPOSTCode errorCode = gSLErrorCode++;
        CXKPOSTCode infoCode  = gSLInfoCode++;
        bool delay = false;

        if (isError) CXKSetPOSTCode(errorCode);
        else         CXKSetPOSTCode(infoCode);

        // Wrapped Back around
        if (gSLErrorCode <= kSLInitialInfoCode) {
            gSLErrorCode = kSLInitialErrorCode;
            delay = true;
        } if (gSLInfoCode >= kSLInitialErrorCode) {
            gSLInfoCode = kSLInitialInfoCode;
            delay = true;
        }

        if (delay) SLDelayProcessor(100);
    #endif /* debug || development */

    return isError;
}

SLEFIABI SLLoadedImage *SLGetLoadedImage(SLImageHandle imageHandle)
{
    SLProtocol loadedImageProtocol = kSLLoadedImageProtocol;
    SLLoadedImage *image = kOSNullPointer;

    SLStatus status = gSLSystemTable->bootServices->handleProtocol(imageHandle, &loadedImageProtocol, (void **)&image);
    bool failed = SLLibraryCheckStatus(status);

    return (failed ? kOSNullPointer : image);
}

SLEFIABI SLString SLGetLoadedImagePath(SLLoadedImage *image)
{
    return SLDevicePathToString(image->filePath, true, false);
}

SLEFIABI SLFileHandle SLGetRootDirectoryForImage(SLImageHandle imageHandle)
{
    SLProtocol volumeProtocol = kSLVolumeProtocol;
    SLVolumeHandle *volume = kOSNullPointer;
    SLStatus status;

    SLLoadedImage *image = SLGetLoadedImage(imageHandle);
    if (!image) return kOSNullPointer;

    status = gSLSystemTable->bootServices->handleProtocol(image->deviceHandle, &volumeProtocol, (void **)&volume);
    if (!SLLibraryCheckStatus(status)) return kOSNullPointer;

    SLFileHandle rootDirectory;
    status = volume->openRoot(volume, &rootDirectory);
    bool failed = SLLibraryCheckStatus(status);

    return (failed ? kOSNullPointer : rootDirectory);
}

SLEFIABI SLFileHandle SLOpenChild(SLFileHandle root, SLString child)
{
    SLFileHandle handle;
    SLStatus status = root->open(root, &handle, child, kSLFileModeRead, 0);
    bool failed = SLLibraryCheckStatus(status);

    return (failed ? kOSNullPointer : handle);
}

SLEFIABI SLFileHandle SLOpenPath(SLString path)
{
    if (!(path[0] == ((SLChar)'\\'))) return false;
    path++;

    SLFileHandle root = SLGetRootDirectoryForImage(gSLMainImageHandle);
    if (!root) return kOSNullPointer;

    SLFileHandle child = SLOpenChild(root, path);
    if (!child) return kOSNullPointer;

    return child;
}

SLEFIABI bool SLReadFile(SLFileHandle file, OSOffset offset, OSBuffer readBuffer)
{
    UInt64 currentOffset;
    SLStatus status = file->getOffset(file, (UIntN *)&currentOffset);
    if (!SLLibraryCheckStatus(status)) return false;

    if (currentOffset != offset)
    {
        status = file->setOffset(file, offset);
        if (!SLLibraryCheckStatus(status)) return false;
    }

    status = file->read(file, (UIntN *)&readBuffer.size, readBuffer.address);
    bool failed = SLLibraryCheckStatus(status);

    return !failed;
}

SLEFIABI OSBuffer SLReadFileFully(SLFileHandle file)
{
    SLGUID fileInfoID = kSLFileInfoID;
    OSBuffer buffer = kOSBufferEmpty;

    SLStatus status = file->getInfo(file, &fileInfoID, (UIntN *)&buffer.size, buffer.address);
    if (!SLLibraryCheckStatus(status)) return kOSBufferEmpty;

    OSSize bufferSize = buffer.size;
    buffer = SLAllocateBuffer(buffer.size);
    if (buffer.size != bufferSize) return kOSBufferEmpty;
    if (OSBufferIsEmpty(buffer)) return kOSBufferEmpty;

    status = file->read(file, (UIntN *)&buffer.size, buffer.address);
    if (!SLLibraryCheckStatus(status)) return kOSBufferEmpty;

    return buffer;
}

SLEFIABI bool SLReadPath(SLString path, OSOffset offset, OSBuffer readBuffer)
{
    SLFileHandle file = SLOpenPath(path);
    if (!file) return false;

    SLStatus status = file->read(file, (UIntN *)&readBuffer.size, readBuffer.address);
    bool failed = SLLibraryCheckStatus(status);

    return !failed;
}

SLEFIABI OSBuffer SLReadPathFully(SLString path)
{
    SLFileHandle file = SLOpenPath(path);
    if (!file) return kOSBufferEmpty;

    return SLReadFileFully(file);
}

SLEFIABI bool SLAllocateBootPagesAtAddress(OSAddress address, OSCount pages)
{
    UInt64 addr = (UInt64)address;
    SLStatus status = gSLSystemTable->bootServices->allocatePages(kSLAllocAtAddres, kSLMemoryTypeBootCode, pages, (unsigned long long int *)&addr);
    if (!SLLibraryCheckStatus(status)) return false;

    return (*((OSAddress *)addr) == address);
}

SLEFIABI OSBuffer SLAllocateBuffer(OSSize size)
{
    OSBuffer buffer = kOSBufferEmpty;
    buffer.size = size;

    SLStatus status = gSLSystemTable->bootServices->allocate(kSLMemoryTypeLoaderData, buffer.size, buffer.address);
    if (!SLLibraryCheckStatus(status)) return kOSBufferEmpty;

    return buffer;
}

SLEFIABI bool SLFreeBuffer(OSBuffer buffer)
{
    SLStatus status = gSLSystemTable->bootServices->free(buffer.address);
    bool failed = SLLibraryCheckStatus(status);

    return !failed;
}

SLEFIABI CXKMemoryMap SLGetCurrentMemoryMap(void)
{
    CXKMemoryMap memoryMap = {0, 0, kOSNullPointer};

    UInt32 descriptorVersion;
    UIntN descriptorSize;
    OSBuffer buffer;
    UIntN key;

    SLStatus status = gSLSystemTable->bootServices->getMemoryMap((UIntN *)&buffer.size, buffer.address, &key, &descriptorSize, &descriptorVersion);
    if (descriptorSize != sizeof(CXKMemoryMapEntry)) status = kSLWrongSize;
    if (descriptorVersion != 1) status = kSLIncompatibleVersion;
    if (!SLLibraryCheckStatus(status)) return memoryMap;

    buffer.size += descriptorSize;
    memoryMap.entryCount = (buffer.size / descriptorSize);
    buffer = SLAllocateBuffer(buffer.size);

    status = gSLSystemTable->bootServices->getMemoryMap((UIntN *)&buffer.size, (OSAddress)memoryMap.entries, (UIntN *)memoryMap.key, &descriptorSize, &descriptorVersion);
    if (descriptorSize != sizeof(CXKMemoryMapEntry)) status = kSLWrongSize;
    if (descriptorVersion != 1) status = kSLIncompatibleVersion;
    bool failed = SLLibraryCheckStatus(status);

    if (failed)
    {
        status = SLFreeBuffer(buffer);
        SLLibraryCheckStatus(status);

        memoryMap = ((CXKMemoryMap){0, 0, kOSNullPointer});
    }

    return memoryMap;
}

SLEFIABI CXKMemoryMap SLDeactivateBootServices(void)
{
    CXKMemoryMap memoryMap = SLGetCurrentMemoryMap();
    if (!memoryMap.key) return memoryMap;

    SLStatus status = gSLSystemTable->bootServices->disable(gSLMainImageHandle, memoryMap.key);
    if (!SLLibraryCheckStatus(status)) memoryMap.key = 0;

    return memoryMap;
}

SLEFIABI void SLDelayProcessor(UInt32 time)
{
    for (volatile int i = 0; i < (10000 * time); i++);
}

SLEFIABI bool SLWaitForKeyPress(bool enableExitKey)
{
    SLStatus status = gSLSystemTable->stdin->reset(gSLSystemTable->stdin, false);
    if (!SLLibraryCheckStatus(status)) return false;

    SLPressedKey key;
    while ((status = gSLSystemTable->stdin->readKey(gSLSystemTable->stdin, &key) == kSLNotReady));
    if (!SLLibraryCheckStatus(status)) return false;

    if (enableExitKey && key.character == 'q')
        gSLSystemTable->bootServices->exit(gSLMainImageHandle, 1, 0, kOSNullPointer);

        return true;
}

#if kCXDebug || kCXDevelopment

SLEFIABI CXKBasicSerialPort *SLInitSerial(UInt64 address)
{
    return CXKBasicSerialIntializeAtAddress((OSAddress)address);
}

SLEFIABI void SLPrintSerialString(CXKBasicSerialPort *port, UInt8 *string, OSLength length)
{
    for (UInt64 i = 0; i < length; i++)
        CXKBasicSerialWriteCharacter(port, *string, true);
}

#endif /* debug || development */
