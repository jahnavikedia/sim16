/*
 * text.h -- our own small string helpers (no <string.h> / <ctype.h> allowed)
 *
 * Used by : assembler.c (parsing source lines), main.c (command line),
 *           printer.c (padding labels).
 */
#ifndef TEXT_H
#define TEXT_H

int  text_equal(const char *a, const char *b);
int  text_equal_ignore_case(const char *a, const char *b);
int  text_length(const char *s);
void text_copy(char *dst, const char *src, int size);
int  is_digit(char c);
int  is_alpha(char c);
int  is_space(char c);
char to_upper(char c);
int  parse_number(const char *s, int *value);

#endif
