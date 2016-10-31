#include <SystemLoader/SystemLoader.h>
#include <SystemLoader/SLLibrary.h>

OSAddress SLGetMainImageHandle(void)
{
    return gSLLoaderImageHandle;
}

SLBootServices *SLBootServicesGetCurrent(void)
{
    return gSLLoaderSystemTable->bootServices;
}

SLRuntimeServices *SLRuntimeServicesGetCurrent(void)
{
    return gSLLoaderSystemTable->runtimeServices;
}

SLSystemTable *SLSystemTableGetCurrent(void)
{
    return gSLLoaderSystemTable;
}

bool SLBootServicesHaveTerminated(void)
{
    return !gSLBootServicesEnabled;
}

bool SLCheckStatus(SLStatus status)
{
    return !SLStatusIsError(status);
}

bool SLBootServicesAllocatePages(OSAddress base, OSCount pages)
{
    OSAddress result = base;
    SLStatus status = gSLLoaderSystemTable->bootServices->allocatePages(kSLAllocTypeAtAddress, kSLMemoryTypeLoaderData, pages, &result);
    if (!SLCheckStatus(status)) return false;
    if (result != base) return false;
    return base;
}

OSBuffer SLBootServicesAllocateAnyPages(OSCount pages)
{
    OSAddress result;
    SLStatus status = gSLLoaderSystemTable->bootServices->allocatePages(kSLAllocTypeAnyPages, kSLMemoryTypeLoaderData, pages, &result);
    if (!SLCheckStatus(status)) return kOSBufferEmpty;

    return OSBufferMake(result, (pages * kSLBootPageSize));
}

OSBuffer SLBootServicesAllocate(OSSize size)
{
    OSAddress result;
    SLStatus status = gSLLoaderSystemTable->bootServices->allocate(kSLMemoryTypeLoaderData, size, &result);
    if (!SLCheckStatus(status)) return kOSBufferEmpty;
    return OSBufferMake(result, size);
}

bool SLBootServicesFree(OSBuffer buffer)
{
    SLStatus status = gSLLoaderSystemTable->bootServices->free(buffer.address);
    return !SLCheckStatus(status);
}

CXKMemoryMap *SLBootServicesGetMemoryMap(void)
{
    OSBuffer buffer = SLAllocate(sizeof(CXKMemoryMap));
    CXKMemoryMap *map = buffer.address;
    UIntN entrySize;
    UInt32 version;

    SLStatus status = gSLLoaderSystemTable->bootServices->getMemoryMap(&buffer.size, map->entries, &map->key, &entrySize, &version);
    if (entrySize != sizeof(CXKMemoryMapEntry)) status = kSLStatusWrongSize;
    if (status == kSLStatusBufferTooSmall) status = kSLStatusSuccess;
    if (version != 1) status = kSLStatusIncompatibleVersion;
    if (!SLCheckStatus(status)) goto failure;

    map->entryCount = (buffer.size / entrySize);
    buffer = SLAllocate(buffer.size);
    map->entries = buffer.address;

    status = gSLLoaderSystemTable->bootServices->getMemoryMap(&buffer.size, map->entries, &map->key, &entrySize, &version);
    if (entrySize != sizeof(CXKMemoryMapEntry)) status = kSLStatusWrongSize;
    if (version != 1) status = kSLStatusIncompatibleVersion;
    if (!SLCheckStatus(status)) goto failure;

    return map;

failure:
    if (map->entries) SLFree(map->entries);
    SLFree(map);

    return kOSNullPointer;
}

CXKMemoryMap *SLBootServicesTerminate(void)
{
    CXKMemoryMap *finalMemoryMap = SLBootServicesGetMemoryMap();
    if (!finalMemoryMap) return kOSNullPointer;

    SLStatus status = gSLLoaderSystemTable->bootServices->terminate(gSLLoaderImageHandle, finalMemoryMap->key);

    if (!SLCheckStatus(status)) {
        SLFree(finalMemoryMap);

        return kOSNullPointer;
    } else {
        gSLBootServicesEnabled = false;

        return finalMemoryMap;
    }
}

