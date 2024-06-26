#include <env.h>
#include <lib.h>
int wait(u_int envid) {
	const volatile struct Env *e;
	int r;
	u_int who;
	e = &envs[ENVX(envid)];
	// debugf("%08x waiting %08x\n", syscall_getenvid(), envid);
	while (e->env_id == envid && e->env_status != ENV_FREE) {
		r = ipc_recv(&who, 0, 0);
		syscall_yield();
	}
	return r;
}
