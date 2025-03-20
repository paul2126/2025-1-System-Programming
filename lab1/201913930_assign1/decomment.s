	.file	"decomment.c"
	.text
	.globl	line_cur
	.data
	.align 4
	.type	line_cur, @object
	.size	line_cur, 4
line_cur:
	.long	1
	.text
	.globl	main
	.type	main, @function
main:
.LFB6:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	$0, -8(%rbp)
.L21:
	call	getchar@PLT
	movl	%eax, -4(%rbp)
	cmpl	$-1, -4(%rbp)
	je	.L24
	movl	-4(%rbp), %eax
	movb	%al, -9(%rbp)
	cmpl	$-1, -4(%rbp)
	je	.L25
	cmpl	$-1, -4(%rbp)
	jl	.L5
	cmpl	$47, -4(%rbp)
	jg	.L5
	cmpl	$10, -4(%rbp)
	jl	.L5
	movl	-4(%rbp), %eax
	subl	$10, %eax
	cmpl	$37, %eax
	ja	.L5
	movl	%eax, %eax
	leaq	0(,%rax,4), %rdx
	leaq	.L7(%rip), %rax
	movl	(%rdx,%rax), %eax
	cltq
	leaq	.L7(%rip), %rdx
	addq	%rdx, %rax
	notrack jmp	*%rax
	.section	.rodata
	.align 4
	.align 4
.L7:
	.long	.L11-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L10-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L9-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L8-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L5-.L7
	.long	.L6-.L7
	.text
.L6:
	cmpb	$47, -9(%rbp)
	jne	.L12
	cmpl	$1, -8(%rbp)
	jne	.L12
	movl	$0, %eax
	call	single_line_comment_loop
	movl	$0, -8(%rbp)
	jmp	.L14
.L12:
	movl	$1, -8(%rbp)
	jmp	.L14
.L8:
	cmpl	$1, -8(%rbp)
	jne	.L15
	movl	$0, %eax
	call	multi_line_comment_loop
	jmp	.L16
.L15:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
.L16:
	movl	$0, -8(%rbp)
	jmp	.L14
.L10:
	cmpl	$1, -8(%rbp)
	jne	.L17
	movl	$47, %edi
	call	print_cur_char
.L17:
	movl	$0, %eax
	call	string_loop
	movl	$0, -8(%rbp)
	jmp	.L14
.L9:
	cmpl	$1, -8(%rbp)
	jne	.L18
	movl	$47, %edi
	call	print_cur_char
.L18:
	movl	$0, %eax
	call	char_const_loop
	movl	$0, -8(%rbp)
	jmp	.L14
.L11:
	cmpl	$1, -8(%rbp)
	jne	.L19
	movl	$47, %edi
	call	print_cur_char
.L19:
	movl	$0, %eax
	call	check_new_line
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$0, -8(%rbp)
	jmp	.L14
.L5:
	cmpl	$1, -8(%rbp)
	jne	.L20
	movl	$47, %edi
	call	print_cur_char
.L20:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$0, -8(%rbp)
	jmp	.L14
.L25:
	nop
.L14:
	jmp	.L21
.L24:
	nop
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE6:
	.size	main, .-main
	.globl	check_new_line
	.type	check_new_line, @function
check_new_line:
.LFB7:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	line_cur(%rip), %eax
	addl	$1, %eax
	movl	%eax, line_cur(%rip)
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7:
	.size	check_new_line, .-check_new_line
	.globl	print_cur_char
	.type	print_cur_char, @function
print_cur_char:
.LFB8:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	movsbl	-4(%rbp), %eax
	movq	stdout(%rip), %rdx
	movq	%rdx, %rsi
	movl	%eax, %edi
	call	fputc@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE8:
	.size	print_cur_char, .-print_cur_char
	.section	.rodata
	.align 8
.LC0:
	.string	"Error: line %d: unterminated comment\n"
	.text
	.globl	print_error
	.type	print_error, @function
print_error:
.LFB9:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movq	stderr(%rip), %rax
	movl	-4(%rbp), %edx
	leaq	.LC0(%rip), %rcx
	movq	%rcx, %rsi
	movq	%rax, %rdi
	movl	$0, %eax
	call	fprintf@PLT
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE9:
	.size	print_error, .-print_error
	.globl	single_line_comment_loop
	.type	single_line_comment_loop, @function
