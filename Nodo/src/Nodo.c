
#include "Nodo.h"

int main(int argc, char*parametros[]) {

	log_crear("INFO", LOG_FILE, PROCESO);

	if (levantarConfiguracionNodo()) {
		log_error_consola("Hubo errores en la carga de las configuraciones.");
	}

	_data = crear_Espacio_Datos(NODO_NUEVO, ARCHIVO_BIN, parametros[0]);
	archivos_temporales = list_create();

	if (levantarHiloFile()) {
		log_error_consola("Conexion con File System fallida.");
	}

	levantarNuevoServer();

	liberar_Espacio_datos(_data, ARCHIVO_BIN);

	return 0;
}

int levantarConfiguracionNodo() {
	char* aux;
	t_config* archivo_config = config_create(PATH_CONFIG);

	PUERTO_FS = config_get_int_value(archivo_config, "PUERTO_FS");
	IP_FS = strdup(config_get_string_value(archivo_config, "IP_FS"));
	ARCHIVO_BIN = strdup(
			config_get_string_value(archivo_config, "ARCHIVO_BIN"));
	DIR_TEMP = strdup(config_get_string_value(archivo_config, "DIR_TEMP"));
	aux = strdup(config_get_string_value(archivo_config, "NODO_NUEVO"));
	PUERTO_NODO = config_get_int_value(archivo_config, "PUERTO_NODO");
	NOMBRE_NODO = strdup(
			config_get_string_value(archivo_config, "NOMBRE_NODO"));
	config_destroy(archivo_config);

	if (strncmp(aux, "SI", 2) == 0) {
		NODO_NUEVO = 1;
	} else {
		NODO_NUEVO = 0;
	}

	return 0;
}

int levantarHiloFile() {
	pthread_t thr_Conexion_FileSystem;
	t_conexion_nodo* reg_conexion = malloc(sizeof(t_conexion_nodo));
	rcx = pthread_create(&thr_Conexion_FileSystem, NULL,
			(void *) conectarFileSystem, reg_conexion);
	if (rcx != 0) {
		log_error_consola(
				"El thread que acepta las conexiones entrantes no pudo ser creado.");
	}
	return 0;
}

void conectarFileSystem(t_conexion_nodo* reg_conexion) {

	reg_conexion->sock_fs = client_socket(IP_FS, PUERTO_FS);
	t_msg* mensaje = string_message(CONEXION_NODO, NOMBRE_NODO,1, CANT_BLOQUES);
	enviar_mensaje(reg_conexion->sock_fs, mensaje);
	destroy_message(mensaje);
	log_info_interno("Conectado al File System en el socket %d",reg_conexion->sock_fs);

	while (true) {

		t_msg*codigo = recibir_mensaje(reg_conexion->sock_fs);
		char*bloque = NULL;
		t_msg* mensaje2;

		switch (codigo->header.id) {

		case GET_BLOQUE:
			//getbloque(bloque2,reg_conexion.sock_fs);
			/*											getBloque(numero) devovera el contenido del bloque "20*numero"
			 almacenado en el espacio de datos.
			 contenidoDeBloque getBloque(unNumero);
			 */
		    bloque = getBloque(codigo->argv[0]);
			mensaje2 = string_message(GET_BLOQUE, bloque, 2, codigo->argv[0],
					tamanio_bloque);
			enviar_mensaje(reg_conexion->sock_fs, mensaje2);
			free(bloque);
			destroy_message(mensaje2);
			destroy_message(codigo);
			break;

		case SET_BLOQUE:
			/*
			 setBloque almacenara los "datos" en "20*numero"
			 setBloque(numero,datos);
			 */
			bloque = malloc(tamanio_bloque);
			memcpy(bloque, codigo->stream, codigo->argv[1]); //1 es el tamaño real, el stream es el bloque de 20mb(aprox)
			memset(bloque + codigo->argv[1], '\0',
					tamanio_bloque - codigo->argv[1]);
			setBloque(codigo->argv[0], bloque);
			free(bloque);
			destroy_message(codigo);
			break;

		case GET_FILE_CONTENT:
			/*
			 arch getFileContent(char* nombre) devolvera el contenido
			 * del archivo "nombre.dat" almacenado en el espacio temporal
			 getFileContent(nombre);
			 */
			bloque = getFileContent(codigo->stream);
			mensaje2 = id_message(GET_FILE_CONTENT);
			mensaje2->stream = bloque;
			enviar_mensaje(reg_conexion->sock_fs, mensaje2);
			destroy_message(mensaje2);
			destroy_message(codigo);
			free(bloque);
			break;
		}

	}
}

