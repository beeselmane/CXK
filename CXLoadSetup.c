#include "CXEFI.h"
#undef NULL
#include <Kernel/CXKBasicSerial.h>
#include <Kernel/CXKMemoryIO.h>
#include <Kernel/CXKPOST.h>

#include <System/Executables/OSELF.h>
#define UIntN UINTN

#define kCXKernelLoaderName  CXUTF16String("CXKSetup.elf")
#define kCXKSetupLoadAddress 0x4000000

bool CXValidateELFHeader(OSELFFileHeader64 *header)
{
    bool valid = true;

    if (header->magic != kOSELFMagic) {
        printf(CXUTF16String("Invalid ELF Magic Number!\r\n"));
        printf(CXUTF16String("M: %X\n"), header->magic);
        valid = false;
    } if (header->is64Bit != kOSELFFileHeaderIs64Bit || header->endian != kOSELFFileHeaderIsLittleEndian) {
        printf(CXUTF16String("Either 32 bit or Big Endian ELF File!\r\n"));
        printf(CXUTF16String("C: %u, E: %u\r\n"), header->is64Bit, header->endian);
        valid = false;
    } if (header->iVersion != kOSELFFileHeaderVersion1 || header->version != kOSELFFileHeaderVersion1) {
        printf(CXUTF16String("ELF Version is not 1.0!\r\n"));
        printf(CXUTF16String("iV: %u, V: %u\r\n"), header->iVersion, header->version);
        valid = false;
    } if (header->abi != kOSELFABISystemV) {
        printf(CXUTF16String("ELF File specifies ABI that is not System V!\r\n"));
        printf(CXUTF16String("S: %u\r\n"), header->abi);
        valid = false;
    } if (header->fileType != kOSELFFileTypeExecutable) {
        printf(CXUTF16String("ELF File is not Executable!\r\n"));
        printf(CXUTF16String("F: %u\r\n"), header->fileType);
        valid = false;
    } if (header->machineType != kOSELFCPUTypeX86_64) {
        printf(CXUTF16String("ELF File is not Built for x86_64 Architecture!\r\n"));
        printf(CXUTF16String("A: %u\r\n"), header->machineType);
        valid = false;
    } if (header->segmentCount != 1) {
        printf(CXUTF16String("Segment Count is not 1!\r\n"));
        printf(CXUTF16String("SC: %u\r\n"), header->segmentCount);
        valid = false;
    }

    if (!valid)
    {
        printf(CXUTF16String("%02X %02X %02X %02X %02X %02X %02X %02X\r\n"), header->abiVersion,
               header->padding[0], header->padding[1], header->padding[2], header->padding[3],
               header->padding[4], header->padding[5], header->padding[6]);
    }

    return valid;
}

void CXLoadSetupImage(CXFileHandle folderPath, CXSystemTable *systemTable)
{
    CXFileHandle setupFile;
    CXKSetPOSTValue(0x05);

    CXStatus status = folderPath->open(folderPath, &setupFile, kCXKernelLoaderName, kCXFileModeRead, 0);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE4);
        return;
    } else {
        CXKSetPOSTValue(0x06);
    }

    OSELFFileHeader64 *fileHeader;
    printf(CXUTF16String("Loaded Setup Image File!\r\n"));
    status = systemTable->bootServices->allocate(kCXMemoryTypeLoaderData, sizeof(OSELFFileHeader64), (void **)&fileHeader);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE5);
        return;
    } else {
        CXKSetPOSTValue(0x07);
    }

    UIntN readSize = sizeof(OSELFFileHeader64);
    status = setupFile->read(setupFile, &readSize, fileHeader);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE6);
        return;
    } else if (readSize != sizeof(OSELFFileHeader64)) {
        CXKSetPOSTValue(0xE7);
        return;
    } else {
        CXKSetPOSTValue(0x08);
    }

    if (!CXValidateELFHeader(fileHeader)) {
        CXKSetPOSTValue(0xE8);
        return;
    } else {
        CXKSetPOSTValue(0x09);
    }

    UInt64 segmentHeaderOffset = fileHeader->segmentHeaderOffset;
    UInt16 segmentHeaderSize = fileHeader->segmentHeaderSize;
    UInt64 entryPoint = fileHeader->entryPoint;
    UInt64 segmentHeaderASize = (segmentHeaderSize > sizeof(OSELFSegmentHeader64)) ? segmentHeaderSize : sizeof(OSELFSegmentHeader64);
    status = setupFile->setOffset(setupFile, segmentHeaderOffset);
    systemTable->bootServices->free(fileHeader);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xE9);
        return;
    } else {
        CXKSetPOSTValue(0x0A);
    }

    OSELFSegmentHeader64 *segmentHeader;
    status = systemTable->bootServices->allocate(kCXMemoryTypeBootCode, segmentHeaderASize, (void **)segmentHeader);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xEA);
        return;
    } else {
        CXKSetPOSTValue(0x0B);
    }

    readSize = segmentHeaderSize;
    status = setupFile->read(setupFile, &readSize, segmentHeader);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xEB);
        return;
    } else if (readSize != segmentHeaderSize) {
        CXKSetPOSTValue(0xEC);
        return;
    } else {
        CXKSetPOSTValue(0x0C);
    }

    status = setupFile->setOffset(setupFile, 0);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xED);
        return;
    } else {
        CXKSetPOSTValue(0x0D);
    }

    static UInt64 pageSize = 4096;
    UInt64 memorySize = segmentHeader->sizeInMemory + segmentHeader->virtualAddress;
    UInt64 pages = memorySize / pageSize;
    if (memorySize % pageSize) pages++;

    UInt64 loadAddressV = kCXKSetupLoadAddress;
    printf(CXUTF16String("Attemping to allocate %llu pages from 0x%llx...\r\n"), pages, loadAddressV);
    status = systemTable->bootServices->allocatePages(kCXAllocAtAddress, kCXMemoryTypeBootCode, pages, (long long unsigned int *)&loadAddressV);
    OSAddress loadAddress = (OSAddress)(UInt8 *)loadAddressV;

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xEE);
        return;
    } else {
        CXKSetPOSTValue(0x0E);
    }

    readSize = segmentHeader->sizeInFile;
    UInt8 *readAddress = (UInt8 *)(loadAddress + segmentHeader->virtualAddress);
    status = setupFile->read(setupFile, &readSize, readAddress);
    systemTable->bootServices->free(segmentHeader);

    if (CXCheckIsError(status)) {
        CXKSetPOSTValue(0xEF);
        return;
    } else if (readSize != segmentHeader->sizeInFile) {
        CXKSetPOSTValue(0xFE);
        return;
    } else {
        CXKSetPOSTValue(0x0F);
    }

    void (*entry)() = (void (*)())(loadAddress + entryPoint);
    printf(CXUTF16String("Calling Entry at 0x%p...\r\n"), entry);
    entry();
}