bool SLDelayProcessor(UIntN time, bool useBootServices)
{
    if (useBootServices) {
        SLStatus status = SLBootServicesGetCurrent()->stall(time);
        return SLCheckStatus(status);
    } else {
        // Try to mimic BS stall() function
        for (volatile UInt64 i = 0; i < (time * 100); i++);
        return true;
    }
}

char SLWaitForKeyPress(void)
{
    SLStatus status = gSLLoaderSystemTable->stdin->reset(gSLLoaderSystemTable->stdin, false);
    if (!SLCheckStatus(status)) return 0;
    status = kSLStatusNotReady;
    SLKeyPress key;

    while (status == kSLStatusNotReady)
        status = gSLLoaderSystemTable->stdin->readKey(gSLLoaderSystemTable->stdin, &key);

    return key.keycode;
}

bool SLBootServicesOutputString(SLString string)
{
    SLStatus status = gSLLoaderSystemTable->stdout->printUTF16(gSLLoaderSystemTable->stdout, string);
    return SLCheckStatus(status);
}

SLFile *SLGetRootDirectoryForImage(OSAddress imageHandle)
{
    SLLoadedImage *loadedImage = SLLoadedImageGetFromHandle(imageHandle);
    if (!loadedImage) return kOSNullPointer;

    return SLLoadedImageGetRoot(loadedImage);
}

SLFile *SLOpenChild(SLFile *parent, SLString child)
{
    SLFile *file;
    SLStatus status = parent->open(parent, &file, child, kSLFileModeRead, 0);
    bool failed = !SLCheckStatus(status);

    return (failed ? kOSNullPointer : file);
}

SLFile *SLOpenPath(SLString path)
{
    SLFile *root = SLGetRootDirectoryForImage(gSLLoaderImageHandle);
    if (!root) return kOSNullPointer;

    SLFile *child = SLOpenChild(root, path);
    if (!child) return kOSNullPointer;

    return child;
}

bool SLCloseFile(SLFile *file)
{
    SLStatus status = file->close(file);
    return !SLCheckStatus(status);
}

bool SLFileRead(SLFile *file, OSOffset offset, OSBuffer readBuffer)
{
    UInt64 currentOffset, size = readBuffer.size;
    SLStatus status = file->getOffset(file, &currentOffset);
    if (!SLCheckStatus(status)) return false;

    if (currentOffset != (UInt64)offset)
    {
        status = file->setOffset(file, offset);
        if (!SLCheckStatus(status)) return false;
    }

    status = file->read(file, &readBuffer.size, readBuffer.address);
    bool failed = (!SLCheckStatus(status)) || (readBuffer.size != size);

    return !failed;
}

OSBuffer SLFileReadFully(SLFile *file)
{
    OSUIDIntelData fileInfoUID = kSLFileInfoID;
    OSSize fileInfoSize = sizeof(SLFileInfo);
    SLFileInfo fileInfo;

    SLStatus status = file->getInfo(file, &fileInfoUID, &fileInfoSize, &fileInfo);
    if (status == kSLStatusBufferTooSmall) status = kSLStatusSuccess;
    if (!SLCheckStatus(status)) return kOSBufferEmpty;

    OSBuffer buffer = SLAllocate(fileInfo.size);
    if (OSBufferIsEmpty(buffer)) return buffer;
    status = file->read(file, &buffer.size, buffer.address);

    if (!SLCheckStatus(status))
    {
        buffer.size = fileInfo.size;
        SLBootServicesFree(buffer);

        return kOSBufferEmpty;
    }

    return buffer;
}

bool SLReadPath(SLString path, OSOffset offset, OSBuffer readBuffer)
{
    SLFile *file = SLOpenPath(path);
    if (!file) return false;

    bool success = SLFileRead(file, offset, readBuffer);
    SLStatus status = file->close(file);

    if (!SLCheckStatus(status))
        success = false;

    return success;
}

