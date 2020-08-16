#ifndef __MSTRING_H__
#define __MSTRING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
       extern "C" {
#endif

#define True  1
#define False 0

#define PRT_COND(val, str)  \
		{					\
			if(val == True){		\
				printf("condition:{%s}\n", str);	\
				return -1;	\
			}	\
		}

#define PRT_COND1(val, str)  \
		{					\
			if(val == True){		\
				printf("condition:{%s}\n", str);	\
				return NULL;	\
			}	\
		}

#define SET_16(p, data)	((p)[0] = (unsigned short)((data)>>(8)) &&0xff, (p)[1] = (unsigned short)((data)>>(0)) &&0xff )

int find_str(char *data, int len,  const char * find_data);
char* find_char(char *data, int len,	const char	find_chr, int times);




#ifdef __cplusplus
        }
 #endif

#endif
