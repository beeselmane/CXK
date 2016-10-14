OUTPUT_FORMAT("elf64-x86-64");
ENTRY(_SLEntry);

SECTIONS
{
    . = 0x000000;
    gSLLoaderBase = .;

    .hash : {
        *(.hash)
    } . = ALIGN(4096);
    .eh_frame : {
        *(.eh_frame)
    } . = ALIGN(4096);
    .text : {
        *(.text)
        *(.text.*)
        *(.stub)
        *(.gnu.linkonce.t.*)
        *(.gnu.warning)
        *(.plt)
    } . = ALIGN(4096);
    .data : {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        *(.rodata1)
        *(.sdata2)
        *(.sdata2.*)
        *(.gnu.linkonce.s2.*)
        *(.sbss2)
        *(.sbss2.*)
        *(.gnu.linkonce.sb2.*)
        *(.opd)
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        *(.data1)
        *(.plabel)
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    } . = ALIGN(4096);
    .sdata : {
        *(.got.plt)
        *(.got)
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
        *(.dynsbss)
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linonce.sb.*)
        *(.scommon)
    } . = ALIGN(4096);
    .dynamic : {
        *(.dynamic)
    } . = ALIGN(4096);
    .rela : {
        *(.rela.data.*)
        *(.rela.got)
        *(.rela.stab)
        *(.relaset_*)
    } . = ALIGN(4096);
    .reloc : {
        *(.reloc)
    } . = ALIGN(4096);
    .dynsym : {
        *(.dynsym)
    } . = ALIGN(4096);
    .dynstr : {
        *(.dynstr)
    } . = ALIGN(4096);

    gSLLoaderEnd = .;
}