void levantarNuevoServer() {
	pthread_t thread;
	int listener, nuevaConexion;
	listener = server_socket(PUERTO_NODO);
	printf("Esperando conexiones entrantes\n");
	while (true) {
		nuevaConexion = accept_connection(listener);
		log_info_consola("Se ha conectado un proceso al socket %d",nuevaConexion);
		log_info_interno("Se ha conectado un proceso al socket %d",nuevaConexion);
		pthread_create(&thread, NULL, atenderConexiones, &nuevaConexion);
	}
}

void *atenderConexiones(void *parametro) {
	t_msg *codigo;
	int sock_conexion = *((int *) parametro);
	char*bloque = NULL;
	t_msg* mensaje2;
	t_queue *cola_nodos=queue_create();
	t_msg_id fin;
	t_nodo_archivo* nodo_arch;
	char*rutina_reduce;


	while (1) {
		if ((codigo = recibir_mensaje(sock_conexion)) != NULL ) {

			switch (codigo->header.id) {

			case GET_BLOQUE:
				//getbloque(bloque2,reg_conexion.sock_fs);
				/*											getBloque(numero) devovera el contenido del bloque "20*numero"
				 almacenado en el espacio de datos.
				 contenidoDeBloque getBloque(unNumero);
				 */
				bloque = getBloque(codigo->argv[0]);
				mensaje2 = string_message(GET_BLOQUE, bloque, 2,
						codigo->argv[0], tamanio_bloque);
				enviar_mensaje(sock_conexion, mensaje2);
				free(bloque);
				destroy_message(mensaje2);
				destroy_message(codigo);
				break;

			case SET_BLOQUE:
				/*
				 setBloque almacenara los "datos" en "20*numero"
				 setBloque(numero,datos);
				 */
				bloque = malloc(tamanio_bloque);
				memcpy(bloque, codigo->stream, codigo->argv[1]); //1 es el tamaño real, el stream es el bloque de 20mb(aprox)
				memset(bloque + codigo->argv[1], '\0',
						tamanio_bloque - codigo->argv[1]);
				setBloque(codigo->argv[0], bloque);
				free(bloque);
				destroy_message(codigo);
				break;

			case GET_FILE_CONTENT:
				/*
				 arch getFileContent(char* nombre) devolvera el contenido
				 * del archivo "nombre.dat" almacenado en el espacio temporal
				 getFileContent(nombre);
				 */
				bloque = getFileContent(codigo->stream);

				mensaje2 = id_message(GET_FILE_CONTENT);
				mensaje2->stream = bloque;

				enviar_mensaje(sock_conexion, mensaje2);
				destroy_message(mensaje2);
				destroy_message(codigo);
				free(bloque);
				break;

			case EJECUTAR_MAP:

				mensaje2 = recibir_mensaje(sock_conexion);

				fin = ejecutar_map(mensaje2->stream, codigo->stream,codigo->argv[0],mensaje2->argv[0]);
				switch (fin) {
				case FIN_MAP_ERROR:
					log_error_consola("Hubo errores en el map %d en el bloque %d.",
							mensaje2->argv[0], codigo->argv[0]);
					log_error_interno("Hubo errores en el map %d en el bloque %d.",
							mensaje2->argv[0], codigo->argv[0]);
					break;
				case FIN_MAP_OK:
					log_info_consola("Map %d en el bloque %d exitoso",
							mensaje2->argv[0], codigo->argv[0]);
					log_info_interno("Map %d en el bloque %d exitoso",
							mensaje2->argv[0], codigo->argv[0]);
					break;

				}
				mensaje2 = id_message(fin);
				enviar_mensaje(sock_conexion, mensaje2);
				destroy_message(mensaje2);
				destroy_message(codigo);
				break;

			case EJECUTAR_REDUCE:
//

							//Se recibe la rutina
							mensaje2 = recibir_mensaje(sock_conexion);
							size_t tamanioEjecutable=strlen(mensaje2->stream);
							rutina_reduce = malloc(tamanioEjecutable);
							memcpy(rutina_reduce, mensaje2->stream, tamanioEjecutable);
							destroy_message(mensaje2);

							//el primero va a ser el nodo local
							while(mensaje2->header.id!=FIN_ENVIO_MENSAJE){
								mensaje2 = recibir_mensaje(sock_conexion);
								nodo_arch=(t_nodo_archivo*) malloc(sizeof(t_nodo_archivo));
								nodo_arch->ip =string_n_split(mensaje2->stream,2,"|")[0];
								nodo_arch->archivo = string_n_split(mensaje2->stream,2,"|")[1];
								nodo_arch->puerto= mensaje2->argv[0];
								queue_push(cola_nodos,(void*) nodo_arch);

							}
							/*
							 * EJECUTAR REDUCE
//							 */
		                    fin = ejecutar_reduce(rutina_reduce,codigo->stream,cola_nodos,codigo->argv[0]);
							mensaje2=id_message(fin);
							enviar_mensaje(sock_conexion,mensaje2);
							destroy_message(mensaje2);
							destroy_message(codigo);
							free(rutina_reduce);
							break;

			}

		} else {
			log_error_consola("Se ha desconectado un proceso del socket %d",sock_conexion);
			log_error_interno("Se ha desconectado un proceso del socket %d",sock_conexion);
			break;
		}
	}
	return NULL ;
}


