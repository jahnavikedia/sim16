/* fact_recursive.c -- recursive factorial */
#include <stdio.h>
int n = 5;
int result;

int fact(int k) {
    if (k <= 1)
        return 1;
    return k * fact(k - 1);
}

int main(void) {
    result = fact(n);
    printf("%d\n", result);   /* I/O: becomes OUT */
    return 0;
}
