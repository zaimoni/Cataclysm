/* int_probe.c */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define STRING_LITERAL_TO_STDOUT(A) fwrite(A,sizeof(A)-1,1,stdout)

union ptr_char
{
	const char* first;
	char test[sizeof(const char*)];
};

union long_char
{
	signed long first;
	char test[sizeof(signed long)];
};

static void write_to_stdout(unsigned int i)
{
	if (10U<=i)
		{
		unsigned int scan_up = 10U;
		while(scan_up<=i/10U) scan_up *= 10U;
		do	{
			fputc((i/scan_up)%10U+'0',stdout);
			scan_up /= 10U;
			}
		while(1U<scan_up);
		};
	fputc((i%10U)+'0',stdout);
	fflush(stdout);
}

int main()
{
	unsigned int i;
	int null_is_zero = 1;
	union ptr_char test_null_zero;
	union long_char test_int_structure;
	STRING_LITERAL_TO_STDOUT("/* auto_int.h */\n");
	STRING_LITERAL_TO_STDOUT("#ifndef AUTO_INT_H\n");
	STRING_LITERAL_TO_STDOUT("#define AUTO_INT_H 1\n\n");
	STRING_LITERAL_TO_STDOUT("/* base 2 logarithm of x; pretend 0 is 1 */\n");
	STRING_LITERAL_TO_STDOUT("#define INT_LOG2(x) (");
#ifdef ULLONG_MAX
#define MAX_SUFFIX "ULL"
	i = sizeof(unsigned long long)*CHAR_BIT;
#else
#define MAX_SUFFIX "UL"
	i = sizeof(unsigned long)*CHAR_BIT;
#endif
	while(1<= --i)
		{
		STRING_LITERAL_TO_STDOUT("(x)>=(1" MAX_SUFFIX "<<");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(") ? ");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(": ");
		};
	write_to_stdout(i);
	STRING_LITERAL_TO_STDOUT(")\n");

	STRING_LITERAL_TO_STDOUT("#define ULONG_LOG2(x) (");
	i = sizeof(unsigned long)*CHAR_BIT;
	while(1<= --i)
		{
		STRING_LITERAL_TO_STDOUT("(x)>=(1UL<<");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(") ? ");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(": ");
		};
	write_to_stdout(i);
	STRING_LITERAL_TO_STDOUT(")\n");

	STRING_LITERAL_TO_STDOUT("#define USHRT_LOG2(x) (");
	i = sizeof(unsigned short)*CHAR_BIT;
	while(1<= --i)
		{
		STRING_LITERAL_TO_STDOUT("(x)>=(1U<<");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(") ? ");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(": ");
		};
	write_to_stdout(i);
	STRING_LITERAL_TO_STDOUT(")\n");

	STRING_LITERAL_TO_STDOUT("#define UCHAR_LOG2(x) (");
	i = sizeof(unsigned char)*CHAR_BIT;
	while(1<= --i)
		{
		STRING_LITERAL_TO_STDOUT("(x)>=(1U<<");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(") ? ");
		write_to_stdout(i);
		STRING_LITERAL_TO_STDOUT(": ");
		};
	write_to_stdout(i);
	STRING_LITERAL_TO_STDOUT(")\n");

	STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_SIZEOF_SHRT ");
	write_to_stdout(sizeof(unsigned short));
	STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_SIZEOF_INT ");
	write_to_stdout(sizeof(unsigned int));
	STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_SIZEOF_LONG ");
	write_to_stdout(sizeof(unsigned long));
#ifdef ULLONG_MAX
	STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_SIZEOF_LLONG ");
	write_to_stdout(sizeof(unsigned long long));
#endif
	STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_SIZEOF_CHARPTR ");
	write_to_stdout(sizeof(char*));
	STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_SIZEOF_FUNCPTR ");
	write_to_stdout(sizeof(&write_to_stdout));

	/* check whether NULL pointer constant has a bitwise representation of all zeros */
	test_null_zero.first = NULL;
	i = sizeof(char*);
	do	if ('\0'!=test_null_zero.test[--i])
			{
			null_is_zero = 0;
			break;
			}
	while(0<i);
	if (null_is_zero) STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_NULL_REALLY_IS_ZERO 1");

	test_int_structure.first = 1;
	if ('\1'==test_int_structure.test[0])
		{
		STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_LITTLE_ENDIAN 1");
		}
	else if ('\1'==test_int_structure.test[sizeof(signed long)-1])
		{
		STRING_LITERAL_TO_STDOUT("\n#define ZAIMONI_BIG_ENDIAN 1");
		}

	STRING_LITERAL_TO_STDOUT("\n\n#endif\n");
	exit(EXIT_SUCCESS);
}
