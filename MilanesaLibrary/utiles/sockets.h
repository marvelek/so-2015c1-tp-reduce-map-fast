/*
 * sockets.h
 *
 *  Created on: 17/6/2015
 *      Author: utnso
 */

#ifndef UTILES_SOCKETS_H_
#define UTILES_SOCKETS_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/****************** FUNCIONES SOCKET. ******************/

/*
 * Crea, vincula y escucha un socket desde un puerto determinado.
 */
int server_socket(uint16_t port);

/*
 * Crea y conecta a una ip:puerto determinado.
 */
int client_socket(char* ip, uint16_t port);

/*
 * Acepta la conexion de un socket.
 */
int accept_connection(int sock_fd);

/*
 * Convierte al socket en no bloqueante
 */
void make_socket_non_blocking(int sfd);

#endif /* UTILES_SOCKETS_H_ */