void setBloque(int numeroBloque, char* bloque_datos) {
    //debo pararme en la posicion donde se encuentra almacenado el bloque y empezar a grabar
	log_info_interno( "Inicio setBloque(%d)", numeroBloque);
	//el memset lo hago para limpiar el bloque por las dudas
	memset(_data + (numeroBloque * tamanio_bloque), 0, tamanio_bloque);
	memcpy(_data + (numeroBloque * tamanio_bloque), bloque_datos,
			tamanio_bloque);
	log_info_interno( "Fin setBloque(%d)", numeroBloque);

}
char* getBloque(int numeroBloque) {
	log_info_interno( "Ini getBloque(%d)", numeroBloque);
	char* bloque = NULL;
	bloque = malloc(tamanio_bloque);
	memcpy(bloque, &(_data[numeroBloque * tamanio_bloque]), tamanio_bloque);
	//memcpy(bloque, _bloques[numero], TAMANIO_BLOQUE);
	log_info_interno( "Fin getBloque(%d)", numeroBloque);
	return bloque;
}


char* getFileContent(char* filename) {

       log_info_interno( "Inicio getFileContent(%s)", filename);
       char* content = NULL;
       //creo el espacio para almacenar el archivo
       char* path = file_combine(DIR_TEMP, filename);
       size_t size = file_get_size(path) + 1;
       content = malloc(size);
       printf("size: %d\n", size);
       char* mapped = NULL;
       mapped = file_get_mapped(path);
       memcpy(content, mapped, size);        //
       file_mmap_free(mapped, path);
       free_null((void*)&path);
       log_info_interno( "Fin getFileContent(%s)", filename);
       return content;

}

char* crear_Espacio_Datos(int NUEVO, char* ARCHIVO, char* parametro) {
	size_t tamanio = 20 * 1024 * 1024;
	int tamanio_bytes = atoi(parametro);
	CANT_BLOQUES = tamanio_bytes / tamanio;
	log_info_interno(
			"Comienzo de creacion del espacio de datos. Cantidad de Bloques: %d",
			CANT_BLOQUES);

	char* direccion;
	char* path;
	if (NUEVO == 1) {
		path = file_combine(DIR_TEMP, ARCHIVO);
		create_file(path, CANT_BLOQUES * tamanio);
		direccion = file_get_mapped(path);
	} else {
		path = file_combine(DIR_TEMP, ARCHIVO);
		direccion = file_get_mapped(path);
	}
	log_info_interno(
			"Fin de creacion del espacio de datos. Cantidad de Bloques: %d",
			CANT_BLOQUES);
	return direccion;
}

