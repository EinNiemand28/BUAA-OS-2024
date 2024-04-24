#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);
extern void handle_ri(void);
void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
    [10] = handle_ri,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

/* Overview:
 *   The fallback handler when an unknown exception code is encountered.
 *   'genex.S' wraps this function in 'handle_reserved'.
 */
void do_reserved(struct Trapframe *tf) {
	print_tf(tf);
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}

void do_ri(struct Trapframe *tf) {
	u_int epc = tf->cp0_epc;
	Pte *pte;
	page_lookup(curenv->env_pgdir, epc, &pte);
	u_int *inst = (u_int *) (KADDR(PTE_ADDR(*pte)) | (epc & 0xfff));
	// printk("%032b\n", *inst);
	u_int rs = (*inst >> 21) & 0x1f;
	u_int rt = (*inst >> 16) & 0x1f;
	u_int rd = (*inst >> 11) & 0x1f;
	// printk("rs: %d\nrt: %d\nrd: %d\n", rs, rt, rd);
	if (((*inst >> 26) == 0) && ((*inst & 0x3f) == 0x3f)) {
		u_int res = 0;
		int i = 0;
		for (i = 0; i < 32; i+=8) {
			u_int rs_i = tf->regs[rs] & (0xff << i);
			u_int rt_i = tf->regs[rt] & (0xff << i);
			if (rs_i < rt_i) {
				res = res | rt_i;
			} else {
				res = res | rs_i;
			}
		}
		tf->regs[rd] = res;
	} else if (((*inst >> 26) == 0) && ((*inst & 0x3f) == 0x3e)) {
		u_int *p = (u_int *)tf->regs[rs];
		u_int val = *p;
		if (val == tf->regs[rt]) {
			*p = tf->regs[rd];
		}
		tf->regs[rd] = val;
	}
	tf->cp0_epc += 4;
}
