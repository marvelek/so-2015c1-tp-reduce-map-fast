#ifndef SRC_NODO_H_
#define SRC_NODO_H_

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <utiles/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "paquete.h"
#include <utiles/messages.h>

typedef struct {
	char nombre[80];
	int socket;
	int cantidad_bloques_totales;
	int cantidad_bloques_libres;
	int* bloques;
	char ip[16];
	int puerto;
	pthread_rwlock_t lock;
} t_nodo;

t_nodo* nodo_crear();
void nodo_eliminar(t_nodo* nodo);
void nodo_asignar_estado(t_nodo* nodo, bool estado);
void nodo_set_socket(t_nodo* nodo, int socket);
int nodo_asignar_bloque_disponible(t_nodo* nodo);
bool nodo_lleno(t_nodo* nodo);
bool nodo_con_espacio(t_nodo* nodo);
t_nodo* nodo_deserealizar_socket(t_msg* mensaje, int socket);
char* nodo_serializar_db(t_nodo* nodo, int* bytes_serializados);
t_nodo* nodo_deserealizar_db(char* nodo_serializado);
void log_info_nodo_conectado_aceptado(t_nodo* nodo);
void log_info_nodo_conectado_nuevo(t_nodo* nodo);
void log_info_nodo_desconectado(t_nodo* nodo);
void nodo_set_cantidad_bloques_libres(t_nodo* nodo, int cant_bloques);
void nodo_set_nombre(t_nodo* nodo, char* nombre);

#endif /* SRC_NODO_H_ */