single_line_comment_loop:
.LFB10:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	$32, %edi
	call	print_cur_char
.L33:
	call	getchar@PLT
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	movb	%al, -5(%rbp)
	cmpb	$10, -5(%rbp)
	jne	.L30
	movl	$0, %eax
	call	check_new_line
	jmp	.L34
.L30:
	cmpl	$-1, -4(%rbp)
	jne	.L33
	movl	line_cur(%rip), %eax
	movl	%eax, %edi
	call	print_error
	movl	$1, %edi
	call	exit@PLT
.L34:
	movl	$10, %edi
	call	print_cur_char
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE10:
	.size	single_line_comment_loop, .-single_line_comment_loop
	.globl	multi_line_comment_loop
	.type	multi_line_comment_loop, @function
multi_line_comment_loop:
.LFB11:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	$0, -12(%rbp)
	movl	line_cur(%rip), %eax
	movl	%eax, -8(%rbp)
	movl	$32, %edi
	call	print_cur_char
.L42:
	call	getchar@PLT
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	movb	%al, -13(%rbp)
	cmpb	$10, -13(%rbp)
	jne	.L36
	movl	$0, %eax
	call	check_new_line
	movsbl	-13(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$0, -12(%rbp)
	jmp	.L42
.L36:
	cmpl	$-1, -4(%rbp)
	jne	.L38
	movl	-8(%rbp), %eax
	movl	%eax, %edi
	call	print_error
	movl	$1, %edi
	call	exit@PLT
.L38:
	cmpb	$42, -13(%rbp)
	jne	.L39
	movl	$1, -12(%rbp)
	jmp	.L42
.L39:
	cmpl	$1, -12(%rbp)
	jne	.L40
	cmpb	$47, -13(%rbp)
	je	.L43
.L40:
	movl	$0, -12(%rbp)
	jmp	.L42
.L43:
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE11:
	.size	multi_line_comment_loop, .-multi_line_comment_loop
	.globl	string_loop
	.type	string_loop, @function
string_loop:
.LFB12:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	$0, -8(%rbp)
	movl	$34, %edi
	call	print_cur_char
.L51:
	call	getchar@PLT
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	movb	%al, -9(%rbp)
	cmpb	$34, -9(%rbp)
	jne	.L45
	cmpl	$1, -8(%rbp)
	je	.L45
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	jmp	.L46
.L45:
	cmpb	$10, -9(%rbp)
	jne	.L47
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$0, -8(%rbp)
	movl	$0, %eax
	call	check_new_line
	jmp	.L51
.L47:
	cmpl	$-1, -4(%rbp)
	je	.L52
	cmpb	$92, -9(%rbp)
	jne	.L50
	cmpl	$1, -8(%rbp)
	je	.L50
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$1, -8(%rbp)
	jmp	.L48
.L50:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$0, -8(%rbp)
.L48:
	jmp	.L51
.L52:
	nop
.L46:
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE12:
	.size	string_loop, .-string_loop
	.globl	char_const_loop
	.type	char_const_loop, @function
char_const_loop:
.LFB13:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	$0, -8(%rbp)
	movl	$39, %edi
	call	print_cur_char
.L60:
	call	getchar@PLT
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	movb	%al, -9(%rbp)
	cmpb	$39, -9(%rbp)
	jne	.L54
	cmpl	$1, -8(%rbp)
	je	.L54
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	jmp	.L55
.L54:
	cmpb	$10, -9(%rbp)
	jne	.L56
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$0, -8(%rbp)
	movl	$0, %eax
	call	check_new_line
	jmp	.L60
.L56:
	cmpl	$-1, -4(%rbp)
	je	.L61
	cmpb	$92, -9(%rbp)
	jne	.L59
	cmpl	$1, -8(%rbp)
	je	.L59
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$1, -8(%rbp)
	jmp	.L57
.L59:
	movsbl	-9(%rbp), %eax
	movl	%eax, %edi
	call	print_cur_char
	movl	$0, -8(%rbp)
.L57:
	jmp	.L60
.L61:
	nop
.L55:
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE13:
	.size	char_const_loop, .-char_const_loop
	.ident	"GCC: (Ubuntu 11.4.0-9ubuntu1) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
