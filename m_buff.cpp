#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m_buff.h"
#include "Mstring.h"

int init_buff(M_buff_t *m_buff, int size)
{
	PRT_COND(m_buff->size > 0 || m_buff->buff != NULL, \
			"[init_buff]: Input [m_buff->buff] is not NULL,or [m_buff->size] > 0, exit !");
	
    memset(m_buff, 0, sizeof(M_buff_t));

	m_buff->buff = (char *) malloc(size);
	PRT_COND(m_buff->buff == NULL, "[init_buff]: Input [m_buff->buff] malloc failed , exit !");
	
	m_buff->size = size;
	return size;
}

int extend_buff()
{

	return 0;
}

int destroy_buff(M_buff_t *m_buff)
{
	if( (m_buff->size > 0) || (m_buff->buff != NULL) )
	{
		free(m_buff->buff);
		m_buff->buff = NULL;
		memset(m_buff, 0, sizeof(M_buff_t));
	}
	
	return 0;
}
