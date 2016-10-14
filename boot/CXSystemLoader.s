#include <Kernel/CXKAssembly.h>

#if kCXArchIA

.section kCXKCodeSectionName
.align kCXKNaturalAlignment

// Arguments:
//   %rcx: Image Handle
//   %rdx: System Table
//
// Return Value (%rax):
//   0: success
//   other: failure
//
// Destroyed Registers:
//   %rax
//   %rbx
//   %rcx
//   %rdx
//
// If Debug:
//
CXKDeclareFunction(_SLEntry):
    popq %rax
    pushq %rax

    CXKLoadSymbol(gSLFirmwareReturnAddress, %rbx)
    movq %rax, (%rbx)

    CXKLoadSymbol(gSLLoaderImageHandle, %rbx)
    movq %rcx, (%rbx)

    CXKLoadSymbol(gSLLoaderSystemTable, %rbx)
    movq %rdx, (%rbx)

    #if kCXBuildDev
        leaq gSLLoaderBase(%rip), %rbx
        leaq gSLLoaderEnd(%rip), %rax
        subq %rbx, %rax
        CXKLoadSymbol(gSLLoaderImageSize, %rbx)
        movq %rax, (%rbx)
    #endif /* kCXBuildDev */

    leaq CXSystemLoaderMain(%rip), %rax
    callq *%rax

    movq %rax, %rcx
    leaq SLLeave(%rip), %rax
    callq *%rax

.align kCXKNaturalAlignment

// Arguments:
//   %rcx: Exit Code
//
// This function will exit the loader
// without returning. It can do whatever
// it wants pretty much...
CXKDeclareFunction(SLLeave):
    movq %rcx, %rdx
    CXKLoadSymbol(gSLLoaderImageHandle, %rbx)
    movq (%rbx), %rcx
    xorq %r8, %r8
    xorq %r9, %r9
    CXKLoadSymbol(gSLLoaderSystemTable, %rax)
    movq (%rax), %rbx
    movq 0x60(%rbx), %rax
    callq *0xD8(%rax)

    // If we're here, there was an error exiting.
    // Return to the address given by the firmware
    // on the initial program stack (saved in entry)
    CXKLoadSymbol(gSLFirmwareReturnAddress, %rbx)
    movq (%rbx), %rax // Load return address into accumulator
    popq %rbx         // Kill real return address on the stack
    pushq (%rax)      // Inject firmware return address
    ret               // Return to given address

.comm gSLFirmwareReturnAddress, 8, 8
.comm gSLLoaderSystemTable,     8, 8
.comm gSLLoaderImageHandle,     8, 8

#if kCXBuildDev
    .comm gSLLoaderImageSize,   8, 8
#endif /* kCXBuildDev */

.section .reloc, "a", @progbits

// Pretend like we're a relocatable executable...
.int 0
.int 10
.word 0

#endif /* Arch */
