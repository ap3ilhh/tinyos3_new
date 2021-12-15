#ifndef __KERNEL_PIPE_H
#define __KERNEL_PIPE_H

#include "util.h"


#define PIPE_BUFFER_SIZE 8192

int pipe_write(void* pipecb_t, const char *buf, unsigned int n);
int pipe_read(void* pipecb_t, char *buf, unsigned int n);
int pipe_writer_close(void* _pipecb);
int pipe_reader_close(void* _pipecb);
void* null_open(uint minor);
int null_write(void* this, const char* buf, unsigned int size);
int null_read(void* this, char *buf, unsigned int size);



typedef struct pipe_control_block {
  FCB * reader, *writer;         //fd gia grapsimo kai diavasma
  CondVar has_space;              //gia wait kai broadcast
  CondVar has_data;
  int w_position, r_position, space_remaining ;     //pou 8a grafw kai diavazw
  char BUFFER[PIPE_BUFFER_SIZE];
}pipe_cb;



#endif