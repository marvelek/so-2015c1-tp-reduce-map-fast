#include "filesystem.h"
#include "consola.h"
#include "database.h"
#include "server.h"
#include <pthread.h>
#include <commons/config.h>
#include <stddef.h>
#include <utiles/log.h>

#define ARCHIVO_CONFIG "config/filesystem"
#define RUTA_LOG "log/filesystem.log"

char* PUERTO_LISTEN;
int CANTIDAD_NODOS;

void levantar_configuracion() {
	t_config* config;
	config = config_create(ARCHIVO_CONFIG);

	if (config == NULL) {
		log_error_consola("No se pudo abrir el archivo de configuracion");
		exit(1);
	}
	log_debug_interno("Leyendo archivo de configuracion");
	if (config_has_property(config, "PUERTO_LISTEN")) {
		PUERTO_LISTEN = config_get_string_value(config, "PUERTO_LISTEN");
	} else {
		log_error_consola("El archivo de configuracion debe tener un PUERTO_LISTEN");
		exit(1);
	}
	if (config_has_property(config, "CANTIDAD_NODOS")) {
		CANTIDAD_NODOS = config_get_int_value(config, "CANTIDAD_NODOS");
	} else {
		log_error_consola("El archivo de configuracion debe tener una CANTIDAD_NODOS");
		exit(1);
	}
	log_info_interno("Archivo de configuracion cargado correctamente");
}

int main(void) {
	log_crear("DEBUG", RUTA_LOG, "FileSystem");

	levantar_configuracion();
	struct arg_struct args;
	args.puerto_listen = PUERTO_LISTEN;
	inicializar_filesystem(false, CANTIDAD_NODOS);

	pthread_t th_server;
	pthread_create(&th_server, NULL, (void *) iniciar_server, (void*) &args);

	pthread_t th_consola;
	pthread_create(&th_consola, NULL, (void *) iniciar_consola, NULL);

	pthread_join(th_consola, NULL);
	return 0;
}
