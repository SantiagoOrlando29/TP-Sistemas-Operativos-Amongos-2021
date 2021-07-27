#include "utils_miram.h"

int variable_servidor = -1;
int socket_servidor;
t_list* tabla_paginacion_ppal;

int borrar = 1;

t_list* memoria_aux;
NIVEL* nivel;
int err;
int numero_mapa = 0;
int cols=10;
int rows=10;
int contador_lru=0;

void iniciar_servidor(config_struct* config_servidor)
{

//Inicializacion de estructuras de memoria
	tabla_paginacion_ppal=list_create();

	log_info(logger, "Servidor iniciando");

	//int socket_servidor;

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

        int activado = 1;
        setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)); //para que si se cierra sin el close no haya que esperar

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	log_info(logger, "Servidor MiRam encendido");


	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = 0;

	log_info(logger, "socket_servidor %d", socket_servidor);

	//se crea mapa
	/*nivel_gui_inicializar();

	nivel_gui_get_area_nivel(&cols, &rows);

	nivel = nivel_crear("AMong-OS");*/

	int hilo;
	while(variable_servidor != 0){

		socket_cliente = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);
		log_info(logger, "socket_cliente %d", socket_cliente);

		if(socket_cliente>0){
			hilo ++ ;
			log_info(logger, "Estableciendo conexión desde %d", dir_cliente.sin_port);
			log_info(logger, "Creando hilo");

			pthread_t hilo_cliente=(char)hilo;
			if(strcmp(configuracion.squema_memoria, "SEGMENTACION")==0){
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente_segmentacion ,(void*)socket_cliente);
			pthread_detach(hilo_cliente);
		    }else{
		    swap_pagina_iniciar();
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente_paginacion ,(void*)socket_cliente);
			pthread_detach(hilo_cliente);

		    }
		}
	}
	log_info(logger, "tttttttt");
	printf("me fui");

	//log_destroy(logger);

}


