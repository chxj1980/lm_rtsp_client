#include "Mstring.h"


int find_str(char *data, int len,  const char * find_data)
{
	PRT_COND(data == NULL || find_data == NULL, "Input data is NULL, exit !");
	PRT_COND(len <= 0, "Input len is <= 0, exit !");	
	
	int i = 0;
	int j = 0;
	
	const char *p = find_data;
	while(i < len)
	{
		if(data[i] == p[j])
		{
			if(j == strlen(find_data) - 1)
			{
				if(strstr(&data[i - j], find_data ))
				{
					//printf("Find success. exit\n");
					return i; //off_size.
				}

				j = 0;	
			}
			j++;
		}

		i++;
	}

	return -1;
}


char * find_char(char *data, int len,  const char  find_chr,  int times)
{
	if(data == NULL  ||  (len <= 0))
	{
		printf("Input data is NULL, or len is <= 0, exit !\n");
		return NULL;
	}

	char *p = NULL;
	int j = 1;
	int i = 0;
	while(i < len)
	{
		if(p = (char *)memchr(data + i, find_chr, 1))
		{
			if(j == times)
			{
				return p;
			}

			j++;
		}	

		i++;
	}
	
	return NULL;
}

