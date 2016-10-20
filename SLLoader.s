	.file	"SLLoader.c"
	.local	gSLSerialPort0
	.comm	gSLSerialPort0,2,2
	.section	.rodata
.LC0:
	.string	"\b \b"
	.text
	.globl	SLLoaderSerial0OutputUTF8
	.type	SLLoaderSerial0OutputUTF8, @function
SLLoaderSerial0OutputUTF8:
.LFB15:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%ecx, %eax
	movb	%al, 16(%rbp)
	cmpb	$8, 16(%rbp)
	jne	.L2
	movzwl	gSLSerialPort0(%rip), %eax
	movzwl	%ax, %eax
	leaq	.LC0(%rip), %rdx
	movl	%eax, %ecx
	call	SLSerialWriteString
	jmp	.L1
.L2:
	movzbl	16(%rbp), %edx
	movzwl	gSLSerialPort0(%rip), %eax
	movzwl	%ax, %eax
	movl	$1, %r8d
	movl	%eax, %ecx
	call	SLSerialWriteCharacter
.L1:
	leave
	.cfi_restore 6
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE15:
	.size	SLLoaderSerial0OutputUTF8, .-SLLoaderSerial0OutputUTF8
	.section	.rodata
	.align 8
.LC1:
	.string	"Error Loading Serial Port! (Tried port %p)\n"
	.align 8
.LC2:
	.string	"Error Registering Serial Port 0 (at %p) to receive output!\n"
	.text
	.globl	SLLoaderSetupSerial
	.type	SLLoaderSetupSerial, @function
SLLoaderSetupSerial:
.LFB16:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movq	$1016, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, %rcx
	call	SLSerialPortInit
	movw	%ax, -10(%rbp)
	cmpw	$-1, -10(%rbp)
	jne	.L5
	movq	-8(%rbp), %rax
	movq	%rax, %rdx
	leaq	.LC1(%rip), %rcx
	call	SLPrintError
	movw	$0, gSLSerialPort0(%rip)
	jmp	.L4
.L5:
	movzwl	-10(%rbp), %eax
	movl	$0, %r9d
	movl	$0, %r8d
	movl	$3, %edx
	movl	%eax, %ecx
	call	SLSerialPortSetupLineControl
	movzwl	-10(%rbp), %eax
	movl	$2, %edx
	movl	%eax, %ecx
	call	SLSerialPortSetBaudDivisor
	movzwl	-10(%rbp), %eax
	movw	%ax, gSLSerialPort0(%rip)
	movq	SLLoaderSerial0OutputUTF8@GOTPCREL(%rip), %rax
	movq	%rax, %rcx
	call	SLRegisterOutputFunction
	movb	%al, -11(%rbp)
	movzbl	-11(%rbp), %eax
	xorl	$1, %eax
	testb	%al, %al
	je	.L7
	movq	-8(%rbp), %rax
	movq	%rax, %rdx
	leaq	.LC2(%rip), %rcx
	call	SLPrintError
	jmp	.L4
.L7:
	movl	$0, %ecx
	call	SLSetBootScreenOutput
.L4:
	leave
	.cfi_restore 6
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE16:
	.size	SLLoaderSetupSerial, .-SLLoaderSetupSerial
	.section	.rodata
	.align 8
.LC3:
	.string	"Corona-X System Loader Version A.4 [Build 0018]\r\n"
.LC4:
	.string	"Run Tests"
.LC5:
	.string	"Running"
	.align 8
.LC6:
	.string	"Not running tests; Need a serial port.\n"
.LC7:
	.string	"Starting Loader. CPU States:\n"
	.text
	.globl	CXSystemLoaderMain
	.type	CXSystemLoaderMain, @function
CXSystemLoaderMain:
.LFB17:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	call	SLLoaderSetupSerial@PLT
	leaq	.LC3(%rip), %rcx
	call	SLPrintString
	movzwl	gSLSerialPort0(%rip), %eax
	testw	%ax, %ax
	je	.L9
	movzwl	gSLSerialPort0(%rip), %eax
	movzwl	%ax, %eax
	movl	%eax, %edx
	leaq	.LC4(%rip), %rcx
	call	SLPromptUser
	movb	%al, -1(%rbp)
	cmpb	$0, -1(%rbp)
	je	.L11
	movl	$3, %edx
	leaq	.LC5(%rip), %rcx
	call	SLShowDelay
	call	SLRunTests@PLT
	jmp	.L11
.L9:
	leaq	.LC6(%rip), %rcx
	call	SLPrintString
.L11:
	leaq	.LC7(%rip), %rcx
	call	SLPrintString
	movl	$1, %r8d
	movl	$0, %edx
	movl	$1, %ecx
	call	SLDumpProcessorState
	movl	$0, %eax
	leave
	.cfi_restore 6
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE17:
	.size	CXSystemLoaderMain, .-CXSystemLoaderMain
	.hidden	SLDumpProcessorState
	.hidden	SLShowDelay
	.hidden	SLPromptUser
	.hidden	SLPrintString
	.hidden	SLSetBootScreenOutput
	.hidden	SLRegisterOutputFunction
	.hidden	SLSerialPortSetBaudDivisor
	.hidden	SLSerialPortSetupLineControl
	.hidden	SLPrintError
	.hidden	SLSerialPortInit
	.hidden	SLSerialWriteCharacter
	.hidden	SLSerialWriteString
	.ident	"GCC: (GNU) 6.2.0"