//Funcion cliente para segmentacion
int funcion_cliente_segmentacion(int socket_cliente){
	//t_list* lista = list_create();
	t_list* lista;
	bool lista_usada = false;
	pcbPatota* patota;
	tcbTripulante* tripulante;
	//t_paquete* paquete;
	int tripulante_id;
	int patota_id;
	char* pid_char;
	char* tid_char;
	//int cant_sabotaje = 0;//para probar mongo

	int tipoMensajeRecibido = -1;
	log_info(logger, "Se conecto este socket a mi %d",socket_cliente);

	while(1){
		tipoMensajeRecibido = recibir_operacion(socket_cliente);

		switch(tipoMensajeRecibido)
		{
			case INICIAR_PATOTA:
				lista = recibir_paquete(socket_cliente);

				//patota_id = (int)atoi(list_get(lista,0));
				pid_char = list_get(lista,0);
				patota_id = (int)atoi(pid_char);
				free(pid_char);
				log_info(logger,"\nEl numero de la patota recibida es %d", patota_id);

				//uint32_t cantidad_tripulantes = (uint32_t)atoi(list_get(lista,1));
				char* cant_tripu_char = list_get(lista,1);
				uint32_t cantidad_tripulantes = (uint32_t)atoi(cant_tripu_char);
				free(cant_tripu_char);

				for(int i=2; i < cantidad_tripulantes +2; i++){
					tripulante = (tcbTripulante*)list_get(lista,i);
					log_info(logger, "ID %d ",tripulante->tid);
					log_info(logger, "posicion x: %d ",tripulante->posicionX);
					log_info(logger, "posicion y: %d ",tripulante->posicionY);
					log_info(logger, "Status: %c ",tripulante->estado);
					log_info(logger, "n patota: %d \n",patota_id);
				}

				//char* tarea = malloc((uint32_t)atoi(list_get(lista, cantidad_tripulantes+2)));
				char* largo_tarea_char = list_get(lista, cantidad_tripulantes+2);
				free(largo_tarea_char);
				char* tarea = (char*)list_get(lista, cantidad_tripulantes+3);


				bool todo_ok = patota_segmentacion(patota_id, cantidad_tripulantes, tarea, lista);
				if (todo_ok == false){
					log_warning(logger, "no se puedo asignar espacio de memoria a todo");
					char* mensaje = "Memoria NO asignada";
					enviar_mensaje(mensaje, socket_cliente);
				} else {
					log_info(logger, "Se asigno espacio de memoria a todo \n");
					char* mensaje = "Memoria asignada";
					enviar_mensaje(mensaje, socket_cliente);
				}

				list_destroy(lista);

				break;


			case LISTAR_TRIPULANTES:;
				t_paquete* paquete = crear_paquete(LISTAR_TRIPULANTES);

				log_info(logger, "------ Lista Tripulantes ---------");
				for(int i=0; i < list_size(lista_tablas_segmentos); i++){
					tabla_segmentacion* tabla_segmentos = (tabla_segmentacion*)list_get(lista_tablas_segmentos, i);
					int pid = tabla_segmentos->id_patota;

					for(int j=2; j < list_size(tabla_segmentos->lista_segmentos); j++){
						segmento* segmento = list_get(tabla_segmentos->lista_segmentos, j);

						sem_wait(&MUTEX_TABLA_MEMORIA);
						for(int k=0; k < list_size(tabla_espacios_de_memoria); k++){
							espacio_de_memoria* espacio = (espacio_de_memoria*)list_get(tabla_espacios_de_memoria, k);
							if(espacio->base == segmento->base){
								tcbTripulante* tripulante = espacio->contenido;

								log_info(logger, "Tripulante: %d     Patota: %d     Status: %c", tripulante->tid, pid, tripulante->estado);
								/*for(int i=0; i < list_size(lista_tripulantes); i+=2){
									tripulante = (tcbTripulante*)list_get(lista_tripulantes, i);
									int numero_patota = (int)atoi(list_get(lista_tripulantes,i+1));
									printf("Tripulante: %d     Patota: %d     Status: %c \n", tripulante->tid, numero_patota, tripulante->estado);
								}*/

								agregar_a_paquete(paquete, tripulante, tamanio_TCB);

								char* pidd_char = malloc(sizeof(char)+1);
								sprintf(pidd_char, "%d", pid);
								agregar_a_paquete(paquete, pidd_char, strlen(pidd_char)+1);
								free(pidd_char);
								//printf("Tripulante: %d     Patota: %d     Status: %c \n", tripulante->tid, pid, tripulante->estado);
								k = list_size(tabla_espacios_de_memoria); //para que corte el for
							}
						}
						sem_post(&MUTEX_TABLA_MEMORIA);
					}
				}

				if(paquete->buffer->size == 0){ //no habia TCBs para mandar
					enviar_header(NO_HAY_NADA_PARA_LISTAR , socket_cliente);
				}else{
					enviar_paquete(paquete, socket_cliente);
				}

				eliminar_paquete(paquete);

				break;


			case EXPULSAR_TRIPULANTE:
				lista = recibir_paquete(socket_cliente);

				//tripulante_id = (int)atoi(list_get(lista,0));
				tid_char = list_get(lista,0);
				tripulante_id = (int)atoi(tid_char);
				//free(tid_char);
				pid_char = list_get(lista,1);
				patota_id = (int)atoi(pid_char);
				log_info(logger, "tid %d  pid %d", tripulante_id, patota_id);

				if(strcmp(configuracion.squema_memoria,"SEGMENTACION")==0){
					bool tripulante_expulsado_con_exito = funcion_expulsar_tripulante(tripulante_id);
					if(tripulante_expulsado_con_exito == false){
						log_warning(logger, "No se pudo eliminar ese tripulante");
					}
				}

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);
				lista_usada = true;
				//return 0;//para que termine el hilo se supone
				break;

			case PEDIR_TAREA:;
				lista = recibir_paquete(socket_cliente);
				//tripulante_id = (int)atoi(list_get(lista, 0));
				tid_char = list_get(lista,0);
				tripulante_id = (int)atoi(tid_char);
				//free(tid_char);

				//patota_id = (int)atoi(list_get(lista, 1));
				pid_char = list_get(lista,1);
				patota_id = (int)atoi(pid_char);
				//free(pid_char);

				bool hay_mas_tareas = enviar_tarea_segmentacion(socket_cliente, patota_id, tripulante_id);
				if(hay_mas_tareas == false){
					funcion_expulsar_tripulante(tripulante_id);
					//list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);
					//close(socket_cliente);
					//NOSE SI TENDRIA Q HACER ESTO. EN DISCORDIADOR LO VA A HACER PERO CON ESTO QUE YA TERMINO ENTONCES NUNCA RECIBE ESTE CLOSE
					//PERO SI LO HAGO ACA DIRIA Q ROMPE PQ CAPAZ EL OTRO NO RECIBIO TODAVIA Y YO YA CERRE CONEXION.
					//return 0;//para que termine el hilo se supone
				}

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;

			case CAMBIAR_DE_ESTADO:;
				lista = recibir_paquete(socket_cliente);
				//tripulante_id = (int)atoi(list_get(lista, 0));
				tid_char = list_get(lista,0);
				tripulante_id = (int)atoi(tid_char);
				//free(tid_char);

				char* estado_recibido = list_get(lista, 1);
				char estado = estado_recibido[0];

				//patota_id = (int)atoi(list_get(lista, 2));
				pid_char = list_get(lista,2);
				patota_id = (int)atoi(pid_char);
				//free(pid_char);

				bool cambio_exitoso = cambiar_estado(patota_id, estado, tripulante_id);
				if(cambio_exitoso == false){
					char* mensaje = "fallo cambio de estado";
					//log_info(logger, "fallo cambio estado");
					enviar_mensaje(mensaje, socket_cliente);
				}else{
					char* mensaje = "cambio de estado exitoso";
					enviar_mensaje(mensaje, socket_cliente);
				}

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;

			case INFORMAR_MOVIMIENTO:;
				lista = recibir_paquete(socket_cliente);
				//tripulante_id = (int)atoi(list_get(lista, 0));
				tid_char = list_get(lista,0);
				tripulante_id = (int)atoi(tid_char);
				//free(tid_char);

				//int posx = (int)atoi(list_get(lista, 1));
				char* posx_char = list_get(lista, 1);
				int posx = (int)atoi(posx_char);
				//free(posx_char);

				//int posy = (int)atoi(list_get(lista, 2));
				char* posy_char = list_get(lista, 2);
				int posy = (int)atoi(posy_char);
				//free(posy_char);

				//patota_id = (int)atoi(list_get(lista, 3));
				pid_char = list_get(lista,3);
				patota_id = (int)atoi(pid_char);
				//free(pid_char);

				bool movimiento_exitoso = cambiar_posicion(tripulante_id, posx, posy, patota_id);
				if(movimiento_exitoso == false){
					char* mensaje = "fallo cambio de posicion";
					enviar_mensaje(mensaje, socket_cliente);
				}else{
					char* mensaje = "cambio de posicion exitoso";
					enviar_mensaje(mensaje, socket_cliente);
				}

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;

			case FIN:
				log_error(logger, "el discordiador finalizo el programa. Terminando servidor");
				variable_servidor = 0;
				log_info(logger, "aaaaaaaaaa");
				//shutdown(socket_servidor, SHUT_RD);//A VECES ANDA Y A VECES NO
				log_info(logger, "bbbbbbbb");
				close(socket_servidor);
				log_info(logger, "bbbbbbbb");
				close(socket_cliente);

				//list_destroy(lista);//TENGO Q HACERLO SOLO SI NO USO OTRAS FUNCIONES Q DESTRUYAN LISTAS

				/*if(list_size(lista) > 0){
					list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);
				}else{
					list_destroy(lista);
				}*/

				list_destroy_and_destroy_elements(lista_tablas_segmentos, (void*)destruir_tabla_segmentacion);
				if(list_size(tabla_espacios_de_memoria) > 1){ //todavia tiene algun espacio de memoria ocupado
					//list_destroy_and_destroy_elements(tabla_espacios_de_memoria, (void*)destruir_espacio_memoria);
					for(int i=0; i < list_size(tabla_espacios_de_memoria); i++){
						espacio_de_memoria* espacio = list_remove(tabla_espacios_de_memoria, i);
						void* content = espacio->contenido;
						free(content);
						free(espacio);
						i--;
					}
				}else{ //solo tiene el espacio entero de la memoria total
					espacio_de_memoria* espacio = (espacio_de_memoria*)list_remove(tabla_espacios_de_memoria, 0);
					free(espacio);
					list_destroy(tabla_espacios_de_memoria);
				}
				//ME FALTA ALGO CON TRIPUS O TAREAS? DIRIA QUE NO PORQUE ESO ES LO Q HAY EN ESPACIO->CONTENIDO
				log_info(logger, "aaaaaaAAAAA");
				log_destroy(logger);
				config_destroy(archConfig);
				return EXIT_FAILURE;

			case -1:
				log_warning(logger, "el cliente se desconecto.");
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
	if(recv(socket_cliente, &cod_op, sizeof(uint32_t), MSG_WAITALL) !=0 ){
		//log_info(logger, "cod_op %d",cod_op);
		return cod_op;
	}
	else{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(uint32_t* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	uint32_t size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s", buffer);
	free(buffer);
}

//podemos usar la lista de valores para poder hablar del for y de como recorrer la lista
t_list* recibir_paquete(int socket_cliente)
{
	uint32_t size;
	uint32_t desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	uint32_t tamanio;

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


void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + sizeof(uint32_t);

	void* a_enviar = malloc(bytes);
	int desplazamiento = 0;

	memcpy(a_enviar + desplazamiento, &(paquete->buffer->size), sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
	memcpy(a_enviar + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

int enviar_mensaje_malloqueado(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + sizeof(uint32_t);

	void* a_enviar = malloc(bytes);
	int desplazamiento = 0;

	memcpy(a_enviar + desplazamiento, &(paquete->buffer->size), sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
	memcpy(a_enviar + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
	return 1;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	uint32_t desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->mensajeOperacion), sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
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
	tcbTripulante* tripulante = malloc(tamanio_TCB);
	tripulante->tid = tid;
	tripulante->estado = estado;
	tripulante->posicionX = posicionX;
	tripulante->posicionY = posicionY;
	tripulante->prox_instruccion = prox_instruccion;
	tripulante->puntero_pcb = puntero_pcb;
	return tripulante;
}

pcbPatota* crear_patota(uint32_t pid, uint32_t tareas){
	pcbPatota* patota = malloc(tamanio_PCB);
	patota->pid = pid;
	patota->tareas = tareas;
	return patota;
}

void mostrar_tripulante(tcbTripulante* tripulante, pcbPatota* patota){
	log_info(logger, "ID %d ",tripulante->tid);
	log_info(logger, "posicion x: %d ",tripulante->posicionX);
	log_info(logger, "posicion y: %d ",tripulante->posicionY);
	log_info(logger, "Status: %c ",tripulante->estado);
	log_info(logger, "n patota: %d ",patota->pid);
}


void agregar_a_paquete(t_paquete* paquete, void* valor, uint32_t tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(uint32_t));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(uint32_t), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(uint32_t);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void enviar_header(tipoMensaje tipo, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->mensajeOperacion = tipo;

	void * magic = malloc(sizeof(paquete->mensajeOperacion));
	memcpy(magic, &(paquete->mensajeOperacion), sizeof(uint32_t));

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
	//printf("Error, no existe tal proceso\n");
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


void iniciar_miram(config_struct* config_servidor){



	if(strcmp(config_servidor->squema_memoria,"PAGINACION")==0){

		config_servidor->posicion_inicial= malloc((config_servidor->tamanio_memoria));

		config_servidor->marcos_libres=list_create();

		config_servidor->swap_libre=list_create();

		config_servidor->conversion_marcos=list_create();

		int nro_marcos =(config_servidor->tamanio_memoria)/(config_servidor->tamanio_pag);
		config_servidor->cant_marcos = nro_marcos;
		log_info(logger,"\nNumero de marco es: %d nro_marcos\n", nro_marcos);

		int nro_lugares_swap = (config_servidor->tamanio_swap)/(configuracion.tamanio_pag);
		config_servidor->cant_lugares_swap=nro_lugares_swap;

		for(int i=0; i<=nro_lugares_swap;i++){
			int vacio=0;
			list_add(config_servidor->swap_libre,(void*)vacio);
		}
		//config_servidor->marcos= malloc(sizeof(int));
		//Inicializo el array en 0, me indica la ocupacion de los marcos
		for(int i=0; i<=nro_marcos;i++){
			int vacio=0;
			list_add(config_servidor->marcos_libres,(void*)vacio);
		}

	}else{
		tabla_espacios_de_memoria = list_create();
		espacio_de_memoria* memoria_principal = crear_espacio_de_memoria(0, config_servidor->tamanio_memoria, true);
		list_add(tabla_espacios_de_memoria, memoria_principal);
		imprimir_tabla_espacios_de_memoria();
		lista_tablas_segmentos = list_create();
	}
}

void finalizar_miram(config_struct* config_servidor){
	free(config_servidor->posicion_inicial);
	for(int i=0; i<config_servidor->cant_marcos;i++){
		free(config_servidor->marcos[i]);
	}
}



pcbPatota* crear_pcb(uint32_t numero_patota){
	pcbPatota* pcb = malloc(tamanio_PCB);
	pcb->pid = numero_patota;
	//pcb->tareas = NULL;

	return pcb;
}





espacio_de_memoria* crear_espacio_de_memoria(int base, int tam, bool libre){
	espacio_de_memoria* nuevo_espacio_de_memoria = malloc(sizeof(espacio_de_memoria));

	nuevo_espacio_de_memoria->base = base;
	nuevo_espacio_de_memoria->tam = tam;
	nuevo_espacio_de_memoria->libre = libre;
	nuevo_espacio_de_memoria->contenido = NULL; //NO SE SI HACE FALTA

    return nuevo_espacio_de_memoria;
}

void imprimir_tabla_espacios_de_memoria(){
	sem_wait(&MUTEX_TABLA_MEMORIA);
	int size = list_size(tabla_espacios_de_memoria);
	log_info(logger, "---------  MEMORIA  --------------------");

    for(int i=0; i < size; i++) {
    	espacio_de_memoria *espacio = list_get(tabla_espacios_de_memoria, i);
    	log_info(logger, "base: %d, tam: %d, libre: %s ", espacio->base, espacio->tam, espacio->libre ? "true" : "false");
    }
    sem_post(&MUTEX_TABLA_MEMORIA);
    log_info(logger, "----------------------------------------");
}

void imprimir_tabla_segmentos_patota(tabla_segmentacion* tabla_segmentos_patota){
	if(list_size(tabla_segmentos_patota->lista_segmentos) >0){
		log_info(logger, "Tabla de segmentos correspondiente a patota %d", tabla_segmentos_patota->id_patota);

		for(int j=0; j < list_size(tabla_segmentos_patota->lista_segmentos); j++){
			segmento* segmento_leido = list_get(tabla_segmentos_patota->lista_segmentos, j);
			log_info(logger, "Base %d   Tamanio %d", segmento_leido->base, segmento_leido->tamanio);
		}
	}else{
		log_info(logger, "Tabla de segmentos de patota %d ya no existe ", tabla_segmentos_patota->id_patota);
	}
	log_info(logger, "---------------------------------------");
}


void eliminar_espacio_de_memoria(int base){
	sem_wait(&MUTEX_TABLA_MEMORIA);
	for(int i = 0; i < list_size(tabla_espacios_de_memoria); i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

    	if(espacio->base == base){
            espacio->libre = true;
            void* elemento = espacio->contenido; //VER SI CON ESTO ANDA. SINO CAPAZ TENGO Q HACER UNO PARA ELIMINAR TCB OTRO PARA PATOTA Y OTRO P TAREA
            free(elemento);
            //free(espacio->contenido);
            espacio = NULL;
        }
    }
	sem_post(&MUTEX_TABLA_MEMORIA);
    ordenar_memoria();
}

int ordenar_memoria(){
    bool espacio_anterior(espacio_de_memoria* espacio_menor, espacio_de_memoria* espacio_mayor) {
        return espacio_menor->base < espacio_mayor->base;
    }
    sem_wait(&MUTEX_TABLA_MEMORIA);
    list_sort(tabla_espacios_de_memoria, (void*) espacio_anterior);
    sem_post(&MUTEX_TABLA_MEMORIA);
    return 1;
}

int unir_espacios_contiguos_libres(){
	sem_wait(&MUTEX_TABLA_MEMORIA);
	for(int i=0; i < list_size(tabla_espacios_de_memoria) -1; i++){

    	if(list_size(tabla_espacios_de_memoria) == 2){
    		espacio_de_memoria* espacio0 = list_get(tabla_espacios_de_memoria, 0);
    		espacio_de_memoria* espacio1 = list_get(tabla_espacios_de_memoria, 1);
    		if(espacio0->libre && espacio1->libre){
    			list_remove(tabla_espacios_de_memoria,1);
    			espacio0->tam = configuracion.tamanio_memoria;

    			free(espacio1);
    		}
    	}else{
			espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);
			espacio_de_memoria* siguiente_espacio = list_get(tabla_espacios_de_memoria, i+1);

			if (espacio->libre && siguiente_espacio->libre){
				espacio->tam += siguiente_espacio->tam;
				list_remove(tabla_espacios_de_memoria, i+1);
				free(siguiente_espacio);
				//i = 0;
				i = -1;
			}
    	}
    }
	sem_post(&MUTEX_TABLA_MEMORIA);
	return 1;
}

void eliminar_segmento(int nro_segmento, tabla_segmentacion* tabla_segmentos_patota){
	for(int i = 0; i < list_size(tabla_segmentos_patota->lista_segmentos); i++){
		segmento* segmento = list_get(tabla_segmentos_patota->lista_segmentos, i);

		if(segmento->numero_segmento == nro_segmento){
			eliminar_espacio_de_memoria(segmento->base);
			list_remove_and_destroy_element(tabla_segmentos_patota->lista_segmentos, i, (void*)destruir_segmentos);
		}
	}
}

espacio_de_memoria* buscar_espacio_de_memoria_libre(int tam){

	if (strcmp(configuracion.criterio_seleccion, "FF") == 0) {
        return busqueda_first_fit(tam);

    } else if (strcmp(configuracion.criterio_seleccion, "BF") == 0) {
        return busqueda_best_fit(tam);

    } else {
        log_error(logger, "No se encontro el algoritmo pedido");
        exit(EXIT_FAILURE);
    }
}

espacio_de_memoria* busqueda_first_fit(int tam){
	sem_wait(&MUTEX_TABLA_MEMORIA);
	for(int i=0; i < list_size(tabla_espacios_de_memoria); i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);
        if(espacio->libre == true && tam <= espacio->tam){
        	sem_post(&MUTEX_TABLA_MEMORIA);
            return espacio;
        }
    }
	sem_post(&MUTEX_TABLA_MEMORIA);
    log_warning(logger, "No hay espacios de memoria libres");
    return NULL;
}

espacio_de_memoria* busqueda_best_fit(int tam){
	sem_wait(&MUTEX_TABLA_MEMORIA);
	int size = list_size(tabla_espacios_de_memoria);
    t_list* espacios_libres = list_create();

    for(int i=0; i < size; i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

    	if(espacio->libre == true && tam <= espacio->tam){

            if(tam == espacio->tam){ //si encuentra uno justo de su tamanio
            	sem_post(&MUTEX_TABLA_MEMORIA);
            	list_destroy(espacios_libres);
            	return espacio;
            }
            list_add(espacios_libres, espacio);
        }
    }

    int espacios_libres_size = list_size(espacios_libres);

    if(espacios_libres_size != 0){
    	espacio_de_memoria* espacio_best_fit = list_get(espacios_libres, 0);
    	for(int i=1; i < espacios_libres_size; i++){
			espacio_de_memoria* espacio_a_comparar = list_get(espacios_libres, i);

			if(espacio_a_comparar->tam  <  espacio_best_fit->tam){
				espacio_best_fit = espacio_a_comparar;
			}
		}

        //log_info(logger, "El espacio de memoria encontrado con Best Fit (base:%d)", espacio_best_fit->base);
    	sem_post(&MUTEX_TABLA_MEMORIA);
    	list_destroy(espacios_libres);
    	return espacio_best_fit;

    }else{
        log_warning(logger, "No hay espacios de memoria libres");
        sem_post(&MUTEX_TABLA_MEMORIA);
        list_destroy(espacios_libres);
        return NULL;
    }
}

espacio_de_memoria* asignar_espacio_de_memoria(size_t tam) {
	espacio_de_memoria *espacio_libre = buscar_espacio_de_memoria_libre(tam);

	if(espacio_libre == NULL){ //NO PROBE ESTE IF
		log_info(logger, "No hay espacio libre de memoria. Hacemos Compactacion");
		int compact = compactar_memoria();
		espacio_libre = buscar_espacio_de_memoria_libre(tam);
	}

	if (espacio_libre != NULL) {

		if (espacio_libre->tam == tam) {//Si el espacio libre encontrado es de igual tamanio al segmento a alojar no es necesario ordenar. CAPAZ NO HACE FALTA
        	espacio_libre->libre = false;
            return espacio_libre;
        } else { //Si no es de igual tamanio, debo crear un nuevo espacio con base en el libre y reacomodar la base y tamanio del libre.
            espacio_de_memoria* nuevo_espacio_de_memoria = crear_espacio_de_memoria(espacio_libre->base, tam, false);

            sem_wait(&MUTEX_TABLA_MEMORIA);
            list_add(tabla_espacios_de_memoria, nuevo_espacio_de_memoria);
            sem_post(&MUTEX_TABLA_MEMORIA);

            //actualizo base y tamanio del espacio libre
            espacio_libre->base += tam;
            espacio_libre->tam -= tam;

            ordenar_memoria();
            //unir_espacios_contiguos_libres();

            return nuevo_espacio_de_memoria;
        }

    } else {
        log_warning(logger, "No se pudo asignar espacio de memoria");
        return NULL;
    }
}

bool patota_segmentacion(int pid, uint32_t cantidad_tripulantes, char* tarea, t_list* lista){
	char letra = 48;

	pcbPatota* pcb_patota = crear_pcb(pid);
	espacio_de_memoria* espacio_de_memoria_pcb_patota = asignar_espacio_de_memoria(tamanio_PCB);
	if (espacio_de_memoria_pcb_patota == NULL){
		return false;
	}

	espacio_de_memoria* espacio_de_memoria_tareas = asignar_espacio_de_memoria(strlen(tarea));
	if (espacio_de_memoria_tareas == NULL){
		log_info(logger, "espacio tareas todo mal");
		eliminar_espacio_de_memoria(espacio_de_memoria_pcb_patota->base);
		log_info(logger, "espacio tareas todo mal 2");
		return false;
	}
	log_info(logger, "espacio tareas todo ok");
	espacio_de_memoria_tareas->contenido = tarea;

	pcb_patota->tareas = 1; //direccion logica del inicio de las tareas. Segmento de tareas es el 1

	espacio_de_memoria_pcb_patota->contenido = pcb_patota;

	segmento* segmento_pcb = malloc(sizeof(segmento));
	segmento* segmento_tareas = malloc(sizeof(segmento));

	segmento_pcb->numero_segmento = 0;
	segmento_pcb->base = espacio_de_memoria_pcb_patota->base;
	segmento_pcb->tamanio = espacio_de_memoria_pcb_patota->tam;

	segmento_tareas->numero_segmento = 1;
	segmento_tareas->base = espacio_de_memoria_tareas->base;
	segmento_tareas->tamanio = espacio_de_memoria_tareas->tam;

	tabla_segmentacion* tabla_segmentos_patota = malloc(sizeof(tabla_segmentacion));

	tabla_segmentos_patota->id_patota = pid;
	tabla_segmentos_patota->lista_segmentos = list_create();

	list_add(tabla_segmentos_patota->lista_segmentos, segmento_pcb);
	list_add(tabla_segmentos_patota->lista_segmentos, segmento_tareas);

	for(int i=2; i < cantidad_tripulantes +2; i++){
		tcbTripulante* tripulante = (tcbTripulante*)list_get(lista,i);

		if(i == 2){//es el primer tripulante de la lista
			tabla_segmentos_patota->primer_tripulante = tripulante->tid;
		}

		espacio_de_memoria* espacio_de_memoria_tcb_tripulante = asignar_espacio_de_memoria(tamanio_TCB);
		if (espacio_de_memoria_tcb_tripulante == NULL){
			for(int j=0; j < i; j++){
				eliminar_segmento(j, tabla_segmentos_patota);
			}
			unir_espacios_contiguos_libres();
			imprimir_tabla_espacios_de_memoria();
			imprimir_tabla_segmentos_patota(tabla_segmentos_patota);
			return false;
		}

		tripulante->puntero_pcb = 0; //direccion logica del pcb. Segmento pcb es el 0 //FALLA POR EL PADDING CREO.
		tripulante->estado = 'N';
		tripulante->prox_instruccion = 0;
		espacio_de_memoria_tcb_tripulante->contenido = tripulante;

		segmento* segmento_tcb = malloc(sizeof(segmento));
		segmento_tcb->numero_segmento = i;
		segmento_tcb->base = espacio_de_memoria_tcb_tripulante->base;
		segmento_tcb->tamanio = espacio_de_memoria_tcb_tripulante->tam;

		list_add(tabla_segmentos_patota->lista_segmentos, segmento_tcb);

		if(i == cantidad_tripulantes +2 -1){ //es el ultimo tripulante de la lista
			tabla_segmentos_patota->ultimo_tripulante = tripulante->tid;
		}

		/*if(tripulante->tid > 9){ //desde el 10 el mapa tira ":" sino
			letra = 55;
		}
		err = personaje_crear(nivel, letra+tripulante->tid, (int)tripulante->posicionX, (int)tripulante->posicionY);
		ASSERT_CREATE(nivel, letra, err);*/
	}

	imprimir_tabla_espacios_de_memoria();
	imprimir_tabla_segmentos_patota(tabla_segmentos_patota);

	sem_wait(&MUTEX_LISTA_TABLAS_SEGMENTOS);
	list_add(lista_tablas_segmentos, tabla_segmentos_patota);
	sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);

	return true;
}

bool funcion_expulsar_tripulante(int tripulante_id){
	sem_wait(&MUTEX_LISTA_TABLAS_SEGMENTOS);
	char letra = 48;
	for(int i=0; i < list_size(lista_tablas_segmentos); i++){
		tabla_segmentacion* tabla_segmentos = (tabla_segmentacion*)list_get(lista_tablas_segmentos, i);

		if(tripulante_id <= tabla_segmentos->ultimo_tripulante){
			for(int j=2; j < list_size(tabla_segmentos->lista_segmentos); j++){
				segmento* segmento_tcb = list_get(tabla_segmentos->lista_segmentos, j);

				if(segmento_tcb->numero_segmento -2 == (tripulante_id - tabla_segmentos->primer_tripulante)){
					log_info(logger, "Se expulso segmento %d", segmento_tcb->numero_segmento);
					eliminar_segmento(segmento_tcb->numero_segmento, tabla_segmentos);

					bool se_elimina_toda_la_tabla = false;
					if(list_size(tabla_segmentos->lista_segmentos) <= 2){ //la tabla de segmentos tiene solo pcb y tareas. Si es 3 o mayor todavia hay tcb
						se_elimina_toda_la_tabla = true;

						segmento* segmento_tareas = list_get(tabla_segmentos->lista_segmentos, 1);
						eliminar_segmento(segmento_tareas->numero_segmento, tabla_segmentos);
						segmento* segmento_pcb = list_get(tabla_segmentos->lista_segmentos, 0);
						eliminar_segmento(segmento_pcb->numero_segmento, tabla_segmentos);

						list_destroy(tabla_segmentos->lista_segmentos);

						list_remove(lista_tablas_segmentos, i);
						free(tabla_segmentos);

						/*void destruir_una_tabla_seg(tabla_segmentacion* tabla){
							free(tabla);
						}
						list_remove_and_destroy_element(lista_tablas_segmentos, i, (void*)destruir_una_tabla_seg);*/
						//list_destroy(tabla_segmentos);

						/*list_destroy_and_destroy_elements(tabla_segmentos->lista_segmentos, (void*)destruir_segmentos);
						//list_remove(lista_tablas_segmentos, i);
						void destruir_una_tabla_seg(tabla_segmentacion* tabla){
							free(tabla);
						}
						list_remove_and_destroy_element(lista_tablas_segmentos, i, (void*)destruir_una_tabla_seg);*/
					}

					/*if(tripulante_id > 9){ //desde el 10 el mapa tira ":" sino
						letra = 55;
					}
					log_info(logger, "letra + tid: %c", letra + tripulante_id);
					item_borrar(nivel, letra + tripulante_id);*/

					sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);

					//unir_espacios_contiguos_libres();
					imprimir_tabla_espacios_de_memoria();
					if(se_elimina_toda_la_tabla == false){
						imprimir_tabla_segmentos_patota(tabla_segmentos);
					}else{
						log_info(logger, "Tabla de segmentos de patota ya no existe");
					}

					return true;
				}
			}
		}
	}
	sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);
	return false;
}

int compactar_memoria(){
    log_info(logger, "Empieza compactacion");
    int listo = ordenar_memoria();
    int listo2 = unir_espacios_contiguos_libres();

    bool compacto_algo = false;

    sem_wait(&MUTEX_TABLA_MEMORIA);
    sem_wait(&MUTEX_LISTA_TABLAS_SEGMENTOS);
    for(int i=0; i < list_size(tabla_espacios_de_memoria)-1; i++){ //-1 para que no se fije en el ultimo espacio que es el libre
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

        if(espacio->libre == true){
        	compacto_algo = true;
        	log_info(logger, "espacio libre econtrado %d", espacio->base);

        	//sem_wait(&MUTEX_LISTA_TABLAS_SEGMENTOS);
        	for(int k=0; k < list_size(lista_tablas_segmentos); k++){
        		tabla_segmentacion* tabla_segmentos = list_get(lista_tablas_segmentos, k);

        		for(int l=0; l < list_size(tabla_segmentos->lista_segmentos); l++){
        			segmento* segmento = list_get(tabla_segmentos->lista_segmentos, l);
            		if (segmento->base > espacio->base){
            			//para segmento->base < espacio->base no hace nada porque significa que la memoria ya los paso y estan antes que el espacio libre
            			segmento->base -= espacio->tam;
            		}
        		}
        	}
        	//sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);
        	list_remove(tabla_espacios_de_memoria, i);

        	//list_remove_and_destroy_element(tabla_espacios_de_memoria, i, (void*)destruir_espacio_memoria);

        	for(int j = i; j < list_size(tabla_espacios_de_memoria); j++){
        		espacio_de_memoria* espacio_temp = list_get(tabla_espacios_de_memoria, j);
        		espacio_temp->base -= espacio->tam;

        		if(j == list_size(tabla_espacios_de_memoria)-1){ //esta recorriendo el ultimo espacio de memoria
        			espacio_temp->tam += espacio->tam; //entonces le amplia el tamanio porque es el espacio libre
        		}
        	}

        	//sem_post(&MUTEX_TABLA_MEMORIA);//MMM CREO Q ESTA MAL. LO VA A HACER MUCHAS VECES.
            //imprimir_tabla_espacios_de_memoria();
        }
    }
    if(compacto_algo == false){
    	log_info(logger, "No hay nada para compactar");
    }
    sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);
    sem_post(&MUTEX_TABLA_MEMORIA);//OK?
    imprimir_tabla_espacios_de_memoria();

    log_info(logger, "Termina compactacion");
    return 1;
}

bool cambiar_estado(int numero_patota, char nuevo_estado, int tid){
	sem_wait(&MUTEX_CAMBIAR_ESTADO);//REEVER SEMAFOROS

	tabla_segmentacion* tabla_segmentos = buscar_tabla_segmentos(numero_patota);
	if(tabla_segmentos != NULL){
		int seg_a_buscar = tid - tabla_segmentos->primer_tripulante;

		for(int k=2; k < list_size(tabla_segmentos->lista_segmentos); k++){
			segmento* segmento = list_get(tabla_segmentos->lista_segmentos, k);

			if(seg_a_buscar == segmento->numero_segmento -2){ //CREO QUE ESTA BIEN PERO NO PROBE MUCHO
				espacio_de_memoria* espacio = buscar_espacio(segmento); //encontro la base en memoria del tcb del tripulante
				tcbTripulante* tripulante = espacio->contenido;
				tripulante->estado = nuevo_estado;
				espacio->contenido = tripulante;
				sem_post(&MUTEX_CAMBIAR_ESTADO);
				return true;
			}
		}
	}
	sem_post(&MUTEX_CAMBIAR_ESTADO);
	return false;
}

bool enviar_tarea_segmentacion(int socket_cliente, int numero_patota, int id_tripulante){
	sem_wait(&MUTEX_PEDIR_TAREA); //REEVER ESTOS SEMAFOROS

	tabla_segmentacion* tabla_segmentos = buscar_tabla_segmentos(numero_patota);

	if(tabla_segmentos != NULL){
		for(int k=2; k < list_size(tabla_segmentos->lista_segmentos); k++){ //busco segmento del tripulante
			segmento* segmento_tcb = list_get(tabla_segmentos->lista_segmentos, k);

			int seg_a_buscar = id_tripulante - tabla_segmentos->primer_tripulante;
			if(seg_a_buscar == segmento_tcb->numero_segmento -2){ //CREO QUE ESTA BIEN PERO NO PROBE MUCHO

				espacio_de_memoria* espacio_tcb = buscar_espacio(segmento_tcb); //encontro la base en memoria del tcb del tripulante
				if(espacio_tcb != NULL){
					tcbTripulante* tripulante = espacio_tcb->contenido;

					segmento* segmento_tarea = list_get(tabla_segmentos->lista_segmentos, 1); // 1 es el segmento de tareas
					espacio_de_memoria* espacio = buscar_espacio(segmento_tarea);

					if(espacio != NULL){ //encontro la base en memoria de las tareas
						char* una_tarea;
						char** tareas = string_split(espacio->contenido,"-");
						una_tarea = tareas[tripulante->prox_instruccion];
						if(una_tarea == NULL){
							log_info(logger, "no hay mas tareas");
							char* mensaje = "no hay mas tareas";
							enviar_mensaje(mensaje, socket_cliente);
							limpiar_array(tareas);
							sem_post(&MUTEX_PEDIR_TAREA);
							return false;
						}

						enviar_mensaje_malloqueado(una_tarea, socket_cliente);
						limpiar_array(tareas);
					}

					tripulante->prox_instruccion++;

					if(tripulante->estado != 'E'){
						tripulante->estado = 'R';
					}

					espacio_tcb->contenido = tripulante;
				}
			}
		}
	}
	sem_post(&MUTEX_PEDIR_TAREA);
	return true;
}

espacio_de_memoria* buscar_espacio(segmento* segmento){
	sem_wait(&MUTEX_TABLA_MEMORIA);
	for(int j=0; j < list_size(tabla_espacios_de_memoria); j++){
		espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, j);

		if(espacio->base == segmento->base){ //encontro la base del segmento en memoria
			sem_post(&MUTEX_TABLA_MEMORIA);
			return espacio;
		}
	}
	sem_post(&MUTEX_TABLA_MEMORIA);
	return NULL;
}

tabla_segmentacion* buscar_tabla_segmentos(int numero_patota){
	sem_wait(&MUTEX_LISTA_TABLAS_SEGMENTOS);
	for(int i=0; i < list_size(lista_tablas_segmentos); i++){
		tabla_segmentacion* tabla_segmentos = list_get(lista_tablas_segmentos, i);

		if(tabla_segmentos->id_patota == numero_patota){
			sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);
			return tabla_segmentos;
		}
	}
	sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);
	return NULL;
}

