#include "util.h"
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_cc.h"

//ta vazw edw arxika kai vlepoume pou 8a mpoune
int pipe_write(void* pipecb_t, const char *buf, unsigned int n);
int pipe_read(void* pipecb_t, char *buf, unsigned int n);
int pipe_writer_close(void* _pipecb);
int pipe_reader_close(void* _pipecb);
void* null_open(uint minor);
int null_write(void* this, const char* buf, unsigned int size);
int null_read(void* this, char *buf, unsigned int size);

int count = 0;


void* null_open(uint minor){
	return NULL;
}

int null_write(void* this, const char* buf, unsigned int size){
	return -1;
}

int null_read(void* this, char *buf, unsigned int size){
	return -1;
}

static file_ops reader_file_ops = {
  .Open = null_open,
  .Read = pipe_read,
  .Write = null_write,
  .Close = pipe_reader_close
};

static file_ops writer_file_ops = {
  .Open = null_open,
  .Read = null_read,
  .Write = pipe_write,
  .Close = pipe_writer_close
};


int sys_Pipe(pipe_t* pipe)
{
	//arxikopoiw gia na ta valw sthn FCB_reserve
	Fid_t fid[2];
	FCB* fcb[2] ;

	if (FCB_reserve(2,fid,fcb) == 0){
		return -1;		
	}

	//enhmerwnw thn metavlhth pipe me fid pou desmeuthkan
	pipe->read = fid[0];
	pipe->write = fid[1];

	//desmeuw xwro gia ena Pipe_cb kai to arxikopoiw
	pipe_cb* pipeCB;
	pipeCB = (pipe_cb*)xmalloc(sizeof(pipe_cb));

	//initialize pipeCB
	pipeCB->reader = fcb[0];
	pipeCB->writer = fcb[1];
	pipeCB->has_space = COND_INIT;
	pipeCB->has_data = COND_INIT;
	pipeCB->w_position = 0;  
	pipeCB->r_position = 0; 
	pipeCB->space_remaining = PIPE_BUFFER_SIZE;
	//ta 2 FCB deixnoun sto idio pipe(streamobject)
	fcb[0]->streamobj = pipeCB;
	fcb[1]->streamobj = pipeCB;

	fcb[0]->streamfunc = &reader_file_ops;
	fcb[1]->streamfunc = &writer_file_ops;
	
	return 0;
}


int pipe_write(void* pipecb_t, const char *buf, unsigned int n)
{	
	pipe_cb* pipeCB =(pipe_cb*)pipecb_t;

	int i;

	//oso o buffer einai gematos kai o reader einai anoixtos kane kernel_wait
	while (pipeCB->space_remaining == 0 && pipeCB->reader != NULL ){
		kernel_wait(&pipeCB->has_space,SCHED_PIPE);
	}

	//otan vgei apo to kernel_wait 
	//an vghke giati o reader ekleise epestrepse lathos
	if (pipeCB->reader == NULL)
		return -1;

	for (i = 0; i < pipeCB->space_remaining; i++){
		if (i >= n)
			break;
		pipeCB->BUFFER[pipeCB->w_position] = buf[i];
		pipeCB->w_position = (pipeCB->w_position + 1)%PIPE_BUFFER_SIZE;
	}
	pipeCB->space_remaining -= i; 
	//ksupna osa perimenoun na grapseis 
	kernel_broadcast(&pipeCB->has_data);


	return i;
}



int pipe_read(void* pipecb_t, char *buf, unsigned int n)
{	
	pipe_cb* pipeCB =(pipe_cb*)pipecb_t;

	int i;

	if (pipeCB->writer == NULL){
	  if(pipeCB->space_remaining != PIPE_BUFFER_SIZE){
			for (int i = 0; i < PIPE_BUFFER_SIZE - pipeCB->space_remaining; i++)
			{
				buf[i] = pipeCB->BUFFER[pipeCB->r_position];
				pipeCB->r_position = (pipeCB->r_position + 1)%PIPE_BUFFER_SIZE;
			}
			pipeCB->space_remaining += i;
			return i;
		}
		return 0;
	}

	while (pipeCB->space_remaining == PIPE_BUFFER_SIZE && pipeCB->writer != NULL){
		kernel_wait(&pipeCB->has_data,SCHED_PIPE);
	}

	if (pipeCB->writer == NULL)
		return -1;
	
	for (int i = 0; i < PIPE_BUFFER_SIZE - pipeCB->space_remaining; i++)
	{
		if (i >= n)
			break;
		buf[i] = pipeCB->BUFFER[pipeCB->r_position];
		pipeCB->r_position = (pipeCB->r_position + 1)%PIPE_BUFFER_SIZE;
	}
	pipeCB->space_remaining += i;
	

	kernel_broadcast(&(pipeCB->has_space));

	return i;
}



int pipe_writer_close(void* _pipecb)
{
	pipe_cb* pipeCB =(pipe_cb*)_pipecb;

	pipeCB->writer = NULL;
	return 0;
}


int pipe_reader_close(void* _pipecb)
{
	pipe_cb* pipeCB =(pipe_cb*)_pipecb;

	pipeCB->reader = NULL; 
	if (pipeCB->writer != NULL){
		return -1;
	}
	free(pipeCB);

	return 0;
}


