#include <lib.h>

void strace_barrier(u_int env_id) {
	int straced_bak = straced;
	straced = 0;
	while (envs[ENVX(env_id)].env_status == ENV_RUNNABLE) {
		syscall_yield();
	}
	straced = straced_bak;
}

void strace_send(int sysno) {
	if (!((SYS_putchar <= sysno && sysno <= SYS_set_tlb_mod_entry) ||
	      (SYS_exofork <= sysno && sysno <= SYS_panic)) ||
	    sysno == SYS_set_trapframe) {
		return;
	}

	// Your code here. (1/2)
	if (straced) {
		int tmp = straced;
		straced = 0;
		int env_id = syscall_getenvid();
		ipc_send(envs[ENVX(env_id)].env_parent_id, sysno, 0, 0);
		syscall_set_env_status(env_id, ENV_NOT_RUNNABLE);
		straced = tmp;
	}
}

void strace_recv() {
	// Your code here. (2/2)
	int r;
	u_int child_id;
	while (1) {
		r = ipc_recv(&child_id, 0, 0);
		strace_barrier(child_id);
		recv_sysno(child_id, r);
		syscall_set_env_status(child_id, ENV_RUNNABLE);
		if (r == SYS_env_destroy) {
			break;
		}
	}
}