char* buscar_tarea(espacio_de_memoria* espacio, int prox_instruccion){ //LA PUEDO BORRAR PORQUE NO LA USO
	char* una_tarea = malloc(30);
	//char* una_tarea;

	char** tareas = string_split(espacio->contenido,"-");
	una_tarea = tareas[prox_instruccion];
	if(una_tarea == NULL){
		log_info(logger, "no hay mas tareas");
		limpiar_array(tareas);
		//strcpy(una_tarea, "no hay mas tareas");
		free(una_tarea);
		return NULL;
		//return una_tarea;
	}

	limpiar_array(tareas);
	return una_tarea;
}

void limpiar_array(char** array){
	int i = 0;
	while(array[i] != NULL){
		free(array[i]);
		i++;
	}
	free(array[i]);
	free(array);
}

bool cambiar_posicion(int tid, int posx, int posy, int pid){
	sem_wait(&MUTEX_CAMBIAR_POSICION);
	char letra = 48;
	tabla_segmentacion* tabla_segmentos = buscar_tabla_segmentos(pid);

	if(tabla_segmentos != NULL){
		int seg_a_buscar = tid - tabla_segmentos->primer_tripulante;

		for(int k=2; k < list_size(tabla_segmentos->lista_segmentos); k++){
			segmento* segmento = list_get(tabla_segmentos->lista_segmentos, k);

			if(seg_a_buscar == segmento->numero_segmento -2){ //CREO QUE ESTA BIEN PERO NO PROBE MUCHO
				espacio_de_memoria* espacio = buscar_espacio(segmento); //encontro la base en memoria del tcb del tripulante
				tcbTripulante* tripulante = espacio->contenido;

				int posx_vieja = tripulante->posicionX;
				int posy_vieja = tripulante->posicionY;

				int difx = posx - posx_vieja;
				int dify = posy - posy_vieja;

				tripulante->posicionX = posx;
				tripulante->posicionY = posy;

				espacio->contenido = tripulante;

				/*if(tid > 9){ //desde el 10 el mapa tira ":" sino
					letra = 55;
				}
			    item_desplazar(nivel, letra+ tid, difx, dify);
			    sleep(1);*/

				sem_post(&MUTEX_CAMBIAR_POSICION);
				return true;
			}
		}
	}

	sem_post(&MUTEX_CAMBIAR_POSICION);
	return false;
}

