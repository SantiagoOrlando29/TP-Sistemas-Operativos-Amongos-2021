#include "utils_discordiador.h"


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

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		printf("error");

	freeaddrinfo(server_info);

	return socket_cliente;
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


nuevoTripulante* crearNuevoTripulante(uint32_t id ,uint32_t posicionX, uint32_t posicionY, uint32_t numeroPatota){
	nuevoTripulante* tripulante = malloc(sizeof(nuevoTripulante));
	tripulante->id = id;
	tripulante->posicionX = posicionX;
	tripulante->posicionY = posicionY;
	tripulante->numeroPatota = numeroPatota;
	return tripulante;
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


void enviar_header(tipoMensaje tipo, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->mensajeOperacion = tipo;

	void * magic = malloc(sizeof(paquete->mensajeOperacion));
	memcpy(magic, &(paquete->mensajeOperacion), sizeof(int));

	send(socket_cliente, magic, sizeof(paquete->mensajeOperacion), 0);

	free(magic);
}


void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}


int codigoOperacion (const char* string){
	char** codigo = string_split(string," ");

	for(int j=0; j < sizeof(conversionProceso)/sizeof(conversionProceso[0]);j++){
		if(!strcmp(codigo[0], conversionProceso[j].string)){
			return conversionProceso[j].valor;
		}
	}
	return -1;
}

void mensajeError (t_log* logger) {
	printf("Error, no existe tal proceso\n");
	log_error(logger, "Error en la operacion");
}

/*Operaciones para recibir mensajes*/


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

/*Operaciciones para mostrar en discordiador*/


void recibir_lista_tripulantes(int tipoMensaje, int conexionMiRam, t_log* logger){
	t_list* lista=list_create();
	nuevoTripulante* tripulante = malloc(sizeof(nuevoTripulante));


	if (tipoMensaje == 2){
		printf("recibi el paquete indicado");
		lista = recibir_paquete(conexionMiRam);
		tripulante = (nuevoTripulante*)list_get(lista, 0);
		printf("\n ID: %d \n", tripulante->id );
		printf("Posicion X: %d \n", tripulante->posicionX );
		printf("Posicion Y: %d \n", tripulante->posicionY );
		printf("Pertenece a Patota: %d \n", tripulante->numeroPatota );
		list_destroy(lista);
	}else {
		mensajeError(logger);
	}
	free(tripulante);
}

/*TAMAÑO DE LAS DIFERENTES ESTRUCTURAS*/
size_t tamanioTripulante (nuevoTripulante* tripulante){
	size_t tamanio = sizeof(uint32_t)*4;
	return tamanio;
}


void leer_tareas(char* archTarea){
	   FILE *fp;
	   char *item;
	   char linea[200];
	   fp = fopen(archTarea, "r");
	   if (fp == NULL)
	     {
	        perror("Error al abrir el archivo.\n");
	        exit(EXIT_FAILURE);
	     }
	   tarea* leida=malloc(sizeof(tarea));
	   while (fgets(linea, sizeof(linea), fp)){
		   int codTarea;
		   if(linea[0]=='D'){    //A corregir
			   codTarea = codigoTarea(strtok(linea,";"));
			   leida->tarea=codTarea;
			   leida->parametro=0;

		   }else{
		   codTarea = codigoTarea(strtok(linea," "));

		   leida->tarea = codTarea;
           item = strtok(NULL,";");
		   leida->parametro=atoi(item);
		   }
		   item = strtok(NULL,";");
		   leida->pos_x=atoi(item);
		   item = strtok(NULL,";");
		   leida->pos_y=atoi(item);
		   item = strtok(NULL,"\n");
		   leida->tiempo=atoi(item);
		   imprimirTarea(leida);
		   }

		}


tarea_tripulante codigoTarea(char *nombretarea){
	if(strcmp(nombretarea,"GENERAR_OXIGENO")==0)
		return GENERAR_OXIGENO;
	else if(strcmp(nombretarea,"GENERAR_COMIDA")==0)
			return GENERAR_COMIDA;
	else if(strcmp(nombretarea,"CONSUMIR_COMIDA")==0)
			return CONSUMIR_COMIDA;
	else if(strcmp(nombretarea,"CONSUMIR_OXIGENO")==0)
			return CONSUMIR_OXIGENO;
	else if(strcmp(nombretarea,"GENERAR_BASURA")==0)
			return GENERAR_BASURA;
	else if(strcmp(nombretarea,"DESCARTAR_BASURA")==0)
			return DESCARTAR_BASURA;
	else
		return 6;

}

void imprimirTarea(tarea* aimprimir){
	printf("%d", aimprimir->tarea);
	printf("%i", aimprimir->parametro);
	printf("%d", aimprimir->pos_x);
	printf("%d", aimprimir->pos_y);
	printf("%d\n", aimprimir->tiempo);

}

