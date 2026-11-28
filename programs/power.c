/* power.c -- computes base^exponent = 8^2 */
#include <stdio.h>
int base = 8;
int exponent = 2;
int result;

int main(void) {
    int r = 1;
    int i;
    for (i = 0; i < exponent; i++) {
        r = r * base;
    }
    result = r;
    printf("%d\n", result);   /* I/O: becomes OUT */
    return 0;
}
