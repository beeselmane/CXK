#include "CXEFI.h"
#undef NULL
#include <Kernel/CXKBasicSerial.h>
#include <Kernel/CXKMemoryIO.h>
#include <Kernel/CXKPOST.h>
#define UIntN UINTN

#define kCXLoaderVersionA 'A'
#define kCXLoaderVersionB '1'

#define kCXLoaderVersion "A.1"
#define kCXLoaderBuild   "000E"

#define kCXLoaderStartTextU CXUTF16String("Corona-X System Loader Version " kCXLoaderVersion " [Build " kCXLoaderBuild "]\r\n")

static CXKBasicSerialPort *gSerialPort0;
static CXHandle gImageHandle;

UTF16String CXMemoryTypeToString(CXMemoryType type)
{
    switch (type)
    {
        case kCXMemoryTypeReserved:         return CXUTF16String("Reserved");
        case kCXMemoryTypeLoaderCode:       return CXUTF16String("Loader Code");
        case kCXMemoryTypeLoaderData:       return CXUTF16String("Loader Data");
        case kCXMemoryTypeBootCode:         return CXUTF16String("Boot Code");
        case kCXMemoryTypeBootData:         return CXUTF16String("Boot Data");
        case kCXMemoryTypeRuntimeCode:      return CXUTF16String("Runtime Code");
        case kCXMemoryTypeRuntimeData:      return CXUTF16String("Runtime Data");
        case kCXMemoryTypeConventional:     return CXUTF16String("Convention");
        case kCXMemoryTypeUnusable:         return CXUTF16String("Unusable");
        case kCXMemoryTypeACPIReclaimed:    return CXUTF16String("ACPI Reclaimable");
        case kCXMemoryTypeACPINVS:          return CXUTF16String("ACPI NVS");
        case kCXMemoryTypeMappedIO:         return CXUTF16String("Mapped IO");
        case kCXMemoryTypeMappedIOPorts:    return CXUTF16String("Mapped IO Ports");
        case kCXMemoryTypePALCode:          return CXUTF16String("PAL Codes");
        case kCXMemorytypePersistent:       return CXUTF16String("Persistent");
        case kCXMemoryTypeMaxMemory:        return CXUTF16String("Max Memory");
        default:                            return CXUTF16String("(Unknown)");
    }
}

UIntN CXIsRuntimeReserved(CXMemoryDescriptor *descriptor)
{
    switch (descriptor->type)
    {
        case kCXMemoryTypeReserved:
        case kCXMemoryTypeRuntimeCode:
        case kCXMemoryTypeRuntimeData:
        case kCXMemoryTypeUnusable:
        case kCXMemoryTypeACPIReclaimed:
        case kCXMemoryTypeACPINVS:
        case kCXMemoryTypeMappedIO:
        case kCXMemoryTypeMappedIOPorts:
        case kCXMemoryTypePALCode:
        case kCXMemoryTypeMaxMemory:
            return true;
        default: return false;
    }
}

CXStatus CXSystemLoaderWaitForKey(CXSystemTable systemTable)
{
    CXStatus status; CXKey key;
    status = systemTable.stdin->reset(systemTable.stdin, false);

    while ((status = systemTable.stdin->readKey(systemTable.stdin, &key)) == kCXNotReady);
    if (key.character == 'q') systemTable.bootServices->exit(gImageHandle, 1, 0, NULL);

    if (key.character == 'c')
    {
        systemTable.stdout->clear(systemTable.stdout);
        return CXSystemLoaderWaitForKey(systemTable);
    }

    return status;
}

typedef struct {
    UInt32 entryType;
    UInt32 padding0;
    OSAddress physicalAddress;
    OSAddress virtualAddress;
    UInt64 pageCount;
    UInt64 attributes;
    UInt64 padding1;
} CXMemoryMapEntry;

