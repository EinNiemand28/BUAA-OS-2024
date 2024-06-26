#include <env.h>
#include <lib.h>
#include <mmu.h>

const volatile struct Env *env;

void exit(int r) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif
	env = &envs[ENVX(syscall_getenvid())];
	// if (envs[ENVX(env->env_parent_id)].env_ipc_recving != 0) {
	// 	//debugf("%d should send: %d\n", env->env_id, r);
	// 	ipc_send(env->env_parent_id, r, 0, 0);
	// }
	syscall_env_destroy(0);
	user_panic("unreachable code");
}

extern int main(int, char **);

void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	env = &envs[ENVX(syscall_getenvid())];

	// call user main routine
	//main(argc, argv);
	int r = main(argc, argv);
	//debugf("%d return: %d\n", env->env_id, r);

	// exit gracefully
	exit(r);
}
