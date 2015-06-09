/*
 * Nodo.c
 *
 *  Created on: 25/4/2015
 *      Author: utnso
 */
#include "Nodo.h"

int main() {


	Log_Nodo = log_create(LOG_FILE, PROCESO, 1, LOG_LEVEL_TRACE);

	if (levantarConfiguracionNodo()) {
		log_error(Log_Nodo, "Hubo errores en la carga de las configuraciones.");
	}

	if (levantarHiloFile()) {
		log_error(Log_Nodo, "Conexion con File System fallida.");
	}

	if (levantarServer()) {
		log_error(Log_Nodo, "Error al levantar el Server.");
	}

	return 0;
}

int levantarConfiguracionNodo() {
 char* aux;
	t_config* archivo_config = config_create(PATH);

	PUERTO_FS = config_get_int_value(archivo_config, "PUERTO_FS");
	IP_FS = strdup(config_get_string_value(archivo_config, "IP_FS"));
	ARCHIVO_BIN = strdup(
			config_get_string_value(archivo_config, "ARCHIVO_BIN"));
	DIR_TEMP = strdup(config_get_string_value(archivo_config, "DIR_TEMP"));
	aux = strdup(config_get_string_value(archivo_config, "NODO_NUEVO"));
	IP_NODO = strdup(config_get_string_value(archivo_config, "IP_NODO"));
	PUERTO_NODO = config_get_int_value(archivo_config, "PUERTO_NODO");
    NODO_ID= PUERTO_FS = config_get_int_value(archivo_config, "NODO_ID");
	config_destroy(archivo_config);

	if (strncmp(aux,"SI",2)==0)
		 { NODO_NUEVO= 1;
				  }
	   else {NODO_NUEVO= 0;
				  }

	//mapeo el disco en memoria
	_data = file_get_mapped(DISCO);
	return 0;
}

int levantarHiloFile() {
	pthread_t thr_Conexion_FileSystem;
	t_conexion_nodo* reg_conexion = malloc(sizeof(t_conexion_nodo));

	reg_conexion->sock_fs = obtener_socket();
	rcx = pthread_create(&thr_Conexion_FileSystem, NULL,
			(void *) conectarFileSystem, reg_conexion);
	if (rcx != 0) {
		log_error(Log_Nodo,
				"El thread que acepta las conexiones entrantes no pudo ser creado.");
	}
	return 0;
}

void conectarFileSystem(t_conexion_nodo* reg_conexion) {
	conectar_socket(PUERTO_FS, IP_FS, reg_conexion->sock_fs);

	/*asegurarse de pasarle bien los parametros, puede recibir char*?*/
 t_msg* mensaje= argv_message(INFO_NODO,2,NODO_NUEVO,NODO_ID);
 enviar_mensaje(reg_conexion->sock_fs,mensaje);
 destroy_message(mensaje);

	t_msg*codigo = recibir_mensaje(reg_conexion->sock_fs);
	char*bloque= NULL ;
	switch (codigo->header.id) {

	case GET_BLOQUE:
		//getbloque(bloque2,reg_conexion.sock_fs);
		/*											getBloque(numero) devovera el contenido del bloque "20*numero"
		 almacenado en el espacio de datos.
		 contenidoDeBloque getBloque(unNumero);
		 */

		bloque = getBloque(codigo->argv[0]);
		t_msg* mensaje = argv_message(GET_BLOQUE, 2, codigo->argv[0],
				tamanio_bloque);
		mensaje->stream = bloque;
		enviar_mensaje(reg_conexion->sock_fs, mensaje);
		destroy_message(mensaje);
		break;

	case SET_BLOQUE:
		/*
		 setBloque almacenara los "datos" en "20*numero"
		 setBloque(numero,datos);
		 */

		bloque= malloc(tamanio_bloque);
		memcpy(bloque, codigo->stream, codigo->argv[1]);//1 es el tamaño real, el stream es el bloque de 20mb(aprox)
		memset(bloque + codigo->argv[1], '\0',
				tamanio_bloque - codigo->argv[1]);
		setBloque(codigo->argv[0], bloque);
		free_null((void*)&bloque);

		destroy_message(codigo);
		break;
	case GET_FILE_CONTENT:
		printf("getFileContent()");
		/*
		 arch getFileContent(char* nombre) devolvera el contenido
		 * del archivo "nombre.dat" almacenado en el espacio temporal
		 getFileContent(nombre);
		 */break;
	}

}