OSBuffer SLReadPathFully(SLString path)
{
    SLFile *file = SLOpenPath(path);
    if (!file) return kOSBufferEmpty;

    OSBuffer result = SLFileReadFully(path);
    SLStatus status = file->close(file);

    if (!SLCheckStatus(status))
    {
        if (!OSBufferIsEmpty(result))
            SLBootServicesFree(result);

        result = kOSBufferEmpty;
    }

    return result;
}

SLLoadedImage *SLLoadedImageGetFromHandle(OSAddress imageHandle)
{
    SLProtocol loadedImageProtocol = kSLLoadedImageProtocol;
    SLLoadedImage *image = kOSNullPointer;

    SLStatus status = gSLLoaderSystemTable->bootServices->handleProtocol(imageHandle, &loadedImageProtocol, &image);
    bool failed = !SLCheckStatus(status);

    return (failed ? kOSNullPointer : image);
}

SLFile *SLLoadedImageGetRoot(SLLoadedImage *image)
{
    SLProtocol volumeProtocol = kSLVolumeProtocol;
    SLVolume *volume = kOSNullPointer;

    SLStatus status = gSLLoaderSystemTable->bootServices->handleProtocol(image->deviceHandle, &volumeProtocol, &volume);
    if (!SLCheckStatus(status)) return kOSNullPointer;

    SLFile *root;
    status = volume->openRoot(volume, &root);
    bool failed = !SLCheckStatus(status);

    return (failed ? kOSNullPointer : root);
}

SLGraphicsOutput **SLGraphicsOutputGetAll(void)
{
    SLBootServices *bootServices = gSLLoaderSystemTable->bootServices;
    SLProtocol protocol = kSLGraphicsOutputProtocol;
    OSAddress *devices;
    UIntN count;

    SLStatus status = bootServices->localeHandles(kSLSearchTypeByProtocol, &protocol, kOSNullPointer, &count, &devices);
    if (!SLCheckStatus(status)) return kOSNullPointer;

    OSBuffer resultBuffer = SLAllocate((count + 1) * sizeof(SLGraphicsOutput *));
    SLGraphicsOutput **results = resultBuffer.address;
    results[count] = kOSNullPointer;
    OSCount i = 0;

    for ( ; i < count; i++)
    {
        SLGraphicsOutput *output;
        status = bootServices->handleProtocol(devices[i], &protocol, &output);
        results[i] = output;

        if (!SLCheckStatus(status)) goto failure;
    }

    status = bootServices->free(devices);
    if (!SLCheckStatus(status)) goto failure;
    return results;

failure:
    bootServices->free(devices);
    SLFree(results);

    return kOSNullPointer;
}

SLGraphicsModeInfo *SLGraphicsOutputGetMode(SLGraphicsOutput *graphics, UInt32 modeNumber)
{
    OSSize size = sizeof(SLGraphicsModeInfo *);
    SLGraphicsModeInfo *info;

    SLStatus status = graphics->getMode(graphics, modeNumber, &size, &info);
    return (SLCheckStatus(status) ? info : kOSNullPointer);
}

SLGraphicsMode *SLGraphicsOutputGetCurrentMode(SLGraphicsOutput *graphics)
{
    return graphics->mode;
}

SLGraphicsContext *SLGraphicsOutputGetContext(SLGraphicsOutput *graphics)
{
    return SLGraphicsOutputGetContextWithMaxSize(graphics, 0xFFFFFFFF, 0xFFFFFFFF);
}

