#include "StdAfx.h"
#include "StringEdit.h"
#include "string.h"

char* TrimRight(char* str, char c)
{
	char *start = str;
	while (*str) str++;
	while (--str != start && *str == c)
		;
	if (*str == c)
	{
		*str = '\0';
	}
	else if (*str != c)
	{
		*(str+1) = '\0';
	}
	return start;
}
char* TrimLeft(char* str, char c)
{
	char* start = str;
	while (*str && *str == c)
		str++;
	if (*str == '\0')
		return start;
	return str;
}
char* Trim(char* str, char c)
{
	return TrimRight(TrimLeft(str, c), c);
}
char* Replace(char* str, char sc, char dc)
{
	char *start = str;
	while (*str)
	{
		if (*str == sc)
			*str = dc;
		str++;
	}
	return start;
}