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
    OSBuffer buffer = SLBootServicesAllocate(sizeof(CXKMemoryMap));
    if (OSBufferIsEmpty(buffer)) return kOSNullPointer;
    CXKMemoryMap *map = buffer.address;
    UIntN entrySize;
    UInt32 version;

    SLStatus status = gSLLoaderSystemTable->bootServices->getMemoryMap(&buffer.size, map->entries, &map->key, &entrySize, &version);
    if (entrySize != sizeof(CXKMemoryMapEntry)) status = kSLStatusWrongSize;
    if (status == kSLStatusBufferTooSmall) status = kSLStatusSuccess;
    if (version != 1) status = kSLStatusIncompatibleVersion;
    if (!SLCheckStatus(status)) goto failure;

    map->entryCount = (buffer.size / entrySize) + 1;
    buffer.size += entrySize;
    buffer = SLBootServicesAllocate(buffer.size);
    if (OSBufferIsEmpty(buffer)) goto failure;
    map->entries = buffer.address;

    status = gSLLoaderSystemTable->bootServices->getMemoryMap(&buffer.size, map->entries, &map->key, &entrySize, &version);
    if (entrySize != sizeof(CXKMemoryMapEntry)) status = kSLStatusWrongSize;
    if (version != 1) status = kSLStatusIncompatibleVersion;
    if (!SLCheckStatus(status)) goto failure;

    return map;

failure:
    if (map->entries)
    {
        buffer.size = (map->entryCount * sizeof(CXKMemoryMapEntry));
        buffer.address = map->entries;

        SLBootServicesFree(buffer);
    }

    buffer.size = sizeof(CXKMemoryMap);
    buffer.address = map;

    SLBootServicesFree(buffer);
    return map;
}

CXKMemoryMap *SLBootServicesTerminate(void)
{
    CXKMemoryMap *finalMemoryMap = SLBootServicesGetMemoryMap();
    if (!finalMemoryMap) return kOSNullPointer;

    SLStatus status = gSLLoaderSystemTable->bootServices->terminate(gSLLoaderImageHandle, finalMemoryMap->key);

    if (!SLCheckStatus(status)) {
        OSBuffer buffer = OSBufferMake(finalMemoryMap, (finalMemoryMap->entryCount * sizeof(CXKMemoryMapEntry)));
        SLBootServicesFree(buffer);

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
        // Assume ~3 GHz to try to mimic BS stall() function
        for (volatile UInt64 i = 0; i < (time * 3000000); i++);
        return true;
    }
}

char SLWaitForKeyPress(void)
{
    SLStatus status = gSLLoaderSystemTable->stdin->reset(gSLLoaderSystemTable->stdin, false);
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

    OSBuffer buffer = SLBootServicesAllocate(fileInfo.size);
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
            SLStatus status;
            SLKeyPress key;

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
        SLDelayProcessor(1000000, true);
        SLPrint4Registers(state, r12, r13, r14, r15);
        SLPrint4Registers(state, rsi, rdi, rbp, rsp);
        SLDelayProcessor(1000000, true);
        SLPrint2Registers(state, rip, rflags);
        SLPrint4Registers16(state, cs, ds, ss, es);
        SLPrint2Registers16(state, fs, gs);
        SLDelayProcessor(1000000, true);
    }

    void SLPrintSystemState(CXKProcessorSystemState *state)
    {
        SLPrint4Registers(state, cr0, cr2, cr3, cr4);
        SLPrintRegister(state, cr8);
        SLDelayProcessor(1000000, true);

        SLPrintString("gdtr: 0x%X (limit = 0x%hX)\n", state->gdtr.base, state->gdtr.limit);
        SLPrintString("idtr: 0x%X (limit = 0x%hX)\n", state->idtr.base, state->idtr.limit);

        SLPrint2Registers16(state, ldtr, tr);
        SLDelayProcessor(1000000, true);
    }

    void SLPrintDebugState(CXKProcessorDebugState *state)
    {
        SLPrint4Registers(state, dr0, dr1, dr2, dr3);
        SLPrint2Registers(state, dr6, dr7);
    }
#endif /* kCXBuildDev */
