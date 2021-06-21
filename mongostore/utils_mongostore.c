#include "utils_mongostore.h"
#include <commons/string.h>

int socket_servidor;
int variable_servidor = -1;

int iniciar_servidor2(char* ip_mongostore, char* puerto_mongostore) //Viejo inicio
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip_mongostore, puerto_mongostore, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    log_trace(logger, "Servidor Mongo encendido");

    return socket_servidor;
}

void iniciar_servidor(config_struct* config_servidor)
{
	t_log* logg;
	logg = log_create("Mongo1.log", "Mongo1", 1, LOG_LEVEL_DEBUG);
	log_info(logg, "Servidor MongoStore iniciando");

	//int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(config_servidor->ip_mongostore, config_servidor->puerto_mongostore, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	log_info(logg, "Servidor MongoStore encendido");


	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = 0;

	printf("Llegue");


	int hilo;
	while(variable_servidor != 0){

		socket_cliente = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);

		if(socket_cliente>0){
			hilo ++ ;
			log_info(logg, "Estableciendo conexión desde %d", dir_cliente.sin_port);
			log_info(logg, "Creando hilo");

			pthread_t hilo_cliente=(char)hilo;
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente ,(void*)socket_cliente);
			pthread_detach(hilo_cliente);
		}
	}

	printf("Me fui\n");

}

int funcion_cliente(int socket_cliente){ //Funciones de prueba, hay que ver que queremos recibir
	int tipoMensajeRecibido = -1;
	printf("Se conecto este socket a mi %d\n",socket_cliente);
	while(1){
		tipoMensajeRecibido = recibir_operacion(socket_cliente);

		switch(tipoMensajeRecibido)
		{
			case FIN:
				log_error(logger, "el discordiador finalizo el programa. Terminando servidor");
				variable_servidor = 0;
				shutdown(socket_servidor, SHUT_RD);
				close(socket_cliente);
				return EXIT_FAILURE;

			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				variable_servidor = 0;
				shutdown(socket_servidor, SHUT_RD);
				close(socket_cliente);
				return EXIT_FAILURE;

			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;

		}
	}
}


int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	log_info(logger, "Estableciendo conexion con Discordiador");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int tipoMensaje;
	if(recv(socket_cliente, &tipoMensaje, sizeof(int), MSG_WAITALL) != 0)
		return tipoMensaje;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s", buffer);
	free(buffer);
}

//podemos usar la lista de valores para poder hablar del for y de como recorrer la lista
t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
	return NULL;
}


// enviar mensajes

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->mensajeOperacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}


t_paquete* crear_paquete(tipoMensaje tipo)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->mensajeOperacion = tipo;
	crear_buffer(paquete);
	return paquete;
}


void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

char* buscarPath(char* path,char* puntoMontaje){
	char* direccion=string_new();
	string_append(&direccion,puntoMontaje);
	string_append(&direccion,path);
	return direccion;
}
int archivoExiste(char* path){
	if(access(path,F_OK)!=-1){
		return 1; //true
	}
	else{
		return 0; //false
	}
}
void crearDireccion(char* direccion){
	int ok=mkdir(direccion,0755);
	if(ok==0){
		printf("\nCreado con exito..\n");
	}
	else{
		printf("\nDireccion ya existente...\n");
	}
}
void eliminarDirectorio(char* path){
	rmdir(path);
}

