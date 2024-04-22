#include <asm/asm.h>
#include <stackframe.h>
#include <trap.h>

.macro BUILD_HANDLER exception handler
NESTED(handle_\exception, TF_SIZE + 8, zero)
	move    a0, sp
	addiu   sp, sp, -8
	jal     \handler
	addiu   sp, sp, 8
	j       ret_from_exception
END(handle_\exception)
.endm

.text

FEXPORT(ret_from_exception)
	lw      v0, TF_STATUS(sp)
	mtc0    v0, CP0_STATUS
	lw      v1, TF_LO(sp)
	mtlo    v1
	lw      v0, TF_HI(sp)
	lw      v1, TF_EPC(sp)
	mthi    v0
	mtc0    v1, CP0_EPC
	mfc0	v0, CP0_EPC
	lw      $31, TF_REG31(sp)
	lw      $30, TF_REG30(sp)
	lw      $28, TF_REG28(sp)
	lw      $25, TF_REG25(sp)
	lw      $24, TF_REG24(sp)
	lw      $23, TF_REG23(sp)
	lw      $22, TF_REG22(sp)
	lw      $21, TF_REG21(sp)
	lw      $20, TF_REG20(sp)
	lw      $19, TF_REG19(sp)
	lw      $18, TF_REG18(sp)
	lw      $17, TF_REG17(sp)
	lw      $16, TF_REG16(sp)
	lw      $15, TF_REG15(sp)
	lw      $14, TF_REG14(sp)
	lw      $13, TF_REG13(sp)
	lw      $12, TF_REG12(sp)
	lw      $11, TF_REG11(sp)
	lw      $10, TF_REG10(sp)
	lw      $9, TF_REG9(sp)
	lw      $8, TF_REG8(sp)
	lw      $7, TF_REG7(sp)
	lw      $6, TF_REG6(sp)
	lw      $5, TF_REG5(sp)
	lw      $4, TF_REG4(sp)
	lw      $3, TF_REG3(sp)
	lw      $2, TF_REG2(sp)
	lw      $1, TF_REG1(sp)
	lw      sp, TF_REG29(sp) /* Deallocate stack */
	eret

NESTED(handle_int, TF_SIZE, zero)
	mfc0    t0, CP0_CAUSE
	mfc0    t2, CP0_STATUS
	and     t0, t2
	andi    t1, t0, STATUS_IM7
	bnez    t1, timer_irq
timer_irq:
	li      a0, 0
	j       schedule
END(handle_int)

BUILD_HANDLER tlb do_tlb_refill

#if !defined(LAB) || LAB >= 4
BUILD_HANDLER mod do_tlb_mod
BUILD_HANDLER sys do_syscall
#endif

BUILD_HANDLER reserved do_reserved