void sig_handler(int signum){
    if (signum == SIGUSR2){
    	log_info(logger, "SIGUSR2\n");
        int compact = compactar_memoria();
    }
    if(signum == SIGUSR1){
    	log_info(logger, "SIGUSR1\n");
    	if(strcmp(configuracion.squema_memoria, "SEGMENTACION") == 0){
    		dump_memoria_segmentacion();
    	}else{
    		dump_memoria_paginacion();
    	}
    }
}

void dump_memoria_segmentacion(){
	char buff1[50];
	char buff2[50];
	char namebuff[100];
	time_t now = contador_lru;
	strftime(buff1, 100, "%d/%m/%Y %H:%M:%S"  , localtime(&now)); //Tmestamp a escribir en el archivo
	strftime(buff2, 100, "%d_%m_%Y_%H_%M_%S"  , localtime(&now)); //Timestamp del nombre del archivo
	sprintf(namebuff, "Dump_%s.dmp",buff2);
	FILE *dump_file;
	dump_file=fopen(namebuff, "w");
	fflush(stdout);
	fprintf(dump_file,"Dump: %s\n", buff1);

	for(int i=0; i < list_size(lista_tablas_segmentos); i++){
		tabla_segmentacion* tabla_seg = (tabla_segmentacion*)list_get(lista_tablas_segmentos, i);
		for(int j=0; j < list_size(tabla_seg->lista_segmentos); j++){
			segmento* seg = (segmento*)list_get(tabla_seg->lista_segmentos, j);
			if(seg->base < 16){
				fprintf(dump_file, "Patota:%2d   Segmento:%2d   Inicio: 0x000%X   Tam: %d b\n", tabla_seg->id_patota, seg->numero_segmento, seg->base, seg->tamanio);
			}else if(seg->base < 256){
				fprintf(dump_file, "Patota:%2d   Segmento:%2d   Inicio: 0x00%X   Tam: %d b\n", tabla_seg->id_patota, seg->numero_segmento, seg->base, seg->tamanio);
			}else{
				fprintf(dump_file, "Patota:%2d   Segmento:%2d   Inicio: 0x0%X   Tam: %d b\n", tabla_seg->id_patota, seg->numero_segmento, seg->base, seg->tamanio);
			}
			fflush(stdout);
		}
	}

	fclose(dump_file);
}