void liberar_Espacio_datos(char* _data,char* ARCHIVO){
	char* path = file_combine(DIR_TEMP, ARCHIVO);
	file_mmap_free(_data,path );
}
t_msg_id ejecutar_map(char*ejecutable, char* nombreArchivoFinal,int numeroBloque, int mapid) {
	log_info_consola("Inicio ejecutarMap ID:%d en el bloque %d", mapid,
			numeroBloque);
	log_info_interno("Inicio ejecutarMap ID:%d en el bloque %d", mapid,
			numeroBloque);
	char*bloque = NULL;
	bloque = getBloque(numeroBloque);
	char* temporal = generar_nombre_temporal(mapid, "map", numeroBloque);
	char*ruta_sort = "/usr/bin/sort";
	char* path_ejecutable = generar_nombre_rutina("map");
	write_file(path_ejecutable, ejecutable, strlen(ejecutable));
	chmod(path_ejecutable, S_IRWXU);
	if (ejecutar(bloque, path_ejecutable, temporal)) {
		return FIN_MAP_ERROR;
	}
	char* data = read_whole_file(temporal);
	if (ejecutar(data, ruta_sort, nombreArchivoFinal)) {
		return FIN_MAP_ERROR;
	}
	list_add_archivo_tmp(nombreArchivoFinal);
	remove(path_ejecutable);
	remove(temporal);
	free(bloque);
	return FIN_MAP_OK;
	log_info_consola("Fin ejecutarMap ID:%d en el bloque %d", mapid,
			numeroBloque);
	log_info_interno("Fin ejecutarMap ID:%d en el bloque %d", mapid,
			numeroBloque);
}

t_msg_id ejecutar_reduce(char*ejecutable, char* nombreArchivoFinal,
		t_queue* colaArchivos, int id_reduce) {

	log_info_consola("Inicio ejecutar Reduce ID:%d ", id_reduce);
	log_info_consola("Inicio ejecutar Reduce ID:%d ", id_reduce);

	t_msg* mensaje;
	t_list* lista_nodos;

	char* path_ejecutable = generar_nombre_rutina("reduce");
	write_file(path_ejecutable, ejecutable, strlen(ejecutable));
	chmod(path_ejecutable, S_IRWXU);
	char*temporal = generar_nombre_temporal(id_reduce, "reduce", 5);

	lista_nodos = deserealizar_cola(colaArchivos);

	apareo(temporal, lista_nodos);
	char* data = read_whole_file(temporal);
	if (ejecutar(data, path_ejecutable, nombreArchivoFinal)) {
		return FIN_REDUCE_ERROR;
	}
	destroy_message(mensaje);

	log_info_consola("Fin ejecutar Reduce ID:%d ", id_reduce);
	log_info_consola("Fin ejecutar Reduce ID:%d ", id_reduce);
	return FIN_REDUCE_OK;
}

t_list* deserealizar_cola(t_queue* colaArchivos) {
	int k, a;
	char** lista_archivos_aux;
	t_nodo_archivo* nodo_aux;
	t_list* lista_nodos = list_create();
	t_nodo_archivo* elem = (t_nodo_archivo*) malloc(sizeof(t_nodo_archivo));
	elem = (t_nodo_archivo *) queue_pop(colaArchivos);
	lista_archivos_aux = string_split(elem->archivo, ";");
	for (k = 0; lista_archivos_aux[k] != NULL; k++) {
		nodo_aux->ip = NULL;
		nodo_aux->puerto = elem->puerto;
		nodo_aux->archivo = lista_archivos_aux[k];
		list_add(lista_nodos, nodo_aux);
	}
	a = queue_size(colaArchivos);
	if (a != 0) {
		elem = (t_nodo_archivo *) queue_pop(colaArchivos);
		while (a != 0) {
			lista_archivos_aux = string_split(elem->archivo, ";");
			for (k = 0; lista_archivos_aux[k] != NULL; k++) {
				nodo_aux->ip = elem->ip;
				nodo_aux->puerto = elem->puerto;
				nodo_aux->archivo = lista_archivos_aux[k];
				list_add(lista_nodos, nodo_aux);
			}
			a = queue_size(colaArchivos);
			elem = (t_nodo_archivo *) queue_pop(colaArchivos);

		}
	}
	free(elem);
	return lista_nodos;
}

