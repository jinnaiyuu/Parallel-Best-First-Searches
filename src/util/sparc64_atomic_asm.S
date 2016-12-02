/*
 * Courtesy of some random webpage.
 * int compare_and_swap(volatile uint32_t *addr, uint32_t *o, uint32_t n);
 * Returns true or false, if true the swap succeeded, if false it didn't and *o will contain the
 * value that is stored in *addr currently.
 */
	.text
	.align 4
	
	.globl compare_and_swap
compare_and_swap:
      ld    [%o1],%g1       ! set the old value
      cas   [%o0],%g1,%o2   ! try the CAS
      cmp   %g1,%o2
      be,a  true
      mov   1,%o0           ! return TRUE/1
      mov   0,%o0           ! return FALSE/0
      st    %o2,[%o1]       ! store existing value in memory
true: retl
      nop
