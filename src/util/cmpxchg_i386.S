/*
 * Return is the new value of the dest location
 */
	.text
	.align 4
	.globl _cmpxchg_64
_cmpxchg_64:
        .globl cmpxchg_64
cmpxchg_64:
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%edi

	/* dest location */
	movl	8(%ebp), %edi

	/* old value */
	movl	16(%ebp), %edx	# old high
	movl	12(%ebp), %eax	# old low

	/* new value */
	movl	24(%ebp), %ecx	# new high
	movl	20(%ebp), %ebx	# new low

	/* do the exchange
	 * if the exchange succeeds, %edx:%eax will be the old value */
	lock; cmpxchg8b (%edi)

	/* return true/false whether or not the exchange succeeded */
	xorl	%eax, %eax
	setzb	%al

	popl	%edi
	popl	%ebx

	popl	%ebp
	ret