int levantarServer() {

	fd_set read_fs; // descriptores q estan lisots para leer
	fd_set master; //descriptores q q estan actualemnte conectados
	size_t tamanio; // hace positivo a la variable
	int socketEscucha, socketNuevaConexion;
	int nbytesRecibidos;
	int max;
	struct sockaddr_in cliente;

	FD_ZERO(&master);
	FD_ZERO(&read_fs);
	char *buffer = malloc(100 * sizeof(char));
	socketEscucha = obtener_socket();
	vincular_socket(socketEscucha, PUERTO_NODO);
	if (listen(socketEscucha, 10) != 0) {
		perror("Error al poner a escuchar socket");
		return EXIT_FAILURE;
	}

	printf("Escuchando conexiones entrantes.\n");
// Escuchar nuevas conexiones entrantes.
//arranca select

	FD_SET(socketEscucha, &master);
	max = socketEscucha;
	printf("%d \n", socketEscucha);

	while (1) {

		memcpy(&read_fs, &master, sizeof(master));
		int dev_select;
		if ((dev_select = select(max + 1, &read_fs, NULL, NULL, NULL)) == -1) {
			perror("select");

		}
		//printf("select = %d \n",dev_select);
		int i;
		for (i = 0; i <= max; i++) //max : cantidad max de sockets
				{
			if (FD_ISSET(i, &read_fs)) {
				//  printf("i = %d \n max = %d \n",i,max);
				if (i == socketEscucha) {
					// pasar a una funcion generica aceptar();
					tamanio = sizeof(struct sockaddr_in);
					if ((socketNuevaConexion = accept(socketEscucha,
							(struct sockaddr*) &cliente, &tamanio)) < 0) {
						perror("Error al aceptar conexion entrante");
						return EXIT_FAILURE;
					} else {
						if (socketNuevaConexion > max) {
							max = socketNuevaConexion;
						}
						FD_SET(socketNuevaConexion, &master);
						//printf("nueva conexion de %s desde socket %d \n",inet_ntoa(cliente.sin_addr), socketNuevaConexion);
					} //if del accept. Recibir hasta BUFF_SIZE datos y almacenarlos en 'buffer'.
				} else {
//
//					verifica si esta en el cojunto de listos para leer
//					pasarlo a una funcion generica
					if ((nbytesRecibidos = recv(i, buffer, BUFF_SIZE, 0)) > 0) {
						int offset = 0, tmp_size = 0, code;
						memcpy(&code, buffer + offset, tmp_size = sizeof(code));
						offset += tmp_size;

						switch (code) {
						case '1':
							printf("getBloque()");
							/*											getBloque(numero) devovera el contenido del bloque "20*numero"
							 almacenado en el espacio de datos.
							 contenidoDeBloque getBloque(unNumero);
							 */break;
						case '2':
							printf("setBloque()");
							/*
							 setBloque almacenara los "datos" en "20*numero"
							 setBloque(numero,datos);
							 */break;
						case '3':
							printf("getFileContent()");
							/*
							 arch getFileContent(char* nombre) devolvera el contenido
							 * del archivo "nombre.dat" almacenado en el espacio temporal
							 getFileContent(nombre);
							 */break;
						case '4':
							printf("ejecutar_mapping()");
							/*
							 ejecutar_mapping(ejecutable,num_bloque,nombre_archivo);
							 */
							break;
						case '5':
							printf("ejecutar_Reduce()");
							/*
							 ejecutar_reduce(ejecutable,lista_archivos,nombre_archivo_tmp);
							 */
							break;
							printf("Mensaje recibido de socket %d: ", i);
							fwrite(buffer, 1, nbytesRecibidos, stdout);
							printf("\n");
							printf("Tamanio del buffer %d bytes!\n",
									nbytesRecibidos);
							fflush(stdout);

						}
					} else if (nbytesRecibidos == 0) {
						printf("se desconecto el socket %d \n", i);
						FD_CLR(i, &master);
						// aca se tendria q actualizar los maximos.

					} else {
						printf("Error al recibir datos\n i= %d\n", i);
						break;

					}
				}						//1er if
			}						// for
		}

	}
	close(socketEscucha);
	return EXIT_SUCCESS;

}
//void getBloque(t_bloque* bloque, int sock){
//
//
//
//
//
//}
void setBloque(int numeroBloque, char* bloque_datos){
//debo pararme en la posicion donde se encuentra almacenado el bloque y empezar a grabar

	log_info(Log_Nodo, "Inicio setBloque(%d)", numeroBloque);

    //el memset lo hago para limpiar el bloque por las dudas
	memset(_data + (numeroBloque * tamanio_bloque), 0, tamanio_bloque);

	memcpy(_data + (numeroBloque * tamanio_bloque), bloque_datos, tamanio_bloque);

	log_info(Log_Nodo, "Fin setBloque(%d)", numeroBloque);

}
char* getBloque(int numeroBloque) {
	log_info(Log_Nodo, "Ini getBloque(%d)", numeroBloque);
	char* bloque = NULL;
	bloque = malloc(tamanio_bloque);
	memcpy(bloque, &(_data[numeroBloque * tamanio_bloque]), tamanio_bloque);
	//memcpy(bloque, _bloques[numero], TAMANIO_BLOQUE);
	log_info(Log_Nodo, "Fin getBloque(%d)", numeroBloque);
	return bloque;
}
