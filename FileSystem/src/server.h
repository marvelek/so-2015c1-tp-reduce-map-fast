/*
 * server.h
 *
 *  Created on: 18/5/2015
 *      Author: utnso
 */

#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

struct arg_struct {
	char* puerto_listen;
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <errno.h>
#include "socket.h"
#include "nodo.h"
#include "filesystem.h"
#include <utiles/messages.h>
#include <utiles/sockets.h>


#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

struct arg_get_bloque {
	int socket;
	int bloque_nodo;
	char* nombre_nodo;
};

struct arg_set_bloque {
	int socket;
	int bloque_nodo;
	char* nombre_nodo;
	char* chunk;
};

void iniciar_server(void* argumentos);
//void decodificar_mensaje(t_mensaje* mensaje, int cant_bytes, int socket);
void manejar_desconexion(int socket);
void decodificar_mensaje(t_msg* mensaje, int socket);
char* mensaje_get_bloque(void* argumentos);

#endif /* SRC_SERVER_H_ */