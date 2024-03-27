void scanf_1_check() {
	printk("Running scanf_1_check\n");
	char num = 0;
	char ch = '0';
	scanf("%c%c", &num, &ch);
	printk("Finished 1st scanf\n");
	printk("%c %c\n", ch, num);
}

void mips_init(u_int argc, char **argv, char **penv, u_int ram_low_size) {
	scanf_1_check();
	halt();
}
