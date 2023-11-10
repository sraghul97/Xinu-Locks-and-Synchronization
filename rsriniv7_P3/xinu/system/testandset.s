/* testandset.S - testandset (for x86) */

		.text
		.globl	test_and_set

/*------------------------------------------------------------------------
 * testandset -  X86 context switch; the call is test_and_set(uint32 *ptr, uint32 new_value);
 *------------------------------------------------------------------------
 */
test_and_set:
		pushl	%ebp		/* Push ebp onto stack		*/
		movl	%esp,%ebp	/* Record current SP in ebp	*/
		pushfl			/* Push flags onto the stack	*/
        pushl %edx      /* Push edx register to stack */      

		movl	8(%ebp),%edx	/* Save ptr in edx register	*/					            
		movl	12(%ebp),%eax	/* Save new_value in eax register	*/
		
        xchg	(%edx),%eax	/* exchange eax and edx. Save ptr in eax (to be returned) and save new_value in edx	*/
		
        popl	%edx       /* Pop edx register from stack  */
        popfl			/* Restore interrupt mask	*/
		popl	%ebp   /* pop ebp from stack */
        ret			/* Return to the calling function	*/
