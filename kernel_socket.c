#include "kernel_socket.h"
#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_cc.h"
#include "kernel_pipe.h"

static file_ops socket_file_ops = {
  .Open = null_open,
  .Read = socket_read,
  .Write = socket_write,
  .Close = socket_close
};

Fid_t sys_Socket(port_t port)
{	
	if (port > MAX_PORT || port == NOPORT)
		return NOFILE;
	Fid_t fid;
  	FCB* fcb;

	if (FCB_reserve(1,&fid,&fcb) == 0){
		return NOFILE;
	}



	socket_cb* socketCB;

	socketCB = (socket_cb*)xmalloc(sizeof(socket_cb));

	socketCB->refcount = 0;
	socketCB->fcb = fcb;
	socketCB->type = SOCKET_UNBOUND;
	socketCB->port = port;


	fcb->streamfunc = & socket_file_ops;
  	fcb->streamobj = socketCB;

	return fid;
}

int sys_Listen(Fid_t sock)
{
	return -1;
}


Fid_t sys_Accept(Fid_t lsock)
{
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}


int socket_read(void* socketcb_t, char *buf, unsigned int n){
	return -1;
}

int socket_write(void* socketcb_t, const char *buf, unsigned int n){
	return -1;
}

int socket_close(void* socketcb_t){
	return -1;
}

void* null_open(uint minor){
	return NULL;
}

void initialize_port_map(){
	for (int i = 1; i < MAX_PORT - 1; i++){
		PORT_MAP[i] = NULL;
	}
}