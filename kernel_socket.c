#include "kernel_socket.h"
#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_pipe.h"
#include "kernel_cc.h"
#include "kernel_proc.h"

static file_ops socket_file_ops = {
  .Open = socket_open,
  .Read = socket_read,
  .Write = socket_write,
  .Close = socket_close
};

static int first_listen = 0;

Fid_t sys_Socket(port_t port)
{	
	if (port > MAX_PORT || port < 0)
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
	/*if (first_listen == 0)
	{
		initialize_port_map();
		first_listen = 1;
	}*/
	

	FCB* fcb = get_fcb(sock);

	if (fcb == NULL)
		return -1;

	socket_cb* socketCB = fcb->streamobj;

	if (socketCB == NULL)
		return -1;
	
	/*  paranomo file id 
		not bound to a port
		to port einai kateilhmmeno
		to socket einai arxikopoihmeno 
	*/	
	if ( (sock <0 || sock >15) || (socketCB->port == NOPORT) || (PORT_MAP[socketCB->port] != NULL) || (socketCB->type == SOCKET_LISTENER) ){
		return -1;
	}
	/*install socket to PORT_MAP[]*/
	PORT_MAP[socketCB->port] = socketCB;
	/*kanw to socket listener*/
	socketCB->type = SOCKET_LISTENER;
	/*arxikopoihsh tou head ths queue*/ 
	rlnode_init(& socketCB->listener_s.request_queue, NULL); 
	/*arxikopoihsh tou condition variable*/
	socketCB->listener_s.req_available_cv = COND_INIT;

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
	FCB* fcb = get_fcb(lsock);

	if (fcb == NULL)
		return NOFILE;

	socket_cb* socketCB = fcb->streamobj;

	if ( (lsock <0 || lsock >15) || socketCB->type != SOCKET_LISTENER){
		return NOFILE;
	}

	/*den exei ginei
	the available file ids for the process are exhausted*/

	PCB* curproc = CURPROC;
	for (int i = 0; i < MAX_FILEID; i++)
	{
		if (curproc->FIDT[i] == NULL)
			break;
		if (MAX_FILEID - 1 == i){
			return -1;
		}
	}

	socketCB->refcount++;

	/*oso h oura einai adeia kai den exei kleisei to port kane kernel_wait*/
	while (is_rlist_empty(& socketCB->listener_s.request_queue) && PORT_MAP[socketCB->port] != NULL){
		kernel_wait(& socketCB->listener_s.req_available_cv ,SCHED_PIPE);
	}
	/*an ekleise to port*/
	if (PORT_MAP[socketCB->port] == NULL)
		return NOFILE;

	/*yparxei request*/

	/*pernw apo th lista to request*/
	rlnode* cli_node = rlist_pop_front(& socketCB->listener_s.request_queue);
	/*pernw ton client*/
	socket_cb* cli_sockCB = cli_node->req->peer;
	/*o client eksuphrethtai*/
	cli_node->req->admitted = 1;

	/*dhmiourgia enos peer socket gia na uparksei epikoinwnia metaksu server-client*/
	Fid_t srv_sock = sys_Socket(socketCB->port);

	if (srv_sock == NOFILE)
		return NOFILE;

	FCB* fcb_srv = get_fcb(srv_sock);

	if (fcb_srv == NULL)
		return NOFILE;

	socket_cb* srv_shockCB = fcb_srv->streamobj;

	/*dhmiourgia 2 pipe_control_block gia thn epikoinwnia twn streams*/
	pipe_cb* pipeCB1 = (pipe_cb*)xmalloc(sizeof(pipe_cb));
	
	FCB *reader_p1, *writer_p1;	

/******************************/

	//initialize pipeCB1
	pipeCB1->reader = reader_p1;
	pipeCB1->writer = writer_p1;
	pipeCB1->has_space = COND_INIT;
	pipeCB1->has_data = COND_INIT;
	pipeCB1->w_position = 0;  
	pipeCB1->r_position = 0; 
	pipeCB1->space_remaining = PIPE_BUFFER_SIZE;


	pipe_cb* pipeCB2 = (pipe_cb*)xmalloc(sizeof(pipe_cb));

	FCB *reader_p2, *writer_p2;

/******************************/

	//initialize pipeCB2
	pipeCB2->reader = reader_p2;
	pipeCB2->writer = writer_p2;
	pipeCB2->has_space = COND_INIT;
	pipeCB2->has_data = COND_INIT;
	pipeCB2->w_position = 0;  
	pipeCB2->r_position = 0; 
	pipeCB2->space_remaining = PIPE_BUFFER_SIZE;

	/*metatroph tou server apo UNBOUND se PEER*/

	srv_shockCB->type = SOCKET_PEER;
	/*o server deixnei ston client*/
	srv_shockCB->peer_s.peer = cli_sockCB;
	srv_shockCB->peer_s.write_pipe = pipeCB1;
	srv_shockCB->peer_s.read_pipe = pipeCB2;

	/*metatroph tou client apo UNBOUND se PEER*/

	cli_sockCB->type = SOCKET_PEER;
	/*o  client  deixnei ston server*/
	cli_sockCB->peer_s.peer = cli_sockCB;
	cli_sockCB->peer_s.write_pipe = pipeCB2;
	cli_sockCB->peer_s.read_pipe = pipeCB1;
	
	/*ksypna auton pou exei kanei request kai perimenei*/
	kernel_signal(& cli_node->req->connected_cv);

	socketCB->refcount--;
	//free();
	return srv_sock;

}

	

