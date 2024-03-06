#include <stdio.h>
int main() {
	int n;
	scanf("%d", &n);
	int m = 0, x = n;
	while (n) {
		m = m * 10 + n % 10;
		n /= 10;
	}
	if (m == x) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}