void apareo(char* temporal,t_list* lista_nodos_archivos){
	char** registros = malloc(sizeof(int));
	int i, pos, valor_actual_1, valor_actual_2;
	char* clave_actual_1, clave_actual_2, registro_actual_1, registro_actual_2, aux_append;

	char* ruta = string_new();
	string_append(&ruta, "/tmp/");
	string_append(&ruta, temporal);
	FILE* archivo = fopen(ruta, "ab");

	// Obtengo el primer registro de cada archivo
	int cantidad_nodos_archivos = list_size(lista_nodos_archivos);
	t_nodo_archivo* elem =(t_nodo_archivo*) malloc(sizeof(t_nodo_archivo));
	for (i=0; i<cantidad_nodos_archivos; i++) {
		elem = (t_nodo_archivo *) list_get(lista_nodos_archivos,i);
		registros[i] = obtener_proximo_registro(elem);
	}
	registros[i+1] = NULL;

	pos = obtener_posicion_menor_clave(registros);
	strcpy(registro_actual_1, registros[pos]);
	elem = (t_nodo_archivo *) list_get(lista_nodos_archivos,pos);
	registros[pos] = obtener_proximo_registro(elem);
	char** array_aux = string_n_split(registro_actual_1, 2, ";");
	strcpy(clave_actual_1, array_aux[0]);
	valor_actual_1 = atoi(array_aux[1]);

	pos = obtener_posicion_menor_clave(registros); // esta pos se obtiene para ya tener un 2do valor antes de entrar al while
	// obtener_posicion_menor_clave devuelve -1 una vez que en el array ya son todos campos nulos u EOF
	while (pos != -1) {
		strcpy(registro_actual_2, registros[pos]);
		elem = (t_nodo_archivo *) list_get(lista_nodos_archivos,pos);
		registros[pos] = obtener_proximo_registro(elem);
		char** array_aux = string_n_split(registro_actual_2, 2, ";");
		strcpy(clave_actual_2, array_aux[0]);
		valor_actual_2 = atoi(array_aux[1]);

		if (string_equals_ignore_case(clave_actual_1, clave_actual_2)) {
			valor_actual_1 = valor_actual_1 + valor_actual_2;
		} else {
			string_append(&clave_actual_1, ";");
			string_append(&clave_actual_1, string_itoa(valor_actual_1));
			string_append(&clave_actual_1, "\n");
			if (archivo != NULL) {
				fputs(clave_actual_1, archivo);
			} else {
				log_error_interno("No se pudo acceder al archivo temporal para guardar data de apareamiento");
			}
			strcpy(clave_actual_1, clave_actual_2);
			valor_actual_1 = valor_actual_2;
		}
		pos = obtener_posicion_menor_clave(registros);
	}

	// guardo en el archivo el ultimo registro con el que estaba trabajando para no perderlo
	string_append(&clave_actual_1, ";");
	string_append(&clave_actual_1, string_itoa(valor_actual_1));
	string_append(&clave_actual_1, "\n");
	if (archivo != NULL) {
		fputs(clave_actual_1, archivo);
	} else {
		log_error_interno("No se pudo acceder al archivo temporal para guardar data de apareamiento");
	}
	// TODO: hay q ver de liberar todas las variables usadas aca
	fclose(archivo);
}

char* obtener_proximo_registro(t_nodo_archivo* nodo_archivo) {
	char* resultado;
	if (nodo_archivo->ip==NULL) {
		resultado = obtener_proximo_registro_de_archivo(nodo_archivo->archivo);
	} else {
		resultado = enviar_mensaje_proximo_registro(nodo_archivo);
	}
	return resultado;
}

