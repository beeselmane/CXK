#include <SystemLoader/SystemLoader.h>

SLEFIABI bool SLValidateELFHeader(OSELFFileHeader64 *header)
{
    bool valid = true;

    if (header->magic != kOSELFMagic) {
        SLPrintString(SLS("Invalid ELF Magic Number!\r\n"));
        SLPrintString(SLS("M: %X\n"), header->magic);
        valid = false;
    } if (header->is64Bit != kOSELFFileHeaderIs64Bit || header->endian != kOSELFFileHeaderIsLittleEndian) {
        SLPrintString(SLS("Either 32 bit or Big Endian ELF File!\r\n"));
        SLPrintString(SLS("C: %u, E: %u\r\n"), header->is64Bit, header->endian);
        valid = false;
    } if (header->iVersion != kOSELFFileHeaderVersion1 || header->version != kOSELFFileHeaderVersion1) {
        SLPrintString(SLS("ELF Version is not 1.0!\r\n"));
        SLPrintString(SLS("iV: %u, V: %u\r\n"), header->iVersion, header->version);
        valid = false;
    } if (header->abi != kOSELFABISystemV) {
        SLPrintString(SLS("ELF File specifies ABI that is not System V!\r\n"));
        SLPrintString(SLS("S: %u\r\n"), header->abi);
        valid = false;
    } if (header->fileType != kOSELFFileTypeExecutable) {
        SLPrintString(SLS("ELF File is not Executable!\r\n"));
        SLPrintString(SLS("F: %u\r\n"), header->fileType);
        valid = false;
    } if (header->machineType != kOSELFCPUTypeX86_64) {
        SLPrintString(SLS("ELF File is not Built for x86_64 Architecture!\r\n"));
        SLPrintString(SLS("A: %u\r\n"), header->machineType);
        valid = false;
    } if (header->segmentCount != 1) {
        SLPrintString(SLS("Segment Count is not 1!\r\n"));
        SLPrintString(SLS("SC: %u\r\n"), header->segmentCount);
        valid = false;
    }

    return valid;
}

SLEFIABI void SLStartSetup(SLFileHandle setupFile, CXKBootOptions *options)
{
    SLConfigOptions *config = SLGetConfigOptions();

    OSBuffer buffer = SLAllocateBuffer(sizeof(OSELFFileHeader64));
    OSELFFileHeader64 *fileHeader = buffer.address;
    if (OSBufferIsEmpty(buffer)) return;

    if (!SLReadFile(setupFile, 0, buffer)) return;
    if (!SLValidateELFHeader(fileHeader)) return;

    UInt64 segmentHeaderOffset = fileHeader->segmentHeaderOffset;
    UInt64 segmentHeaderSize = (fileHeader->segmentHeaderSize > sizeof(OSELFSegmentHeader64)) ? fileHeader->segmentHeaderSize : sizeof(OSELFSegmentHeader64);
    UInt64 entryPoint = fileHeader->entryPoint;

    if (!SLFreeBuffer(buffer)) return;
    buffer = SLAllocateBuffer(segmentHeaderSize);
    OSELFSegmentHeader64 *segmentHeader = buffer.address;
    if (OSBufferIsEmpty(buffer)) return;

    if (!SLReadFile(setupFile, segmentHeaderOffset, buffer)) return;
    OSCount pages = segmentHeader->sizeInMemory / kSLLoaderBootPageSize;
    if (segmentHeader->sizeInMemory % kSLLoaderBootPageSize) pages++;
    SLPrintString(SLS("Allocating %llu pages from %llX...\r\n"), pages, config->setupAddress);
    if (!SLAllocateBootPagesAtAddress(config->setupAddress, pages));
    buffer = OSBufferMake(config->setupAddress, segmentHeader->sizeInFile);
    if (!SLReadFile(setupFile, segmentHeader->offset, buffer)) return;
    buffer = OSBufferMake(segmentHeader, segmentHeaderSize);
    if (!SLFreeBuffer(buffer)) return;

    void (*entry)(CXKBootOptions *options) = (void (*)())(config->setupAddress + entryPoint);

    SLPrintString(SLS("Loaded Segment 0 into Memory!\r\n"));
    SLPrintString(SLS("Entry will be called at 0x%p\r\n"), entry);
    SLPrintString(SLS("Disabling EFI Boot Services...\r\n"));

    options->memoryMap = SLDeactivateBootServices();
    if (!options->memoryMap.key) return;
    entry(options);
}