CXStatus CXSystemLoaderPrintMemoryMapStatistics(CXSystemTable systemTable)
{
    UIntN memoryMapSize = 0;
    CXMemoryDescriptor *mmap = NULL;
    UIntN mapKey;
    UIntN descriptorSize = 0;
    UInt32 descriptorVersion = 0;

    CXStatus status = systemTable.bootServices->getMemoryMap(&memoryMapSize, mmap, &mapKey, &descriptorSize, &descriptorVersion);
    status = systemTable.bootServices->allocate(kCXMemoryTypeBootData, memoryMapSize, (void **)&mmap);
    status = systemTable.bootServices->getMemoryMap(&memoryMapSize, mmap, &mapKey, &descriptorSize, &descriptorVersion);
    if (CXCheckIsError(status)) return status;

    printf(CXUTF16String("Memory Map Size: %lu (/ %lu) [s%lu]\r\n"), memoryMapSize, descriptorSize, sizeof(CXMemoryDescriptor));
    printf(CXUTF16String("Map Key: %lu, Descriptor Version: %u\r\n"), mapKey, descriptorVersion);

    CXMemoryMapEntry *entryList = (CXMemoryMapEntry *)mmap;
    UIntN descriptorCount = memoryMapSize / descriptorSize;
    UIntN osReserved = 0;
    UIntN memoryMax = 0;

    for (UIntN i = 0; i < descriptorCount; i++)
    {
        CXMemoryMapEntry *entry = &entryList[i];

        if (CXIsRuntimeReserved((CXMemoryDescriptor *)entry))
            osReserved++;

        if (entry->entryType == kCXMemoryTypeMaxMemory)
            memoryMax++;
    }

    printf(CXUTF16String("Counted %lu Descriptors\r\n"), descriptorCount);
    printf(CXUTF16String("%lu Descriptors will remain after Boot Service Exit\r\n\r"), osReserved);
    printf(CXUTF16String("%lu Max Memory Descriptors Found.\r\n"), memoryMax);
    /*printf(CXUTF16String("Descriptors which will persist:\r\n"));

    for (UIntN i = 0; i < descriptorCount; i++)
    {
        CXMemoryMapEntry *entry = &entryList[i];

        if (CXIsRuntimeReserved((CXMemoryDescriptor *)entry))
        {
            UTF16String name = CXMemoryTypeToString(entry->entryType);
            printf(CXUTF16String("%s: %lu pages from 0x%p [Virtual 0x%p]\r\n"), name, entry->pageCount, entry->physicalAddress, entry->virtualAddress);
        }
    }*/

    printf(CXUTF16String("\r\n"));
    systemTable.bootServices->free(mmap);
    return status;
}

UTF16String CXSystemLoaderGetLoaderPath(CXHandle imageHandle, CXSystemTable systemTable)
{
    CXLoadedImage *image = NULL;
    CXProtocol loadedImageProtocol = kCXLoadedImageProtocol;
    CXStatus status;

    status = systemTable.bootServices->handleProtocol(imageHandle, &loadedImageProtocol, (void **)&image);
    if (CXCheckIsError(status)) return CXUTF16String("(Status Error)");

    return CXDevicePathToString(image->filePath, true, false);
}

void CLSerialWrite(const char *string)
{
    while (s) CXKBasicSerialPortWriteCharacter(gSerialPort0, *s++, true);
}

void noSerialCheck()
{
    printf(CXUTF16String("Error: No Serial Port Found!!\r\n"));
    static UInt8 testValue = 0xC7;

    CXKBasicSerialPort *port = (CXKBasicSerialPort *)kCXKBasicSerialDefaultOffset;
    printf(CXUTF16String("Serial Base Address: 0x%p\r\n"), port);
    printf(CXUTF16String("Serial Scratch Port: 0x%p\r\n"), &port->scratchSpace);

    CXKWriteByteToPort(&port->scratchSpace, testValue);
    UInt8 portValue = CXKReadByteFromPort(&port->scratchSpace);

    printf(CXUTF16String("Wrote 0x%02X, Read 0x%02X\r\n"), testValue, portValue);
    printf(CXUTF16String("Serial Port Unaccessable.\r\n\r\n"));
}

