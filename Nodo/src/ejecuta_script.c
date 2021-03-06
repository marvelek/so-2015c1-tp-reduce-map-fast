#include "ejecuta_script.h"

void error(char *s) {
	perror(s);
	exit(1);
}

int ejecuta_rutina(char* data, char* path_ejecutable, char* path_salida, char* map_o_reduce) {
	log_info_consola("Entrando a Ejecutar %s", map_o_reduce);

	int size = strlen(data);
	int in[2], out[2];
	//Crea 2 pipes, uno para stdin y otro para stdout. in[0] y out[0] se usan para leer e in[1] y out[1] para escribir
	if (pipe(in) < 0) {
		error("pipe in");
		return -1;
	}

	if (pipe(out) < 0) {
		error("pipe in");
		return -1;
	}

	pid_t pid_sort = fork();

	if (!pid_sort) {
		//Fork devuelve 0 en el hijo y el pid del hijo en el padre, asi que aca estoy en el hijo.

		//Duplica stdin al lado de lectura in y stdout y stdeer al lado de escritura out
		int fd = open(path_salida, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(in[0], 0);
		dup2(fd, 1);

		close(fd);

		//Cierra los pipes que se usan en el padre
		close(in[1]);
		close(in[0]);
		close(out[1]);
		close(out[0]);

		//La imagen del proceso hijo se reemplaza con la del path_ejecutable
		execlp("/usr/bin/sort", "/usr/bin/sort", NULL);
		//On success aca nunca llega porque la imagen (incluido el codigo) se reemplazo en la lina anterior
		error("No se pudo ejecutar el proceso");
		return -1;
	}

	pid_t pid_map = fork();

	if (!pid_map) {
		//Fork devuelve 0 en el hijo y el pid del hijo en el padre, asi que aca estoy en el hijo.

		//Duplica stdin al lado de lectura in y stdout y stdeer al lado de escritura out
		dup2(out[0], 0);
		dup2(in[1], 1);

		//Cierra los pipes que se usan en el padre
		close(in[1]);
		close(in[0]);
		close(out[1]);
		close(out[0]);

		//La imagen del proceso hijo se reemplaza con la del path_ejecutable
		execlp(path_ejecutable, path_ejecutable, NULL);
		//On success aca nunca llega porque la imagen (incluido el codigo) se reemplazo en la lina anterior
		error("No se pudo ejecutar el proceso");
		return -1;
	}

	//Cierra los lados de los pipes que se usan en el hijo
	close(out[0]);
	close(in[1]);
	close(in[0]);
	int total = 0;
	int pendiente = size;

	//Escribe en el lado de escritura del pipe que el proceso hijo va a ver como stdin
	while (total < size) {
		int enviado = write(out[1], data, pendiente);
		total += enviado;
		pendiente -= enviado;
	}
	//Se cierra para generar un EOF y que el proceso hijo termine de leer de stdin
	close(out[1]);
	int status;
	waitpid(pid_map, &status, 0);
	waitpid(pid_sort, &status, 0);
	FILE* temp_file = fopen(path_salida, "r");
	free(data);
	fclose(temp_file);
	return status;
}

int ejecuta_rutina_primero_sort(char* data, char* path_ejecutable, char* path_salida, char* map_o_reduce) {
	log_info_consola("Entrando a Ejecutar %s", map_o_reduce);

	int size = strlen(data);
	int in[2], out[2];
	//Crea 2 pipes, uno para stdin y otro para stdout. in[0] y out[0] se usan para leer e in[1] y out[1] para escribir
	if (pipe(in) < 0) {
		error("pipe in");
		return -1;
	}

	if (pipe(out) < 0) {
		error("pipe in");
		return -1;
	}

	pid_t pid_sort = fork();

	if (!pid_sort) {
		//Fork devuelve 0 en el hijo y el pid del hijo en el padre, asi que aca estoy en el hijo.

		//Duplica stdin al lado de lectura in y stdout y stdeer al lado de escritura out
		int fd = open(path_salida, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(in[0], 0);
		dup2(fd, 1);

		close(fd);

		//Cierra los pipes que se usan en el padre
		close(in[1]);
		close(in[0]);
		close(out[1]);
		close(out[0]);

		//La imagen del proceso hijo se reemplaza con la del path_ejecutable
		execlp(path_ejecutable, path_ejecutable, NULL);
		//On success aca nunca llega porque la imagen (incluido el codigo) se reemplazo en la lina anterior
		error("No se pudo ejecutar el proceso");
		return -1;
	}

	pid_t pid_map = fork();

	if (!pid_map) {
		//Fork devuelve 0 en el hijo y el pid del hijo en el padre, asi que aca estoy en el hijo.

		//Duplica stdin al lado de lectura in y stdout y stdeer al lado de escritura out
		dup2(out[0], 0);
		dup2(in[1], 1);

		//Cierra los pipes que se usan en el padre
		close(in[1]);
		close(in[0]);
		close(out[1]);
		close(out[0]);

		//La imagen del proceso hijo se reemplaza con la del path_ejecutable
		execlp("/usr/bin/sort", "/usr/bin/sort", NULL);
		//On success aca nunca llega porque la imagen (incluido el codigo) se reemplazo en la lina anterior
		error("No se pudo ejecutar el proceso");
		return -1;
	}

	//Cierra los lados de los pipes que se usan en el hijo
	close(out[0]);
	close(in[1]);
	close(in[0]);
	int total = 0;
	int pendiente = size;

	//Escribe en el lado de escritura del pipe que el proceso hijo va a ver como stdin
	while (total < size) {
		int enviado = write(out[1], data, pendiente);
		total += enviado;
		pendiente -= enviado;
	}
	//Se cierra para generar un EOF y que el proceso hijo termine de leer de stdin
	close(out[1]);
	int status;
	waitpid(pid_map, &status, 0);
	waitpid(pid_sort, &status, 0);
	FILE* temp_file = fopen(path_salida, "r");
	free(data);
	fclose(temp_file);
	return status;
}

pid_t ejecuta_reduce(int in[2], char* path_ejecutable, char* path_salida) {
	log_info_consola("Entrando a Ejecutar REDUCE");

	pid_t pid = fork();

	if (!pid) {
		//Fork devuelve 0 en el hijo y el pid del hijo en el padre, asi que aca estoy en el hijo.

		//Duplica stdin al lado de lectura in y stdout y stdeer al lado de escritura out
		int fd = open(path_salida, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(in[0], 0);
		dup2(fd, 1);

		close(fd);

		//Cierra los pipes que se usan en el padre
		close(in[1]);
		close(in[0]);

		//La imagen del proceso hijo se reemplaza con la del path_ejecutable
		execlp(path_ejecutable, path_ejecutable, NULL);
		//On success aca nunca llega porque la imagen (incluido el codigo) se reemplazo en la lina anterior
		error("No se pudo ejecutar el proceso");
		return -1;
	}
	return pid;
}
char* generar_nombre_rutina(int tarea_id, char*map_o_reduce, int numeroBloque) {
	char* file_map1 = string_new();
	string_append(&file_map1, map_o_reduce);
	string_append(&file_map1, "_id");
	string_append(&file_map1, string_itoa(tarea_id));
	string_append(&file_map1, "_bloque");
	string_append(&file_map1, string_itoa(numeroBloque));

	return file_map1;
}