void destruir_marcos(marco* marcos){
	free(marcos);
}

void destruir_tabla(tabla_paginacion* una_tabla){
	list_destroy_and_destroy_elements(una_tabla->lista_marcos, (void*) destruir_marcos);
}

void destruir_lista_paquete(char* contenido){
	free(contenido);
}
void destruir_segmentos(segmento* seg){
	free(seg);
}
void destruir_espacio_memoria(espacio_de_memoria* espacio){
	//free(espacio->contenido);
	void* content = espacio->contenido;
	free(content);
	free(espacio);
}
void destruir_tabla_segmentacion(tabla_segmentacion* tabla){ //ESTARA BIEN???
	list_destroy_and_destroy_elements(tabla->lista_segmentos, (void*)destruir_segmentos);
	//free(tabla->lista_segmentos);
	free(tabla);
}


////////////PAGINACION -------------------
//Funcion cliente para paginacion
int funcion_cliente_paginacion(int socket_cliente){
	t_list* lista = list_create();
	pcbPatota* patota;
	tcbTripulante* tripulante;
	t_paquete* paquete;
	int tipoMensajeRecibido = -1;
	log_info(logger,"Se conecto este socket a mi %d\n",socket_cliente);
	fflush(stdout);

	while(1){

		tipoMensajeRecibido = recibir_operacion(socket_cliente);
		switch(tipoMensajeRecibido)
		{
			case INICIAR_PATOTA:;
				lista=recibir_paquete(socket_cliente);
				int pid = (int)atoi(list_get(lista,0));
				log_info(logger, "PID es %d",pid);

				int cantidad_tripulantes = (int)atoi((list_get(lista,1)));

				char* tarea=(char*)list_get(lista, cantidad_tripulantes+3);
				strcat(tarea,".");

				int cuantos_marcos_necesito = cuantos_marcos2(cantidad_tripulantes,strlen(tarea)+1)  ;
				log_info(logger, "Ingresa nueva patota, necesito %d marcos", cuantos_marcos_necesito);
				if(cuantos_marcos_necesito > (cuantos_marcos_libres(&configuracion))+ espacios_swap_libres(&configuracion)){
					log_info(logger,"No hay suficientes marcos, no se puede almacenar nada mas");
					//ack a discordiador
				}else{
					int posicion_libre = -1;
					log_info(logger, "Guardando info");
					tabla_paginacion* una_tabla=malloc(sizeof(tabla_paginacion));
					una_tabla->id_patota = pid;
					una_tabla->cant_tripulantes= cantidad_tripulantes;
					una_tabla->long_tareas = strlen(tarea)+1;
					una_tabla->lista_marcos=list_create();
					list_add(tabla_paginacion_ppal, una_tabla);


					for(int i=0 ; i<cuantos_marcos_necesito; i++){
						sem_wait(&MUTEX_MEM_PRINCIPAL);
						posicion_libre = posicion_marco();
						sem_post(&MUTEX_MEM_PRINCIPAL);
						log_info(logger, "Asigne esta posicion %d\n", posicion_libre);
						marco* marco_nuevo = malloc (sizeof(marco));
						marco_nuevo->id_marco=posicion_libre;
						marco_nuevo->ubicacion=MEM_PRINCIPAL;
						contador_lru++;
						marco_nuevo->bit_uso=1;
						marco_nuevo->ultimo_uso = contador_lru;
						list_add(una_tabla->lista_marcos,marco_nuevo);

					}
					sem_wait(&MUTEX_MEM_PRINCIPAL);
					almacenar_informacion2(&configuracion,lista, pid);
					sem_post(&MUTEX_MEM_PRINCIPAL);
					char* mensaje = "Memoria asignada";
					enviar_mensaje(mensaje, socket_cliente);
					imprimir_tabla_paginacion();

				}

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;


			case EXPULSAR_TRIPULANTE:;
				lista = recibir_paquete(socket_cliente);


				int tripulante_id_2 = (int)list_get(lista,0);
				//free(tid_char);
				int patota_id_2 = (int)list_get(lista,1);
				log_info(logger, "tid %d  pid %d", tripulante_id_2, patota_id_2);



				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);
				//return 0;//para que termine el hilo se supone
				break;

			case PEDIR_TAREA:;

				lista = recibir_paquete(socket_cliente);

				int tripulante_id = (int)atoi(list_get(lista, 0));
				int patota_id = (int)atoi(list_get(lista, 1));
				sem_wait(&MUTEX_MEM_PRINCIPAL);

				tcbTripulante* tripulante1 = obtener_tripulante2(patota_id, tripulante_id, &configuracion);

				log_info(logger,"El tripulante que recibi es el %d\n y %d", tripulante_id, patota_id);
				log_info(logger,"El tripulante que recibi eSSSSSs el %d\n y %d", tripulante1->tid, tripulante1->prox_instruccion);
				fflush(stdout);
				bool hay_mas_tareas = enviar_tarea_paginacion(socket_cliente, patota_id, tripulante1);

				if(hay_mas_tareas == false){
				//	tabla_paginacion* una_tabla= (tabla_paginacion*)list_get(tabla_paginacion_ppal, posicion_patota(patota_id, tabla_paginacion_ppal));
				//	una_tabla->cant_tripulantes--;
				//	if(una_tabla->cant_tripulantes == 0){
				//		list_remove_and_destroy_element(tabla_paginacion_ppal, posicion_patota(patota_id, tabla_paginacion_ppal),(void*)destruir_tabla );
				//	}
				}

				sem_post(&MUTEX_MEM_PRINCIPAL);

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);


				break;

			case CAMBIAR_DE_ESTADO:;
				lista = recibir_paquete(socket_cliente);
				tripulante_id = (int)atoi(list_get(lista, 0));

				char* estado_recibido = list_get(lista, 1);
				char estado = estado_recibido[0];

				patota_id = (int)atoi(list_get(lista, 2));
				sem_wait(&MUTEX_MEM_PRINCIPAL);
				tcbTripulante* tripulante2 = obtener_tripulante2(patota_id, tripulante_id, &configuracion);
				tripulante2->estado=estado;
				actualizar_tripulante(tripulante2,patota_id,&configuracion);
				sem_post(&MUTEX_MEM_PRINCIPAL);



				char* mensaje1 = "cambio de estado exitoso";
				enviar_mensaje(mensaje1, socket_cliente);


				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;

			case INFORMAR_MOVIMIENTO:;
				lista = recibir_paquete(socket_cliente);
				tripulante_id = (int)atoi(list_get(lista, 0));
				int posx = (int)atoi(list_get(lista, 1));
				int posy = (int)atoi(list_get(lista, 2));
				patota_id = (int)atoi(list_get(lista, 3));
				sem_wait(&MUTEX_MEM_PRINCIPAL);
				tcbTripulante* tripulante3 = obtener_tripulante2(patota_id, tripulante_id, &configuracion);
				sem_post(&MUTEX_MEM_PRINCIPAL);
				tripulante3->posicionX=posx;
				tripulante3->posicionY=posy;
				actualizar_tripulante(tripulante3,patota_id,&configuracion);
				char* mensaje2 = "cambio de posicion exitoso";
				enviar_mensaje(mensaje2, socket_cliente);


				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;

			case LISTAR_TRIPULANTES:;

				t_paquete* paquete = crear_paquete(LISTAR_TRIPULANTES);
				if(paquete->buffer->size == 0){ //no habia TCBs para mandar
					enviar_header(NO_HAY_NADA_PARA_LISTAR , socket_cliente);
				}else{
					enviar_paquete(paquete, socket_cliente);
				}

				eliminar_paquete(paquete);

				break;

			case FIN:
				log_error(logger, "el discordiador finalizo el programa. Terminando servidor");
				variable_servidor = 0;
				//nivel_destruir(nivel);
				//nivel_gui_terminar();

				//numero_mapa=1;
				shutdown(socket_servidor, SHUT_RD);
				close(socket_cliente);
				return EXIT_FAILURE;

			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				return EXIT_FAILURE;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
		}
	}

	list_clean(lista);
	list_destroy(lista);

}

