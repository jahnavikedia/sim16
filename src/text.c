/*
 * text.c -- our own small string helpers (no <string.h> / <ctype.h> allowed)
 *
 * Purpose : the class rules allow only printf/putchar from the C library,
 *           so comparing, copying and number parsing are written here.
 * Used by : assembler.c, main.c, printer.c (both assignments).
 */
#include "text.h"

/*
 * text_equal: are two strings exactly the same?
 * Returns 1 if equal, 0 if not.
 */
int text_equal(const char *a, const char *b)
{
    int i = 0;
    while (a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    return a[i] == b[i];    /* both ended at the same place */
}

/*
 * text_equal_ignore_case: same as text_equal, but 'add' == 'ADD'.
 * Returns 1 if equal, 0 if not.
 */
int text_equal_ignore_case(const char *a, const char *b)
{
    int i = 0;
    while (a[i] != '\0' && to_upper(a[i]) == to_upper(b[i])) {
        i++;
    }
    return to_upper(a[i]) == to_upper(b[i]);
}

/*
 * text_length: number of characters before the '\0'.
 */
int text_length(const char *s)
{
    int n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

/*
 * text_copy: copy src into dst (dst holds size chars, including '\0').
 * Too-long text is cut off; dst always ends with '\0'.
 */
void text_copy(char *dst, const char *src, int size)
{
    int i = 0;
    while (i < size - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* is_digit: '0'..'9' */
int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/* is_alpha: letter or '_' (the characters a label may start with) */
int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* is_space: space or tab */
int is_space(char c)
{
    return c == ' ' || c == '\t';
}

/* to_upper: 'a'..'z' -> 'A'..'Z', anything else unchanged */
char to_upper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

/*
 * hex_digit_value: '0'..'9', 'a'..'f', 'A'..'F' -> 0..15, else -1.
 */
static int hex_digit_value(char c)
{
    if (is_digit(c)) {
        return c - '0';
    }
    c = to_upper(c);
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/*
 * parse_number: read a decimal or hex number (SPEC 3.9).
 *   "12", "-3", "08"      -> decimal (leading zeros are fine)
 *   "0x1F", "-0x1"        -> hex
 * The whole string must be the number.
 * Returns 0 and sets *value on success, -1 if s is not a number.
 */
int parse_number(const char *s, int *value)
{
    int i = 0;
    int sign = 1;
    int base = 10;
    int result = 0;
    int digits = 0;

    if (s[i] == '-') {                          /* optional minus */
        sign = -1;
        i++;
    }
    if (s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        base = 16;                              /* 0x prefix -> hex */
        i += 2;
    }
    while (s[i] != '\0') {
        int d = (base == 16) ? hex_digit_value(s[i]) : (is_digit(s[i]) ? s[i] - '0' : -1);
        if (d < 0) {
            return -1;                          /* not a digit of this base */
        }
        if (result > 100000) {
            return -1;                          /* far too big for any field */
        }
        result = result * base + d;
        digits++;
        i++;
    }
    if (digits == 0) {
        return -1;                              /* "", "-" or "0x" alone */
    }
    *value = sign * result;
    return 0;
}

/*
 * text_append: add src to the end of dst (dst holds size chars).
 */
void text_append(char *dst, const char *src, int size)
{
    int n = text_length(dst);
    text_copy(dst + n, src, size - n);
}

/*
 * text_append_int: add a decimal number to the end of dst.
 * (Our own replacement for the sprintf function, which is not allowed.)
 */
void text_append_int(char *dst, int value, int size)
{
    char digits[12];
    char tmp[13];
    int n = 0, i = 0;

    if (value < 0) {
        tmp[i] = '-';
        i++;
        value = -value;
    }
    do {                                    /* digits come out last-first */
        digits[n] = (char)('0' + value % 10);
        n++;
        value /= 10;
    } while (value > 0);
    while (n > 0) {                         /* reverse them */
        n--;
        tmp[i] = digits[n];
        i++;
    }
    tmp[i] = '\0';
    text_append(dst, tmp, size);
}
