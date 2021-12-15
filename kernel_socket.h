#ifndef __KERNEL_SOCKETS_H
#define __KERNEL_SOCKETS_H

#include "tinyos.h"


typedef enum {
	SOCKET_LISTENER,
	SOCKET_UNBOUND,
	SOCKET_PEER
}	socket_type;

typedef struct listener_socket {
	rlnode queue;
	CondVar req_available;
} listener_socket;

typedef struct unbound_socket {
	rlnode unbound_socket;
} unbound_socket;

typedef struct peer_socket {
	socket_t* peer;
	pipecb_t* write_pipe;
	pipecb_t* read_pipe;
} peer_socket;

typedef struct socket_control_block{
	uint refcount;
	FCB* fcb;
	socket_type type;
	port_t port;

	union{
		listener_socket listener_s;
		unbound_socket unbound_s;
		peer_socket peer_s;
	};

}socket_cb;

//typedef struct port


















#endif