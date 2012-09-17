/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */

/* Changes done post-static-analysis by Matthew Iselin */

#include <stdarg.h>
#include <util.h>
#include <string.h>
#include <types.h>
#include <test.h>

/* we use this so that we can do without the ctype library */
#define is_digit(c)	((c) >= '0' && (c) <= '9')

static size_t skip_atoi(const char **s)
{
	size_t i=0;

	while (is_digit(**s))
		i = i * 10 + (size_t) (*((*s)++) - '0');
	return i;
}

#define ZEROPAD	1UL		/* pad with zero */
#define SIGN	2UL		/* unsigned/signed long */
#define PLUS	4UL		/* show plus */
#define SPACE	8UL		/* space if plus */
#define LEFT	16UL		/* left justified */
#define SPECIAL	32UL		/* 0x */
#define SMALL	64UL		/* use 'abcdef' instead of 'ABCDEF' */

static char * number(char * str, uintmax_t num, unsigned char base, size_t size, size_t precision, size_t type)
{
	char c,sign,tmp[36];
	const char *digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	size_t i = 0;

	if (type&SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz";
	if (type&LEFT) type &= ~ZEROPAD;
	if (base<2 || base>36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ' ;

	// Handle zero sentinels
    if(precision == (size_t) ~0)
        precision = 0;
    if(size == (size_t) ~0)
        size = 0;

	// As we now take a uint (to avoid signed division for conversion to string), the sign
	// check involves a cast to signed... -Matt
	if (((type & SIGN) > 0) && (((intmax_t) num) < 0)) {
		sign='-';
		num = -num;
	} else
		sign=(type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);
	if (sign && (size > 0)) size--;
	if (type&SPECIAL){
		if((base == 16) && (size >= 2)) size -= 2;
		else if((base == 8) && (size > 0)) size--;
	}
	if (num == 0)
		tmp[i++]='0';
	else {
		while(num != 0) {
			tmp[i++]=digits[num % base];
			num /= base;
		}
	}
	if (i > precision) precision = i;
	if(size > precision)
		size -= precision;
	else
		size = 0;
	if ((type & (ZEROPAD + LEFT)) == 0) {
		while(size > 0) {
			*str++ = ' ';
			size--;
		}
	}
	if (sign)
		*str++ = sign;
	if (type&SPECIAL) {
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}
	if ((type&LEFT) == 0) {
		while(size > 0) {
			*str++ = c;
			size--;
		}
	}
	while((i < precision) && (precision > 0)) {
		*str++ = '0';
		precision--;
	}
	while(i > 0) {
		*str++ = tmp[--i];
	}
	while(size > 0) {
		*str++ = ' ';
		size--;
	}
	return str;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	size_t len;
	size_t i;
	char *str;
	char *s;
	int *ip;

	size_t flags;		/* flags to number() */

	size_t field_width;	/* width of output field */
	size_t precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	char qualifier;		/* 'h', 'l', or 'L' for integer fields */
	char qualifier2; /* 'l' if 'll' */

	for (str=buf ; *fmt ; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
				}

		/* get field width */
		field_width = (size_t) ~0;
		if (is_digit(*fmt))
			field_width = (size_t) skip_atoi(&fmt);
		else if (*fmt == '*') {
			/* it's the next argument */
			field_width = va_arg(args, size_t);
			if (((ssize_t) field_width) < 0) {
				field_width = (size_t) (-((ssize_t) field_width));
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = (size_t) ~0;
		if (*fmt == '.') {
			++fmt;
			if (is_digit(*fmt))
				precision = (size_t) skip_atoi(&fmt);
			else if (*fmt == '*') {
				/* it's the next argument */
				precision = va_arg(args, size_t);
			}
			if (((ssize_t) precision) < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}
		if(*fmt == 'l' && qualifier == 'l') {
			qualifier2 = *fmt;
			++fmt;
		}

		switch (*fmt) {
		case 'c':
            if (((flags & LEFT) == 0) && (field_width != (size_t) ~0))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (char) va_arg(args, unative_t);
            if(field_width != (size_t) ~0)
				while (--field_width > 0)
					*str++ = ' ';
			break;

		case 's':
			s = va_arg(args, char *);
			len = strlen(s);
			if (precision == (size_t) ~0)
				precision = len;
			else if (len > precision)
				len = precision;

            if (((flags & LEFT) == 0) && (field_width != (size_t) ~0))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
            if(field_width && (field_width != (size_t) ~0))
				while (len < field_width--)
					*str++ = ' ';
			break;

		case 'o':
			if(qualifier == 'l' && qualifier2 == 'l')
				str = number(str, va_arg(args, unsigned long long), 8,
							field_width, precision, flags);
			else if(qualifier == 'l')
				str = number(str, va_arg(args, unsigned long), 8,
							field_width, precision, flags);
			else
				str = number(str, va_arg(args, unsigned int), 8,
							field_width, precision, flags);
			break;

		case 'p':
			if (field_width == (size_t) ~0) {
				field_width = 8;
				flags |= ZEROPAD;
			}
			str = number(str, (uintptr_t) va_arg(args, void *), 16,
				field_width, precision, flags);
			break;

		case 'x':
			flags |= SMALL;
			/*@fallthrough@*/
		case 'X':
			if(qualifier == 'l' && qualifier2 == 'l')
				str = number(str, va_arg(args, unsigned long long), 16,
							field_width, precision, flags);
			else if(qualifier == 'l')
				str = number(str, va_arg(args, unsigned long), 16,
							field_width, precision, flags);
			else
				str = number(str, va_arg(args, unsigned int), 16,
							field_width, precision, flags);
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
			/*@fallthrough@*/
		case 'u':
			if(qualifier == 'l' && qualifier2 == 'l')
				str = number(str, va_arg(args, unsigned long long), 10,
							field_width, precision, flags);
			else if(qualifier == 'l')
				str = number(str, va_arg(args, unsigned long), 10,
							field_width, precision, flags);
			else
				str = number(str, va_arg(args, unsigned int), 10,
							field_width, precision, flags);
			break;

		case 'n':
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;

		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			break;
		}
	}
	*str = '\0';
	return str-buf;
}

DEFINE_TEST(number_32, ORDER_PRIMARY, 0,
			TEST_INIT_VAR(char, buf[32], {0}),
			number(buf, 0xF00DUL, 16, 4, 0, 0),
			strcmp(buf, "F00D"))

DEFINE_TEST(number_64, ORDER_PRIMARY, 0,
			TEST_INIT_VAR(char, buf[32], {0}),
			number(buf, 0xDEADBEEF15F00DULL, 16, 8, 0, 0),
			strcmp(buf, "DEADBEEF15F00D"))
