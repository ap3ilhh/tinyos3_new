#include "kernel_socket.h"
#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_pipe.h"

static file_ops socket_file_ops = {
  .Open = socket_open,
  .Read = socket_read,
  .Write = socket_write,
  .Close = socket_close
};

Fid_t sys_Socket(port_t port)
{	
	if (port >= MAX_PORT || port < 0)
		return NOFILE;
	Fid_t fid;
  FCB* fcb;

	if (FCB_reserve(1,&fid,&fcb) == 0){
		return NOFILE;
	}

	socket_cb* socketCB;

	socketCB = (socket_cb*)xmalloc(sizeof(socket_cb));

	socketCB->refcount = 0;			//TOCHECK
	socketCB->fcb = fcb;
	socketCB->type = SOCKET_UNBOUND;
	socketCB->port = port;
	rlnode_new(& socketCB->unbound_s.unbound_socket);

	fcb->streamfunc = & socket_file_ops;
  fcb->streamobj = socketCB;

	return fid;
}

int sys_Listen(Fid_t sock)
{
	FCB* fcb = get_fcb(sock);

	socket_cb* socketCB = fcb->streamobj;
	
	/*  paranomo file id 
		not bound to a port
		to port einai kateilhmmeno
		to socket einai arxikopoihmeno 
	*/	
	if ( (sock <0 || sock >15) || (socketCB->port == NOPORT) || (PORT_MAP[socketCB->port] != NULL) || (socketCB->type == SOCKET_LISTENER) ){
		return -1;
	}

	PORT_MAP[socketCB->port] = socketCB;
	/*kanw to socket listener*/
	socketCB->type = SOCKET_LISTENER;
	/*arxikopoihsh tou head ths queue*/ 
	rlnode_init(& socketCB->listener_s.queue, NULL); 
	/*arxikopoihsh tou condition variable*/
	socketCB->listener_s.req_available = COND_INIT;

	/*if((sock < 0 || sock > 15) || (fcb == NULL) || (fcb->streamobj->port > MAX_PORT) ||
	 (fcb->streamobj.port == NOPORT)){
		return -1;
	}
	int flag = 0;
	for (int i = 0; i < MAX_FILEID; i++)
	{
		if(FT[i]->streamobj.port == fcb->streamobj->port){
			if(FT[i]->streamobj->type == SOCKET_LISTENER){
				flag = 1;
				break;
			}
		}
	}
	if(flag == 1)
		return -1;*/
	return 0;	
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

void* socket_open(uint minor){
	return NULL;
}

void initialize_port_map(){
	for (int i = 1; i < MAX_PORT - 1; i++){
		PORT_MAP[i] = NULL;
	}
}