SLGraphicsContext *SLGraphicsOutputGetContextWithMaxSize(SLGraphicsOutput *graphics, UInt32 maxHeight, UInt32 maxWidth)
{
    UInt32 modes = graphics->mode->numberOfModes;
    SLGraphicsModeInfo *maxMode = kOSNullPointer;
    UInt32 maxModeNumber = 0;
    UInt32 maxModeHeight = 0;
    UInt32 maxModeWidth = 0;

    for (UInt32 i = 0; i < modes; i++)
    {
        SLGraphicsModeInfo *mode = SLGraphicsOutputGetMode(graphics, i);

        if (mode->format != kSLGraphicsPixelFormatRGBX8 && mode->format != kSLGraphicsPixelFormatBGRX8)
            continue;

        if (mode->width > maxWidth)
            continue;

        if (mode->width > maxModeWidth)
            maxModeWidth = mode->width;
    }

    for (UInt32 i = 0; i < modes; i++)
    {
        SLGraphicsModeInfo *mode = SLGraphicsOutputGetMode(graphics, i);
        
        if (mode->format != kSLGraphicsPixelFormatRGBX8 && mode->format != kSLGraphicsPixelFormatBGRX8)
            continue;
        
        if (mode->width == maxModeWidth)
        {
            if (mode->height > maxHeight)
                continue;

            if (mode->height > maxModeHeight)
            {
                maxModeHeight = mode->height;
                maxModeNumber = i;
                maxMode = mode;
            }
        }
    }

    if (!maxMode) return kOSNullPointer;
    SLStatus status = graphics->setMode(graphics, maxModeNumber);
    if (!SLCheckStatus(status)) return kOSNullPointer;

    OSBuffer buffer = SLAllocate(sizeof(SLGraphicsContext));
    SLGraphicsContext *context = buffer.address;
    context->height = maxMode->height;
    context->width = maxMode->width;
    context->framebuffer = graphics->mode->framebuffer;
    context->framebufferSize = graphics->mode->framebufferSize;
    context->pixelCount = graphics->mode->framebufferSize / sizeof(UInt32);
    context->isBGRX = (maxMode->format == kSLGraphicsPixelFormatBGRX8);

    return context;
}

extern UInt8 gSLBitmapFont8x16Data[256 * 16];

void SLGraphicsContextWriteCharacter(SLGraphicsContext *context, UInt8 character, SLGraphicsPoint location, SLBitmapFont *font, UInt32 color, UInt32 bgColor)
{
    // This only supports the 8x16 bitmap font for now...
    if (font->height != 16 || font->width != 8) return;
    UInt8 *characterData = font->fontData + (character * font->height);

    for (OSCount i = 0; i < 16; i++)
    {
        UInt32 *rowPointer = context->framebuffer + (((location.y + i) * context->width) + location.x);
        UInt8 data = characterData[i];

        for (SInt8 j = (font->width - 1); j >= 0; j--)
        {
            UInt8 state = (data >> j) & 1;
            UInt32 fillValue = (state ? color : bgColor);

            rowPointer[j] = fillValue;
        }
    }
}

