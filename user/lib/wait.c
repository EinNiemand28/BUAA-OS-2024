#include <env.h>
#include <lib.h>
void wait(u_int envid) {
	const volatile struct Env *e;

	e = &envs[ENVX(envid)];
	// debugf("%08x waiting %08x\n", syscall_getenvid(), envid);
	while (e->env_id == envid && e->env_status != ENV_FREE) {
		syscall_yield();
	}
}
