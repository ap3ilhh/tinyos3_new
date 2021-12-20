#ifndef __KERNEL_SOCKETS_H
#define __KERNEL_SOCKETS_H

#include "tinyos.h"
#include "kernel_pipe.h"

typedef enum {
	SOCKET_LISTENER,
	SOCKET_UNBOUND,
	SOCKET_PEER
}socket_type;

typedef struct listener_socket {
	rlnode request_queue;
	CondVar req_available_cv;
}listener_socket;

typedef struct unbound_socket {
	rlnode unbound_socket;
}unbound_socket;

typedef struct socket_control_block socket_cb;


typedef struct peer_socket {
	socket_cb* peer;
	pipe_cb* write_pipe;
	pipe_cb* read_pipe;
}peer_socket;


struct socket_control_block{
	uint refcount;
	FCB* fcb;
	socket_type type;
	port_t port;

	union{
		listener_socket listener_s;
		unbound_socket unbound_s;
		peer_socket peer_s;
	};

};
//typedef struct socket_control_block socket_cb;


socket_cb* socket_t;

	

void* socket_open(uint minor);

int socket_read(void* socketcb_t, char *buf, unsigned int n);

int socket_write(void* socketcb_t, const char *buf, unsigned int n);

int socket_close(void* socketcb_t);

socket_cb* PORT_MAP[MAX_PORT];

typedef struct connection_request{
	int admitted;					/*flag=1 to request eksuphrethtai*/
	socket_cb* peer;				/*deixnei sto socket pou exei kanei request*/
	CondVar connected_cv;
	rlnode queue_node;
}connection_request;

#endif