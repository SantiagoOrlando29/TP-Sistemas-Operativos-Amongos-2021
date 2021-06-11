#include "utils_miram.h"

int variable_servidor = -1;

void iniciar_servidor(config_struct* config_servidor)
{
	t_log* logg;
	logg = log_create("MiRam1.log", "MiRam1", 1, LOG_LEVEL_DEBUG);
	log_info(logg, "Servidor iniciando");

	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(config_servidor->ip_miram, config_servidor->puerto_miram, &hints, &servinfo);

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

	log_info(logg, "Servidor MiRam encendido");


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

	printf("Me fui");


}


int funcion_cliente(int socket_cliente){
	int tipoMensajeRecibido = -1;
	printf("Se conecto este socket a mi %d\n",socket_cliente);
    while(1){
		tipoMensajeRecibido = recibir_operacion(socket_cliente);
		switch(tipoMensajeRecibido){
					case INICIAR_PATOTA:
						log_info(logger, "Respondiendo");
						enviar_header(INICIAR_PATOTA, socket_cliente);
						break;

					case LISTAR_TRIPULANTES:;
						t_paquete* paquete = crear_paquete(LISTAR_TRIPULANTES);
						tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
						agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));
						enviar_paquete(paquete,socket_cliente);
						eliminar_paquete(paquete);
						break;


					case INICIAR_PLANIFICACION:
						log_info(logger, "Iniciando planificacion");
						t_paquete* tarea_a_enviar;
						tarea* tarea1 = crear_tarea(GENERAR_OXIGENO,5,2,2,5);
						agregar_a_paquete(tarea_a_enviar, tarea1, sizeof(tarea));
						enviar_paquete(tarea_a_enviar,socket_cliente);
						eliminar_paquete(tarea_a_enviar);
						break;

					case PEDIR_TAREA:
						log_info(logger, "Tripulante quiere tarea");
						enviar_header(PEDIR_TAREA, socket_cliente);

						return 0;

					case FIN:
						log_error(logger, "el discordiador finalizo el programa. Terminando servidor");
						variable_servidor = 0;
						return EXIT_FAILURE;

					case -1:
						log_error(logger, "el cliente se desconecto. Terminando servidor");
						return EXIT_FAILURE;
					default:
						log_warning(logger, "Operacion desconocida. No quieras meter la pata");
						break;
		}
    }

}



int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) !=0 ){
		printf("%d", cod_op);
		return cod_op;
	}
	else{
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

tcbTripulante* crear_tripulante(uint32_t tid, char estado, uint32_t posicionX, uint32_t posicionY, uint32_t prox_instruccion, uint32_t puntero_pcb){
	tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
	tripulante->tid = tid;
	tripulante->estado = estado;
	tripulante->posicionX = posicionX;
	tripulante->posicionY = posicionY;
	tripulante->prox_instruccion = prox_instruccion;
	tripulante->puntero_pcb = puntero_pcb;
	return tripulante;
}

pcbPatota* crear_patota(uint32_t pid, uint32_t tareas){
	pcbPatota* patota = malloc(sizeof(pcbPatota));
	patota->pid = pid;
	patota->tareas = tareas;
	return patota;
}

void mostrar_tripulante(tcbTripulante* tripulante,pcbPatota* patota){
	printf("ID %d \n",tripulante->tid);
	printf("posicion x: %d \n",tripulante->posicionX);
	printf("posicion y: %d \n",tripulante->posicionY);
	printf("Status: %c \n",tripulante->estado);
	printf("n patota: %d \n",patota->pid);
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

size_t tamanio_tcb (tcbTripulante* tripulante){
	size_t tamanio = sizeof(uint32_t)*5;
	tamanio += sizeof(char);
	return tamanio;
}

size_t tamanio_pcb(pcbPatota* patota){
	size_t tamanio = sizeof(uint32_t)*2;
	return tamanio;
}

void mensajeError (t_log* logger) {
	printf("Error, no existe tal proceso\n");
	log_error(logger, "Error en la operacion");
}

tarea* crear_tarea(tarea_tripulante cod_tarea,int parametro,int pos_x,int pos_y,int tiempo){
	tarea* tarea_recibida = malloc(sizeof(tarea));
	tarea_recibida->tarea=cod_tarea;
	tarea_recibida->parametro=parametro;
	tarea_recibida->pos_x=pos_x;
	tarea_recibida->pos_y=pos_y;
	tarea_recibida->tiempo=tiempo;
	return tarea_recibida;

}
/*

void iniciar_miram(config_struct* config_servidor){

	config_servidor->posicion_inicial= malloc(config_servidor->tamanio_memoria);

	if(strcmp(config_servidor->squema_memoria,"PAGINACION")==0){

		config_servidor->cant_marcos=atoi(config_servidor->tamanio_memoria)/atoi(config_servidor->tamanio_pag);
		config_servidor->marcos= malloc(config_servidor->cant_marcos);

		//Inicializo el array en 0, me indica la ocupacion de los marcos
		for(int i=0; i<config_servidor->cant_marcos;i++){
			config_servidor->marcos[i]=malloc(sizeof(int));
			config_servidor->marcos[i]= 0;
		}


	}else{
		//Inicializo Segmentacion
	}
}

/*
void agregar_memoria_aux(t_list* tabla_aux){

	tabla_paginacion* tabla1;
	marco* marco1,marco2,marco3,marco4;
	marco2->id_marco=2;
	marco3->id_marco=3;
	marco4->id_marco=4;
	tabla1->id_patota =1;
	tabla1->ubicacion = MEM_PRINCIPAL;
	list_add(tabla1->marco_inicial, marco2);
	list_add(tabla1->marco_inicial, marco3);
	list_add(tabla1->marco_inicial, marco4);
	list_add(tabla_aux, tabla1);

}

void imprimir_memoria(t_list* tabla_aux){
	for(int i=0; i<list_size(tabla_aux);i++){
		tabla_paginacion auxiliar=(tabla_paginacion*)list_get(tabla_aux,i);
		printf("Proceso correspondiente a patota %d", auxiliar->id_patota);
		for(int j=0;j<list_size(auxiliar->marco_inicial);j++){
			marco marco_leido=(marco*)list_get(auxiliar->marco_inicial,j);
			printf("Marco en uso %d", marco_leido->id_marco);
		}
	}

}
*/