tcbTripulante* obtener_tripulante2(int patota_id,int tripulante_id, config_struct* config_servidor){
	tcbTripulante* un_tripu= malloc(sizeof(tcbTripulante));

	tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,posicion_patota(patota_id, tabla_paginacion_ppal));

	log_info(logger,"POS VECTOR %d", posicion_vector(tripulante_id-1));

	int ptro_tarea= 2*sizeof(uint32_t)+posicion_vector(tripulante_id-1)*21;

	int indice_marco = 0;

	float decimal = (float)(uint32_t)ptro_tarea/configuracion.tamanio_pag;

	int parte_entera = (uint32_t)ptro_tarea/configuracion.tamanio_pag;
	float parte_decimal = (decimal - parte_entera);
	indice_marco=parte_entera;
	uint32_t offset = configuracion.tamanio_pag*(parte_decimal);
	log_info(logger,"El indice %d", indice_marco);
	log_info(logger,"\nEl offset %d", offset);

	marco* marcos =  (marco*)list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;
	log_info(logger,"MARCO %d",marcos->id_marco);
	fflush(stdout);
	log_info(logger,"OFFSET %d",offset);
	fflush(stdout);

	uint32_t tid;

	leer_atributo_cero(&tid, offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_uno(&tid,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;
	leer_atributo_dos(&tid,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_tres(&tid,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;


	log_info(logger,"TID magico %d",tid);
	fflush(stdout);

	log_info(logger,"MARCO %d",marcos->id_marco);
	fflush(stdout);
	log_info(logger,"OFFSET %d",offset);
	fflush(stdout);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
	marcos =  (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	char estado =*(char*)leer_atributo_char(offset,marcos->id_marco, config_servidor);
	offset+=sizeof(char);
	log_info(logger,"Estado magico %c\n",estado);
	fflush(stdout);

	uint32_t pos_x;
	leer_atributo_cero(&pos_x, offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_uno(&pos_x,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_dos(&pos_x,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
	//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_tres(&pos_x,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;
	log_info(logger,"Pos x magico %d\n",pos_x);
	fflush(stdout);

	uint32_t pos_y;
	leer_atributo_cero(&pos_y, offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
	//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_uno(&pos_y,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
	//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_dos(&pos_y,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_tres(&pos_y,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	log_info(logger,"Pos y magico %d",pos_y);
	fflush(stdout);


	uint32_t prox_in;
	leer_atributo_cero(&prox_in, offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_uno(&prox_in,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_dos(&prox_in,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_tres(&prox_in,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	log_info(logger,"Prox instruccion %d",prox_in);
	fflush(stdout);

	uint32_t puntero_pcb;
	leer_atributo_cero(&puntero_pcb, offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
	//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_uno(&puntero_pcb,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_dos(&puntero_pcb,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;

	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_tres(&puntero_pcb,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	contador_lru++;
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	log_info(logger,"Puntero magico  %d",puntero_pcb);
	fflush(stdout);


	un_tripu->estado=estado;
	un_tripu->posicionX=pos_x;
	un_tripu->posicionY=pos_y;
	un_tripu->prox_instruccion=prox_in;
	un_tripu->puntero_pcb=puntero_pcb;
	un_tripu->tid=tid;




	return un_tripu;

}







void leer_informacion2(config_struct* config_servidor, tabla_paginacion* una_tabla, t_list* lista, int patota_id){

	uint32_t offset = 0;
	uint32_t indice_marco =0;
	for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
			if(una_tabla->id_patota==patota_id){
				tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,i);
				break;
			}
	}


	marco* marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;
	log_info(logger,"MARCO %d\n",marcos->id_marco);
	fflush(stdout);
	log_info(logger,"OFFSET %d\n",offset);
	fflush(stdout);
	uint32_t pid;
	leer_atributo_cero(&pid, offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_uno(&pid,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_dos(&pid,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_tres(&pid,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;


	log_info(logger,"ID magico %d\n",pid);


	fflush(stdout);
	log_info(logger,"MARCO %d\n",marcos->id_marco);
	fflush(stdout);
	log_info(logger,"OFFSET %d\n",offset);
	fflush(stdout);

	uint32_t puntero_tarea;
	leer_atributo_cero(&puntero_tarea, offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_uno(&puntero_tarea,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_dos(&puntero_tarea,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	leer_atributo_tres(&puntero_tarea,offset,marcos->id_marco, config_servidor);
	offset+=1;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;

	log_info(logger,"Puntero magico %d\n",puntero_tarea);
	fflush(stdout);


	for(int i=0; i < una_tabla->cant_tripulantes;i++){

		log_info(logger,"MARCO %d\n",marcos->id_marco);
		fflush(stdout);
		log_info(logger,"OFFSET %d\n",offset);
		fflush(stdout);
		uint32_t tid;
		leer_atributo_cero(&tid, offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_uno(&tid,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_dos(&tid,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_tres(&tid,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		log_info(logger,"TID magico %d\n",tid);
		fflush(stdout);

		log_info(logger,"MARCO %d\n",marcos->id_marco);
		fflush(stdout);
		log_info(logger,"OFFSET %d\n",offset);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos =  (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		char estado =*(char*)leer_atributo_char(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(char);
        log_info(logger,"Estado magico %c\n",estado);
		fflush(stdout);

		uint32_t pos_x;
		leer_atributo_cero(&pos_x, offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_uno(&pos_x,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);


		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_dos(&pos_x,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_tres(&pos_x,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		log_info(logger,"Pos x magico %d\n",pos_x);
		fflush(stdout);

		uint32_t pos_y;
		leer_atributo_cero(&pos_y, offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_uno(&pos_y,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_dos(&pos_y,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_tres(&pos_y,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		log_info(logger,"Pos y magico %d\n",pos_y);
		fflush(stdout);


		uint32_t prox_in;
		leer_atributo_cero(&prox_in, offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_uno(&prox_in,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}

		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_dos(&prox_in,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

	    leer_atributo_tres(&prox_in,offset,marcos->id_marco, config_servidor);
	    offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;


		log_info(logger,"Prox instruccion %d\n",prox_in);
		fflush(stdout);

		uint32_t puntero_pcb;
		leer_atributo_cero(&puntero_pcb, offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_uno(&puntero_pcb,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_dos(&puntero_pcb,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		leer_atributo_tres(&puntero_pcb,offset,marcos->id_marco, config_servidor);
		offset+=1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;

			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;

		log_info(logger,"Puntero magico  %d\n",puntero_pcb);
		fflush(stdout);




	}

	for(int j=0; j <(una_tabla->long_tareas);j++){

		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			contador_lru++;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		marcos->ultimo_uso = contador_lru;
		char valor =*(char*)leer_atributo_char(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(char);
		//log_info(logger,"%c",valor);
		fflush(stdout);

	}

}


void almacenar_informacion2(config_struct* config_servidor, t_list* lista, int patota_id){

	uint32_t offset = 0;
	uint32_t indice_marco=0;
	uint32_t pid = (uint32_t)atoi(list_get(lista,0));

	tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,posicion_patota(patota_id,tabla_paginacion_ppal));

/////////////////////////////////////////PID/////////////////////////////////////////////////////////////////////////////
	//tabla_paginacion* una_tabla = list_get(una_tabla,posicion_patota(pid, una_tabla));
	marco* marcos =(marco*)list_get(una_tabla->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_cero(pid,offset,marcos->id_marco, config_servidor);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}

	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_uno(pid,offset,marcos->id_marco, config_servidor);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_dos(pid,offset,marcos->id_marco, config_servidor);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_tres(pid,offset,marcos->id_marco, config_servidor);

/////////////////////////////////////////Puntero tareas/////////////////////////////////////////////////////////////////////////////

	uint32_t cantidad_tripulantes = (uint32_t)atoi((list_get(lista,1)));
	uint32_t puntero_tarea = cantidad_tripulantes*21 + 8;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_cero(puntero_tarea,offset,marcos->id_marco, config_servidor);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){

		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_uno(puntero_tarea,offset,marcos->id_marco, config_servidor);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_dos(puntero_tarea,offset,marcos->id_marco, config_servidor);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		contador_lru++;
		marcos->bit_uso=1;
		marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_tres(puntero_tarea,offset,marcos->id_marco, config_servidor);

/////////////////////////////////////////Tripulantes/////////////////////////////////////////////////////////////////////////////

	int cant_tripu=0;
	char* tarea=(char*)list_get(lista, cantidad_tripulantes+3);
	for(int i =2; i<cantidad_tripulantes+2;i++){
/////////////////////////////////////////TID/////////////////////////////////////////////////////////////////////////////
		tcbTripulante* tripulante= (tcbTripulante*) list_get(lista,i);
		uint32_t tid = tripulante->tid;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_cero(tid,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_uno(tid,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_dos(tid,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_tres(tid,offset,marcos->id_marco, config_servidor);
/////////////////////////////////////////Estado/////////////////////////////////////////////////////////////////////////////

		char estado = tripulante->estado;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_char(tripulante,offset,marcos->id_marco, config_servidor);
/////////////////////////////////////////Pos_x/////////////////////////////////////////////////////////////////////////////

		uint32_t pos_x = tripulante->posicionX;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_cero(pos_x,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_uno(pos_x,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}

		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_dos(pos_x,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_tres(pos_x,offset,marcos->id_marco, config_servidor);
/////////////////////////////////////////Pos_y/////////////////////////////////////////////////////////////////////////////

		uint32_t pos_y =tripulante->posicionY;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_cero(pos_y,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_uno(pos_y,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_dos(pos_y,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}

		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_tres(pos_y,offset,marcos->id_marco, config_servidor);
/////////////////////////////////////////Proxima instruccion/////////////////////////////////////////////////////////////////////////////
		uint32_t prox_i = puntero_tarea;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}

		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_cero(prox_i,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_uno(prox_i,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_dos(prox_i,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_tres(prox_i,offset,marcos->id_marco, config_servidor);
/////////////////////////////////////////Puntero pcb/////////////////////////////////////////////////////////////////////////////

		uint32_t p_pcb =1;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_cero(p_pcb,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_uno(p_pcb,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;

		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_dos(p_pcb,offset,marcos->id_marco, config_servidor);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);

		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;
		marcos->ultimo_uso = contador_lru;
		offset +=escribir_atributo_tres(p_pcb,offset,marcos->id_marco, config_servidor);


		list_add(configuracion.conversion_marcos,(void*)cant_tripu);
		cant_tripu++;


	}


	for(int i=0;i<strlen(tarea)+1;i++){
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(marcos->ubicacion==MEM_SECUNDARIA){
			log_info(logger,"ENTRE EN MS");
			int numBloque=marcos->id_marco;
			marcos->id_marco=swap_a_memoria(numBloque);
			marcos->ubicacion=MEM_PRINCIPAL;
			marcos->bit_uso=1;
			marcos->ultimo_uso=contador_lru;
			//list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)marcos);
		}
		marcos->bit_uso=1;
		contador_lru++;
		marcos->ultimo_uso = contador_lru;
		offset +=escribir_char_tarea(tarea[i],offset,marcos->id_marco, config_servidor);
	}


}

int posicion_vector(int tripulante_id){
	int id_local;
	id_local=(int)list_get(configuracion.conversion_marcos, tripulante_id);

	return id_local;
}


/*
void buscar_tripulante(int id_patota, int nro_tripulante, tabla_paginacion tabla_pag, config_struct* config){
  //Leer tripulante de memoria
  tabla_paginacion* aux= list_get(tabla_pag, posicion_patota(id_patota, tabla_pag));
  int offset= 2*sizeof(int);
  int indice_calculado = offset/(atoi(config->tamanio_pag));
  marco* marco_leido = list_get(aux->lista_marcos,indice_calculado);
  salto_tripulante(nro_tripulante);
  leer_tripulante(nro_tripulante); //obtengo tripulante
*/
int alcanza_espacio(int* offset,int tamanio_marco, int tipo_dato){
	if(tamanio_marco >= (*offset)+tipo_dato){

		return 0;
	}
	(*offset)=0;
	return 1;
}






int escribir_atributo_cero(uint32_t dato, int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,(void*)&dato,1);
	return 1;
}


int escribir_atributo_uno(uint32_t dato, int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,(void*)&dato + 1,1);
	return 1;
}

int escribir_atributo_dos(uint32_t dato, int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,(void*)&dato + 2 ,1);
	return 1;
}

int escribir_atributo_tres(uint32_t dato, int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,(void*)&dato + 3,1);
	return 1;
}


void leer_atributo_cero(void* dato, int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy((void*)(dato),(void*)p+desplazamiento,1);
}

void leer_atributo_uno(void* dato,int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy((void*)(dato)+1,(void*)p+desplazamiento ,1);
}
void leer_atributo_dos(void* dato,int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy((void*)(dato)+2,(void*) p+desplazamiento ,1);
}
void leer_atributo_tres(void* dato,int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy((void*)dato +3,(void*)p+desplazamiento ,1);
}

void* leer_atributo(int offset, int nro_marco, config_struct* config_s){
	void* dato =malloc(sizeof(int));
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(&dato,p+desplazamiento,sizeof(int));
	return dato;
}



int escribir_atributo_char(tcbTripulante* tripulante, int offset, int nro_marco, config_struct* config_s){
	char* estado= malloc(sizeof(char)+1);
	sprintf(estado,"%c",tripulante->estado);
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,estado,sizeof(char));
	return sizeof(char);
}

int escribir_char_tarea(char caracter, int offset, int nro_marco, config_struct* config_s){
	char* valor= malloc(sizeof(char)+1);
	sprintf(valor,"%c",caracter);
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,valor,sizeof(char));
	return sizeof(char);
}


void* leer_atributo_char(int offset, int nro_marco, config_struct* config_s){
	void* dato =malloc(sizeof(char)+1);
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(dato,p+desplazamiento,sizeof(char));
	return dato;
}



int posicion_marco(){

	for(int i=0;i<configuracion.cant_marcos;i++){
		if((int)(list_get(configuracion.marcos_libres, i))==0){
			int valor = 1;
			list_add_in_index(configuracion.marcos_libres, i, (void*)valor);
			return i;
		}

	}
	//Si no hay marcos libres en memoria principal, ejecuto el algoritmo de reemplazo
	if(strcmp(configuracion.algoritmo_reemplazo,"LRU")==0){
		log_info(logger,"Algoritmo de reemplazo es LRU\n");
		return reemplazo_lru();
	}else{
		log_info(logger,"Algoritmo de reemplazo es clock\n");
		return reemplazo_clock();
	}



}




int cuantos_marcos_libres(config_struct* config_servidor){
	int contador_marcos = 0;
	for(int i=0;i<config_servidor->cant_marcos;i++){
		if((int)(list_get(config_servidor->marcos_libres, i))==0){
			contador_marcos++;
		}

	}
	return contador_marcos;
}

void imprimir_ocupacion_marcos(config_struct* configuracion){
	log_info(logger,"Marcos ocupados:\n");
	for(int i=0; i<(configuracion->cant_marcos);i++){
			log_info(logger,"|%d|",(int) list_get(configuracion->marcos_libres,i));
		}
	log_info(logger,"\n");
}

int posicion_patota(int id_buscado,t_list* tabla_aux){
	for(int i=0; i<list_size(tabla_aux);i++){
		tabla_paginacion* auxiliar= (tabla_paginacion*)list_get(tabla_aux, i);
		if(auxiliar->id_patota==id_buscado){
			return i;
		}
	}
	return -1;

	}




int por_atributo(int nuevo_marco,int *tamanio_marco, int tipo_dato, int cantidad_marcos){


	*tamanio_marco = *tamanio_marco - tipo_dato;

	while(*tamanio_marco<0){
		cantidad_marcos++;
		*tamanio_marco = nuevo_marco;
		*tamanio_marco = *tamanio_marco - tipo_dato;
	}
	return cantidad_marcos;
}

int cuantos_marcos2(int cuantos_tripulantes, int longitud_tarea){
	float tamanio_marco =  configuracion.tamanio_pag;
	float tamanio_necesario = cuantos_tripulantes*(5*sizeof(uint32_t))+sizeof(char)+longitud_tarea+(2*sizeof(uint32_t));
	float parte_decimal = (tamanio_necesario + (tamanio_marco/2)) / tamanio_marco;
	int parte_entera =(int) (tamanio_necesario + (tamanio_marco/2)) / tamanio_marco;
	if(parte_entera-parte_decimal == 0){
		return parte_entera;
	}else{
		return parte_entera+1;
	}
}


void dump_memoria_paginacion(){
	char buff1[50];
	char buff2[50];
	char namebuff[100];
	time_t now = contador_lru;
	strftime(buff1, 100, "%d/%m/%Y %H:%M:%S"  , localtime(&now)); //TImestamp a escribir en el archivo
	strftime(buff2, 100, "%d_%m_%Y_%H_%M_%S"  , localtime(&now)); //Timestamp del nombre del archivo
	sprintf(namebuff, "Dump_%s.dmp",buff2);
	FILE *dump_file;
	dump_file=fopen(namebuff, "w");
	fflush(stdout);
	fprintf(dump_file,"Dump: %s\n", buff1);
	int estado,proceso,pagina;
	for(int k=0;k<configuracion.cant_marcos;k++){
		buscar_marco(k,&estado,&proceso, &pagina);
		if(estado==1){
			fprintf(dump_file,"Marco:%2d    Estado:Ocupado    Proceso:%2d    Pagina:%2d\n",k,proceso,pagina);
			fflush(stdout);
		}else{
			fprintf(dump_file,"Marco:%2d    Estado:Libre      Proceso: -    Pagina: -\n",k);
			fflush(stdout);

		}
	}
	fclose(dump_file);
}





void buscar_marco(int id_marco,int * estado,int* proceso, int *pagina){
	sem_wait(&MUTEX_LISTA_TABLAS_PAGINAS);
	*estado = 0 ;
	for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
		tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,i);
		for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
			marco* un_marco=list_get(una_tabla->lista_marcos,j);
			if(un_marco->id_marco==id_marco && un_marco->ubicacion==MEM_PRINCIPAL){
				char buff1[100];
			//	strftime(buff1, 100, "%d/%m/%Y %H:%M:%S"  , localtime(&un_marco->ultimo_uso));
				//log_info(logger,"Ultimo uso %s\n", buff1);
				 *estado = 1;
				 *proceso=una_tabla->id_patota;
				 *pagina=j;
				 break;
			}
				if(*estado==1){
					break;
				}

				*estado=0;
				*proceso=0;
				*pagina=0;

		}

	}
	sem_post(&MUTEX_LISTA_TABLAS_PAGINAS);
}


void imprimir_tabla_paginacion(){
	sem_wait(&MUTEX_LISTA_TABLAS_PAGINAS);
	for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
		tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,i);
		log_info(logger,"Patota %d\n",una_tabla->id_patota);
		for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
			marco* un_marco=list_get(una_tabla->lista_marcos,j);
			log_info(logger,"--------");
			log_info(logger,"ID MARCO %d\n",j);
			log_info(logger,"Uso %d\n",un_marco->bit_uso );
			//log_info(logger,"Libre %d\n",un_marco->libre );
			if(un_marco->ubicacion==MEM_SECUNDARIA){
				log_info(logger,"Esta en memoria secundaria");
			}else{
				log_info(logger,"No lo esta");

			}
			log_info(logger,"--------");

			//log_info(logger,"Time %f\n",un_marco->ultimo_uso);
			}
	}
	sem_post(&MUTEX_LISTA_TABLAS_PAGINAS);
}

void swap_pagina_iniciar(){
	log_info(logger, "Inicializo el archivo");
	FILE* swapfile=fopen("swap.bin","wb");
	int swap_frames = 16384/32;
	void * leido = calloc(swap_frames, 1);
	fwrite(leido, swap_frames, 1, swapfile);
	fclose(swapfile);
}


void swap_pagina(void* contenidoAEscribir,int numDeBloque){
	mem_hexdump(contenidoAEscribir, configuracion.tamanio_pag);
	log_info(logger, "Swappeando %d", numDeBloque);
	FILE* swapfile;
	swapfile = fopen("swap.bin","rb+");
	printf("\n el numero de bloque es %d \n",numDeBloque);
	fseek(swapfile,numDeBloque*configuracion.tamanio_pag*sizeof(char),SEEK_SET);
	//char* aux = malloc(configuracion.tamanio_pag*sizeof(char));
	//aux=completarBloque(contenidoAEscribir);
	void * leido = calloc(configuracion.tamanio_pag, 1);
	fwrite(leido, sizeof(configuracion.tamanio_pag), 1, swapfile);

	fwrite(contenidoAEscribir,sizeof(char)*configuracion.tamanio_pag,1,swapfile);
	//free(aux);
	fclose(swapfile);
}

int swap_a_memoria(int numBloque){

	log_info(logger, "Recupero de %d", numBloque);
	int nuevo_marco=posicion_marco();
	void * leido = calloc(configuracion.tamanio_pag, 1);
	leido = recuperar_pag_swap(numBloque);
	mem_hexdump((void*)leido, configuracion.tamanio_pag);
	memcpy(configuracion.posicion_inicial+(nuevo_marco*configuracion.tamanio_pag),leido,configuracion.tamanio_pag*sizeof(char));
	free(leido);
	return nuevo_marco;

}




void* recuperar_pag_swap(int numDeBloque){
	log_info(logger, "Recuperando este bloque de swap %d", numDeBloque);
	FILE* swapfile = fopen("swap.bin","r");
	void * leido = calloc(configuracion.tamanio_pag, 1);
	fseek(swapfile,(numDeBloque*configuracion.tamanio_pag),SEEK_SET);
	fread(leido,configuracion.tamanio_pag,1,swapfile);
	//char* aux = vaciar_bloque(leido);
	fclose(swapfile);
	//free(leido);
	return leido;
}


int reemplazo_lru(){
	   int nro_marco=0;
	   int pagina=0;
	   int marco_patota=0;
	   contador_lru++;
	   int lru_actual = contador_lru;
	   marco* lru_m=malloc(sizeof(marco));
	   for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
		   tabla_paginacion* una_tabla =(tabla_paginacion*)list_get(tabla_paginacion_ppal,i);
		   for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
			   marco* un_marco=(marco*)list_get(una_tabla->lista_marcos,j);
			   if(un_marco->ultimo_uso < lru_actual && un_marco->ubicacion==MEM_PRINCIPAL){
				   lru_actual = un_marco->ultimo_uso;
		   		   lru_m = un_marco;
		   		   pagina=i;
		   		   marco_patota=j;
		   		}
		   	}
	   }

	   lru_m->ubicacion=MEM_SECUNDARIA;
	   nro_marco=lru_m->id_marco;
	   log_info(logger, "El marco elegido para reemplazar es %d", lru_m->id_marco);
	   void* contenidoAEscribir = malloc(configuracion.tamanio_pag);
	//   mem_hexdump(contenidoAEscribir, configuracion.tamanio_pag);
	   log_info(logger,"Memoria %d",(int) contenidoAEscribir);
	   memcpy(contenidoAEscribir,configuracion.posicion_inicial + nro_marco*(configuracion.tamanio_pag) ,configuracion.tamanio_pag*sizeof(char));
	   lru_m->id_marco=lugar_swap_libre();
	   log_info(logger,"EL NUMERO DE BLOQUE ES %d",lru_m->id_marco);
	   swap_pagina(contenidoAEscribir,lru_m->id_marco);

	  // tabla_paginacion* una_tabla =(tabla_paginacion*)list_get(tabla_paginacion_ppal,pagina);
	 //  list_remove(una_tabla->lista_marcos,marco_patota);
	  //list_add_in_index(una_tabla->lista_marcos, marco_patota,(void*) lru_m);
	  // list_add_in_index(tabla_paginacion_ppal,pagina ,(void*)una_tabla);
	  // free(contenidoAEscribir);
	   return nro_marco;
}


void actualizar_lru(marco* un_marco){
	un_marco->ultimo_uso= contador_lru;


}

int lugar_swap_libre(){
	for(int i=0;i<configuracion.cant_lugares_swap;i++){
		if((int)list_get(configuracion.swap_libre,i)==0){
			int valor = 1;
			list_add_in_index(configuracion.swap_libre,i,(void*)valor);
			return i;
		}
	}
	return -1;
}

int espacios_swap_libres(config_struct* config_servidor){
	int contador_swap = 0;
	sem_wait(&MUTEX_MEM_SEC);
	for(int i=0;i<config_servidor->cant_lugares_swap;i++){
		if((int)(list_get(config_servidor->swap_libre, i))==0){
			contador_swap++;
		}

	}
	sem_post(&MUTEX_MEM_SEC);
	return contador_swap;
}



int reemplazo_clock(){
	int nro_marco;
	int flag =0;
	marco* clock_m=malloc(sizeof(marco));
		for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
			tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,i);
			while(flag !=1){
				for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
					marco* un_marco=list_get(una_tabla->lista_marcos,j);
					if(un_marco->ubicacion == 0){
						if(un_marco->bit_uso==1){
	   		            un_marco->bit_uso=0;
						}else{
							clock_m = un_marco;
							flag=1;
							break;
						}

	   	   }
   }
}break;
}




		   clock_m->ubicacion=MEM_SECUNDARIA;
		   nro_marco=clock_m->id_marco;

		   void* contenidoAEscribir = malloc(configuracion.tamanio_pag);
		   memcpy(contenidoAEscribir,configuracion.posicion_inicial + nro_marco*(configuracion.tamanio_pag) ,configuracion.tamanio_pag);
		   clock_m->id_marco=lugar_swap_libre();
		   swap_pagina(contenidoAEscribir,clock_m->id_marco);

           log_info(logger,"MARCO ENTREGADO POR CLOCK %d \n",  nro_marco);
		   return nro_marco;

}


/*
int reemplazo_clock(){
			log_info(logger,"Estoy en el clock\n");
			int nro_marco;
			int flag =0;
			marco* clock_m=malloc(sizeof(marco));
			sem_wait(&MUTEX_LISTA_TABLAS_PAGINAS);
			for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
				tabla_paginacion* una_tabla = (tabla_paginacion*)list_get(tabla_paginacion_ppal,i);
				while(flag !=1){
					for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
						log_info(logger,"Henos qui");
						marco* un_marco= (marco*)list_get(una_tabla->lista_marcos,j);
						if(un_marco->bit_uso==1 && un_marco->ubicacion==MEM_PRINCIPAL){
			   		       un_marco->bit_uso=0;
			   		        }
					    if(un_marco->bit_uso==0 && un_marco->ubicacion==MEM_PRINCIPAL){
							clock_m = un_marco;
							flag=1;
							break;
							}
			   	   }
		   }
		}
		sem_post(&MUTEX_LISTA_TABLAS_PAGINAS);
		   log_info(logger,"Aun no rompi");
		   clock_m->ubicacion=MEM_SECUNDARIA;
		   nro_marco=clock_m->id_marco;
		   void* contenidoAEscribir = malloc(configuracion.tamanio_pag);
		   memcpy(contenidoAEscribir,configuracion.posicion_inicial + nro_marco*(configuracion.tamanio_pag) ,configuracion.tamanio_pag);
		   log_info(logger,"SI llegue aqui");
		   clock_m->id_marco=lugar_swap_libre();
		   log_info(logger,"Aqui no");
		   swap_pagina(contenidoAEscribir,clock_m->id_marco);
		   log_info(logger,"MARCO ENTREGADO POR CLOCK %d \n",  nro_marco);
		   fflush(stdout);
		   return nro_marco;
}
*/

void actualizar_tripulante(tcbTripulante* tripulante, int id_patota, config_struct* config_servidor){

	int indice_marco = 0;
	int offset=0;
	int nro_marco=0;
	int pos=posicion_patota(id_patota, tabla_paginacion_ppal);
	tabla_paginacion* auxiliar= (tabla_paginacion*)list_get(tabla_paginacion_ppal, pos);


	int ptro_tarea=2*sizeof(uint32_t)+ 21* (posicion_vector(tripulante->tid -1));

	float decimal = (float)ptro_tarea/configuracion.tamanio_pag;

	int parte_entera = (uint32_t)ptro_tarea/configuracion.tamanio_pag;
	float parte_decimal = (decimal - parte_entera);
	indice_marco=parte_entera;
	offset = configuracion.tamanio_pag*(parte_decimal);
	log_info(logger,"El indice %d\n", indice_marco);
	log_info(logger,"\nEl offset %d\n", offset);

	marco* marcos =  (marco*)list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS ");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	log_info(logger,"MARCO %d\n",marcos->id_marco);
	fflush(stdout);
	log_info(logger,"OFFSET %d\n",offset);
	fflush(stdout);
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;


//////////////////////////////////////TID////////////////////////////////////////////////////////////////////////////
    uint32_t tid = tripulante->tid;
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);


	marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
	offset +=escribir_atributo_cero(tid,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_uno(tid,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_dos(tid,offset,marcos->id_marco, config_servidor);


    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_tres(tid,offset,marcos->id_marco, config_servidor);
    //////////////////////////////////////ESTADO////////////////////////////////////////////////////////////////////////////

    char estado = tripulante->estado;
    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));

    marcos = (marco*)list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;

    offset +=escribir_atributo_char(tripulante,offset,marcos->id_marco, config_servidor);

//////////////////////////////////////POSICION X////////////////////////////////////////////////////////////////////////////

    uint32_t pos_x = tripulante->posicionX;
    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);

    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_cero(pos_x,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);

    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_uno(pos_x,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_dos(pos_x,offset,marcos->id_marco, config_servidor);


    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_tres(pos_x,offset,marcos->id_marco, config_servidor);
    //////////////////////////////////////POSICION Y////////////////////////////////////////////////////////////////////////////

    uint32_t pos_y =tripulante->posicionY;
    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_cero(pos_y,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_uno(pos_y,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_dos(pos_y,offset,marcos->id_marco, config_servidor);


    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_tres(pos_y,offset,marcos->id_marco, config_servidor);
    //////////////////////////////////////PROXIMA TAREA////////////////////////////////////////////////////////////////////////////

    uint32_t prox_i = tripulante->prox_instruccion;
    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_cero(prox_i,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_uno(prox_i,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_dos(prox_i,offset,marcos->id_marco, config_servidor);


    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_tres(prox_i,offset,marcos->id_marco, config_servidor);
    //////////////////////////////////////PUNTERO PCB////////////////////////////////////////////////////////////////////////////

    uint32_t p_pcb =tripulante->puntero_pcb;
    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_cero(p_pcb,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_uno(p_pcb,offset,marcos->id_marco, config_servidor);



    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;

	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_dos(p_pcb,offset,marcos->id_marco, config_servidor);


    indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), 1);
    marcos =(marco*) list_get(auxiliar->lista_marcos,indice_marco);
	if(marcos->ubicacion==MEM_SECUNDARIA){
		log_info(logger,"ENTRE EN MS");
		int numBloque=marcos->id_marco;
		marcos->id_marco=swap_a_memoria(numBloque);
		marcos->ubicacion=MEM_PRINCIPAL;
		//list_add_in_index(auxiliar->lista_marcos,indice_marco, (void*)marcos);
	}
	marcos->bit_uso=1;
	contador_lru++;
	marcos->ultimo_uso = contador_lru;
    offset +=escribir_atributo_tres(p_pcb,offset,marcos->id_marco, config_servidor);




}


char* obtener_tarea2(int id_patota, tcbTripulante* tripulante){

	int flag =0;
	char* una_tarea=calloc(100,1);
	int cuantas_letras=0;
	char tarea='a';
	if(tripulante->prox_instruccion == 0){
		return NULL;
	}

	sem_wait(&MUTEX_LISTA_TABLAS_PAGINAS);
	tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,posicion_patota(id_patota,tabla_paginacion_ppal));

	uint32_t puntero_tareas = (uint32_t)tripulante->prox_instruccion;


	log_info(logger,"\nEl punterin %d\n",(uint32_t) puntero_tareas);
	fflush(stdout);
	float decimal = (float)puntero_tareas/configuracion.tamanio_pag;

	int parte_entera = (int)puntero_tareas/configuracion.tamanio_pag;
	float parte_decimal = (decimal - parte_entera);
	int indice_marco=parte_entera;
	uint32_t offset = configuracion.tamanio_pag*(parte_decimal);

	//Traigo el marco al que apunta el puntero de las tareas a memoria

	while(tarea != '-'){

		marco* otro_marco=(marco*)list_get(una_tabla->lista_marcos,indice_marco);
		if(otro_marco->ubicacion == MEM_SECUNDARIA){
			int nuevo_marco = swap_a_memoria(otro_marco->id_marco);
			otro_marco->ubicacion=MEM_PRINCIPAL;
			otro_marco->id_marco=nuevo_marco;
		//	list_add_in_index(una_tabla->lista_marcos,indice_marco, (void*)otro_marco);
		}
		otro_marco->bit_uso=1;
		contador_lru++;
		otro_marco->ultimo_uso = contador_lru;
		tarea=*(char*)leer_atributo_char(offset,otro_marco->id_marco, &configuracion);
		offset+=sizeof(char);
		if(tarea != '-' && tarea != '.'){
			una_tarea=strncat(una_tarea,&tarea,1);
			indice_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(char));
		}
		if(tarea == '.'){
			flag=1;
			actualizar_tripulante(tripulante, id_patota,&configuracion);
			break;
		}
		cuantas_letras++;


	}
	if(flag==0){
		tripulante->prox_instruccion += cuantas_letras;
	}else{
		tripulante->prox_instruccion=0;
	}

	actualizar_tripulante(tripulante, id_patota,&configuracion);
	sem_post(&MUTEX_LISTA_TABLAS_PAGINAS);
	return una_tarea;
}

bool enviar_tarea_paginacion(int socket_cliente, int numero_patota, tcbTripulante* tripulante){
	sem_wait(&MUTEX_PEDIR_TAREA); //REEVER ESTOS SEMAFOROS

	char* una_tarea = obtener_tarea2(numero_patota, tripulante);
	log_info(logger,"\n tareas : %s \n",una_tarea);
	fflush(stdout);
	if(una_tarea == NULL){ //no tiene mas tareas
		char* mensaje = "no hay mas tareas";
		enviar_mensaje(mensaje, socket_cliente);
		sem_post(&MUTEX_PEDIR_TAREA);
		return false;
	}

	enviar_mensaje(una_tarea, socket_cliente);
	free(una_tarea);

	sem_post(&MUTEX_PEDIR_TAREA);
	return true;
}