int obtener_posicion_menor_clave(char** registros) {
	int pos, i, aux;
	char* clave_1 = string_new();
	char* clave_2 = string_new();

	// obtiene primer campo != EOF
	for (pos=0; (registros[pos]!=NULL) && (string_is_empty(clave_1)); pos++){
		if (registros[pos]!=EOF) {
			clave_1 = string_n_split(registros[pos], 2, ";")[0];
		}
	}

	if (!string_is_empty(clave_1)) {
		for (i = pos + 1; registros[i]!=NULL; i++) {
			if (registros[i]!=EOF) {
				clave_2 = string_n_split(registros[i], 2, ";")[0];
				aux = obtener_menor(clave_1, clave_2); //devuelve 0 si son iguales, 1 si clave_1 es menor, 2 si clave_2 es menor
				if (aux==2) {
					pos = i;
					strcpy(clave_1, clave_2);
				}
			}
		}
	} else {
		pos = -1;
	}
	// devuelve -1 una vez que en el array ya son todos campos nulos u EOF
	return pos;
}

int obtener_menor(char* clave_1, char* clave_2) {
	int resultado;
	//TODO: devuelve 0 si son iguales, 1 si clave_1 es menor, 2 si clave_2 es menor

	return resultado;
}

char* obtener_proximo_registro_de_archivo(char* archivo) {
	char* resultado = string_new();
	fpos_t* posicion_puntero = obtener_posicion_puntero_arch_tmp(archivo);

	FILE* file = fopen(archivo, "r");
	if (file != NULL) {
		if (posicion_puntero != NULL) {
			fsetpos(file, posicion_puntero);
		}
		resultado = fgets(resultado, 101, file);
		fgetpos(file, posicion_puntero);
		fclose(file);
		actualizar_posicion_puntero_arch_tmp(archivo, posicion_puntero);
	} else {
		log_error_interno("No pudo abrirse el archivo temporal");
		resultado = NULL;
	}

	return resultado;
}

char* enviar_mensaje_proximo_registro(t_nodo_archivo* nodo_archivo) {
	char* resultado;
	int socket_tmp;
	socket_tmp = client_socket(nodo_archivo->ip, nodo_archivo->puerto);
	t_msg* msg = string_message(GET_NEXT_ROW, nodo_archivo->archivo, 0);
	enviar_mensaje(socket_tmp, msg);
	msg = recibir_mensaje(socket_tmp);
	if (msg->header.id==GET_NEXT_ROW_OK) {
		strcpy(resultado, msg->stream);
	}
	if (msg->header.id==GET_NEXT_ROW_ERROR) {
		log_error_interno("El nodo no devolvio el proximo registro. Devolvio ERROR.");
	}
	// TODO: Fijarse si aca falla re-pedimos el proximo registro o no.
	return resultado;
}

void list_add_archivo_tmp(char* nombre_archivo) {
	t_archivo_tmp* archivo = malloc(sizeof(t_archivo_tmp));
	strcpy(archivo->nombre_archivo, nombre_archivo);
	archivo->posicion_puntero = NULL;
	list_add(archivos_temporales, archivo);
}

fpos_t* obtener_posicion_puntero_arch_tmp(char* nombre_archivo) {
	t_archivo_tmp* archivo;

	bool _archivo_con_nombre(t_archivo_tmp* archivo_tmp) {
		return string_equals_ignore_case(archivo_tmp->nombre_archivo, nombre_archivo);
	}
	archivo = list_find(archivos_temporales,(void*) _archivo_con_nombre);

	return archivo->posicion_puntero;
}

void actualizar_posicion_puntero_arch_tmp(char* nombre_archivo, fpos_t* posicion_puntero) {
	t_archivo_tmp* archivo;

	bool _archivo_con_nombre(t_archivo_tmp* archivo_tmp) {
		return string_equals_ignore_case(archivo_tmp->nombre_archivo, nombre_archivo);
	}

	archivo = list_find(archivos_temporales,(void*) _archivo_con_nombre);
	archivo->posicion_puntero = posicion_puntero; // TODO: preguntar si esto funciona! Asi se estaria actualizando el puntero en el archivo_tmp q se encuentra en la lista??
}