int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{	
	//elegxoi gia lathos
	/*asundeto port
		non-listening socket ??????
		illegal port
		to port den exei listener
	*/

	if (PORT_MAP[port] == NULL || port <=0 || port >= MAX_PORT)
		return -1;

	socket_cb* listen_sock = PORT_MAP[port]->fcb->streamobj;

	if (listen_sock->type != SOCKET_LISTENER)
		return -1;	 			

	int time;
	FCB* fcb = get_fcb(sock);

	if (fcb == NULL)
		return -1;

	socket_cb* cli_sockCB = fcb->streamobj;
	connection_request* req = (connection_request*)xmalloc(sizeof(connection_request));

	cli_sockCB->refcount++;

	req->admitted = 0;
	/*autos pou zhtaei to request*/
	req->peer = cli_sockCB;
	req->connected_cv = COND_INIT;
	rlnode_init(& req->queue_node, req);

	/*to request bainei sthn oura tou listener*/
	socket_cb* listener_sock = PORT_MAP[port];
	rlist_push_back(& listener_sock->listener_s.request_queue,& req->queue_node);
	/*ksupanei ton listener*/
	kernel_signal(&listener_sock->listener_s.req_available_cv);
	/*oso den eksuphrethtai to request kane kernel_wait gia timeout xrono*/
	while(req->admitted == 0)
	{
		time = kernel_timedwait(& req->connected_cv, SCHED_PIPE,timeout);
		if (time == 0){
			break;
		}

			
	}

	cli_sockCB->refcount--;

		if (time == 0){
			return -1;
		}

	return 0;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{

	if ( sock <0 || sock >15){
		return -1;
	}

	FCB* fcb = get_fcb(sock);

	if (fcb == NULL)
		return -1;

	socket_cb* socketCB = fcb->streamobj;

	pipe_cb* write_pipe;
	pipe_cb* read_pipe;

	if (socketCB->type == SOCKET_PEER)
	{
		write_pipe = socketCB->peer_s.write_pipe;
		read_pipe = socketCB->peer_s.read_pipe;
	}else
		return -1;

	switch(how)
	{
		case SHUTDOWN_READ:
			pipe_reader_close(read_pipe);
			break;
		case SHUTDOWN_WRITE:
			pipe_writer_close(write_pipe);
			break;
		case SHUTDOWN_BOTH:
			pipe_reader_close(read_pipe);	
			pipe_writer_close(write_pipe);
			break;
	}
	return 0;
}


int socket_read(void* socketcb_t, char *buf, unsigned int n){
	return -1;
}

int socket_write(void* socketcb_t, const char *buf, unsigned int n){
	return -1;
}

int socket_close(void* socketcb_t){

	socket_cb* socketCB = (socket_cb*)socketcb_t;

	if (socketCB == NULL)
		return -1;

	socketCB->fcb = NULL;
	PORT_MAP[socketCB->port] = NULL;
	free(socketCB);


	return 0;

}

void* socket_open(uint minor){
	return NULL;
}

void initialize_port_map(){
	for (int i = 1; i < MAX_PORT - 1; i++){
		PORT_MAP[i] = NULL;
	}
}