#ifndef		_PROC_H
#define		_PROC_H

#ifdef __cplusplus
 extern "C" {
#endif
#include 	"stm32f4xx_hal.h"
#include	"io.h"
#define		_PROC_BUFFER_SIZE 128

typedef		void *func(void *);
 
typedef	struct {
func					*f;
void					*arg;
char					*name;
unsigned int	t,dt;
} _proc;

void					_wait(int);
extern				_buffer 	*_proc_buf;
void					_proc_list(void),
							*_proc_loop(void);
_proc					*_proc_add(void *,void *,char *,int),
							*_proc_find(void *,void *);
void					_task(const void *);
#ifdef __cplusplus
}
#endif

#endif