#if kCXBuildDev
    bool SLPromptUser(const char *s, SLSerialPort port)
    {
        SLPrintString("%s (y/n)? ", s);
        UInt8 response = 0;

        if (!gSLBootServicesEnabled) {
            while (response != 'y' && response != 'n')
                response = SLSerialReadCharacter(port, true);
        } else {
            gSLLoaderSystemTable->stdin->reset(gSLLoaderSystemTable->stdin, false);
            SLKeyPress key; key.keycode = 0;
            SLStatus status;

            while ((key.keycode != 'y' && response != 'y') && (key.keycode != 'n' && response != 'n'))
            {
                status = gSLLoaderSystemTable->stdin->readKey(gSLLoaderSystemTable->stdin, &key);
                response = SLSerialReadCharacter(port, false);

                if (status != kSLStatusNotReady)
                    response = key.keycode;
            }

            if (key.keycode == 'y')
                response = 'y';

            if (key.keycode == 'n')
                response = 'n';
        }

        SLPrintString("%c\n", response);
        return (response == 'y');
    }

    void SLShowDelay(const char *s, UInt64 seconds)
    {
        SLPrintString("%s in ", s);

        while (seconds)
        {
            SLPrintString("%d...", seconds);
            SLDelayProcessor(1000000, gSLBootServicesEnabled);
            SLDeleteCharacters(4);

            seconds--;
        }

        SLDeleteCharacters(3);
        SLPrintString("Now.\n");
    }

    void SLDumpProcessorState(bool standard, bool system, bool debug)
    {
        if (standard)
        {
            CXKProcessorBasicState state;

            CXKProcessorGetBasicState(&state);
            SLPrintString("Basic Register State:\n");
            SLPrintBasicState(&state);
            SLPrintString("\n");
        }

        if (system)
        {
            CXKProcessorSystemState state;

            CXKProcessorGetSystemState(&state);
            SLPrintString("System Register State:\n");
            SLPrintSystemState(&state);

            CXKProcessorMSR efer = CXKProcessorMSRRead(0xC0000080);
            SLPrintString("efer: 0x%zX\n", efer);
            SLPrintString("\n");
        }

        if (debug)
        {
            CXKProcessorDebugState state;

            CXKProcessorGetDebugState(&state);
            SLPrintString("Debug Register State:\n");
            SLPrintDebugState(&state);
            SLPrintString("\n");
        }
    }

    #define SLPrintRegister(s, r)   SLPrintString(OSStringValue(r) ": 0x%zX\n", s->r)
    #define SLPrintRegister16(s, r) SLPrintString(OSStringValue(r) ": 0x%hX\n", s->r)

    #define SLPrint2Registers(s, r0, r1)            \
        SLPrintRegister(s, r0);                     \
        SLPrintRegister(s, r1);

    #define SLPrint4Registers(s, r0, r1, r2, r3)    \
        SLPrint2Registers(s, r0, r1);               \
        SLPrint2Registers(s, r2, r3);

    #define SLPrint2Registers16(s, r0, r1)          \
        SLPrintRegister16(s, r0);                   \
        SLPrintRegister16(s, r1);

    #define SLPrint4Registers16(s, r0, r1, r2, r3)  \
        SLPrint2Registers16(s, r0, r1);             \
        SLPrint2Registers16(s, r2, r3);

    void SLPrintBasicState(CXKProcessorBasicState *state)
    {
        SLPrint4Registers(state, rax, rbx, rcx, rdx);
        SLPrint4Registers(state, r8,  r9,  r10, r11);
        SLPrint4Registers(state, r12, r13, r14, r15);
        SLPrint4Registers(state, rsi, rdi, rbp, rsp);
        SLPrint2Registers(state, rip, rflags);
        SLPrint4Registers16(state, cs, ds, ss, es);
        SLPrint2Registers16(state, fs, gs);
    }

    void SLPrintSystemState(CXKProcessorSystemState *state)
    {
        SLPrint4Registers(state, cr0, cr2, cr3, cr4);
        SLPrintRegister(state, cr8);

        SLPrintString("gdtr: 0x%X (limit = 0x%hX)\n", state->gdtr.base, state->gdtr.limit);
        SLPrintString("idtr: 0x%X (limit = 0x%hX)\n", state->idtr.base, state->idtr.limit);

        SLPrint2Registers16(state, ldtr, tr);
    }

    void SLPrintDebugState(CXKProcessorDebugState *state)
    {
        SLPrint4Registers(state, dr0, dr1, dr2, dr3);
        SLPrint2Registers(state, dr6, dr7);
    }

    void SLUnrecoverableError(void)
    {
        OSFault();
    }
#else /* !kCXBuildDev */
    void SLUnrecoverableError(void)
    {
        OSFault();
    }
#endif /* kCXBuildDev */

#if kCXDebug

    void __SLLibraryInitialize(void)
    {
        __SLBitmapFontInitialize();
        __SLVideoConsoleInitAll();
    }

#endif /* kCXDebug */
