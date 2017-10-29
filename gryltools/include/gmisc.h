#ifndef GMISC_H_INCLUDED
#define GMISC_H_INCLUDED

/*! GMisc: Various useful wrappers and string manipulators.
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GMISC_GETLINE_OK       0
#define GMISC_GETLINE_NO_INPUT 1
#define GMISC_GETLINE_TOO_LONG 2

extern const char* gmisc_whitespaces;

int gmisc_GetLine( const char *prmpt, char *buff, size_t sz, FILE* file );
void gmisc_NullifyStringEnd(char* str, size_t size, const char* delim);
void gmisc_CStringToLower(char* str, size_t size); // If size==0, until '\0' 
void gmisc_strnSubst(char* str, size_t sz, const char* targets, char subst);
void gmisc_strSubst(char* str, const char* targets, char subst);

void gmisc_PrintTimeByFormat(FILE* file, const char* fmt);

#endif // GMISC_H_INCLUDED