void CXSerialSetup()
{
    CXKBasicSerialPortSetupLineControl(gSerialPort0, kCXKBasicSerialWordLength8Bits, kCXKBasicSerial1StopBit, kCXKBasicSerialNoParity);
    CXKBasicSerialPortSetBaudRate(gSerialPort0, 57600);

#if 0
    UInt8 interruptsEnabled = CXKReadByteFromPort(&gSerialPort0->byte1.interruptsEnabled);
    UInt8 interruptInfo = CXKReadByteFromPort(&gSerialPort0->byte2.interruptInfo);
    UInt8 lineControl = CXKReadByteFromPort(&gSerialPort0->lineControl);
    UInt8 modemControl = CXKReadByteFromPort(&gSerialPort0->modemControl);
    UInt8 lineStatus = CXKReadByteFromPort(&gSerialPort0->lineStatus);
    UInt8 modemStatus = CXKReadByteFromPort(&gSerialPort0->modemStatus);
    UInt8 scratchSpace = CXKReadByteFromPort(&gSerialPort0->scratchSpace);

    CXKWriteByteToPort(&gSerialPort0->lineControl, lineControl | (1 << kCXKBasicSerialDivisorExposeShift));
    UInt8 divisorLSB = CXKReadByteFromPort(&gSerialPort0->byte0.divisorLSB);
    UInt8 divisorMSB = CXKReadByteFromPort(&gSerialPort0->byte1.divisorMSB);
    CXKWriteByteToPort(&gSerialPort0->lineControl, lineControl);

    printf(CXUTF16String("Discovered Serial Port at 0x%p:\r\n"), gSerialPort0);
    printf(CXUTF16String("IE [0x%p]: %02X\r\n"), &gSerialPort0->byte1.interruptsEnabled, interruptsEnabled);
    printf(CXUTF16String("II [0x%p]: %02X\r\n"), &gSerialPort0->byte2.interruptInfo, interruptInfo);
    printf(CXUTF16String("LC [0x%p]: %02X\r\n"), &gSerialPort0->lineControl, lineControl);
    printf(CXUTF16String("MC [0x%p]: %02X\r\n"), &gSerialPort0->modemControl, modemControl);
    printf(CXUTF16String("LS [0x%p]: %02X\r\n"), &gSerialPort0->lineStatus, lineStatus);
    printf(CXUTF16String("MS [0x%p]: %02X\r\n"), &gSerialPort0->modemStatus, modemStatus);
    printf(CXUTF16String("SS [0x%p]: %02X\r\n"), &gSerialPort0->scratchSpace, scratchSpace);
    printf(CXUTF16String("DL [0x%p]: %02X\r\n"), &gSerialPort0->byte0.divisorLSB, divisorLSB);
    printf(CXUTF16String("DH [0x%p]: %02X\r\n"), &gSerialPort0->byte1.divisorMSB, divisorMSB);
    printf(CXUTF16String("\r\n"));
#endif

    const char *startText = {'C', 'X', 'B', 'L', 'v', kCXLoaderVersionA, '.', kCXLoaderVersionB, '\r', '\n'};l
    CLSerialWrite(startText);
}

#define kCXLoaderDirectory CXUTF16String("EFI\\corona")
;

extern void CXLoadSetupImage(CXFileHandle folderPath, CXSystemTable *systemTable);

void startLoadSetup(CXSystemTable *systemTable, CXHandle imageHandle)
{
    CXKSetPOSTValue(0x00);

    CXLoadedImage *image = NULL;
    CXProtocol loadedImageProtocol = kCXLoadedImageProtocol;
    CXProtocol volumeProtocol = kCXVolumeProtocol;
    CXFileHandle rootDirectory;
    CXFileHandle folderPath;
    CXVolumeHandle *volume;
    CXStatus status;

    status = systemTable->bootServices->handleProtocol(imageHandle, &loadedImageProtocol, (void **)&image);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE0);
        return;
    } else {
        CXKSetPOSTValue(0x01);
    }

    status = systemTable->bootServices->handleProtocol(image->deviceHandle, &volumeProtocol, (void *)&volume);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE1);
        return;
    } else {
        CXKSetPOSTValue(0x02);
    }

    status = volume->openRoot(volume, &rootDirectory);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE2);
        return;
    } else {
        CXKSetPOSTValue(0x03);
    }

    status = rootDirectory->open(rootDirectory, &folderPath, kCXLoaderDirectory, kCXFileModeRead, 0);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE3);
        return;
    } else {
        CXKSetPOSTValue(0x04);
    }

    CXLoadSetupImage(folderPath, systemTable);
}

CXStatus CXEFI CXSystemLoaderMain(input CXHandle imageHandle, input CXSystemTable *systemTable)
{
    CXStatus status;
    gImageHandle = imageHandle;
    CXKSetPOSTValue(0xFF);

    printf(kCXLoaderStartTextU);
    gSerialPort0 = CXKBasicSerialPortIntializeAtAddress((OSAddress)kCXKBasicSerialDefaultOffset);

    if (!gSerialPort0) noSerialCheck();
    else               CXSerialSetup();

    printf(CXUTF16String("Loader Path: %s\r\n\r\n"), CXSystemLoaderGetLoaderPath(imageHandle, *systemTable));
    status = CXSystemLoaderPrintMemoryMapStatistics(*systemTable);
    status = CXSystemLoaderWaitForKey(*systemTable);
    startLoadSetup(systemTable, imageHandle);
    CXSystemLoaderWaitForKey(*systemTable);

    return status;
}
