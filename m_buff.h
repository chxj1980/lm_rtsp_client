#ifndef __M_BUFF_H__
#define __M_BUFF_H__


typedef struct
{
	char *buff;
	unsigned int size;

	unsigned int cur_pot;
	unsigned int off;
}M_buff_t;

int init_buff(M_buff_t *m_buff, int size);
int destroy_buff(M_buff_t *m_buff);

#endif
