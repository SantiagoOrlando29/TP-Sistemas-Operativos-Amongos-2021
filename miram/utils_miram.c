#include "utils_miram.h"

int variable_servidor = -1;
int socket_servidor;
t_list* tabla_paginacion_ppal;

int borrar = 1;

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



	int hilo;
	while(variable_servidor != 0){

		socket_cliente = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);

		if(socket_cliente>0){
			hilo ++ ;
			log_info(logger, "Estableciendo conexión desde %d", dir_cliente.sin_port);
			log_info(logger, "Creando hilo");

			pthread_t hilo_cliente=(char)hilo;
			if(strcmp(configuracion.squema_memoria, "SEGMENTACION")==0){
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente_segmentacion ,(void*)socket_cliente);
			pthread_detach(hilo_cliente);
		    }else{
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente_paginacion ,(void*)socket_cliente);
			pthread_detach(hilo_cliente);

		    }
		}
	}
	//printf("me fui");

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
	int cant_sabotaje = 0;//para probar mongo

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

				if(strcmp(configuracion.squema_memoria,"SEGMENTACION")==0){
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

									agregar_a_paquete(paquete, tripulante, tamanio_TCB);

									char* pid_char = malloc(sizeof(char)+1);
									sprintf(pid_char, "%d", pid);
									agregar_a_paquete(paquete, pid_char, strlen(pid_char)+1);

									//printf("Tripulante: %d     Patota: %d     Status: %c \n", tripulante->tid, pid, tripulante->estado);
									k = list_size(tabla_espacios_de_memoria); //para que corte el for
								}
							}
							sem_post(&MUTEX_TABLA_MEMORIA);
						}
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
				pid_char = list_get(lista,0);
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
					log_info(logger, "fallo cambio estado");
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

			case INFORMAR_BITACORA:; //PARA PROBAR LO DE MONGO
				t_list* lista_pr = recibir_paquete(socket_cliente);
				tripulante_id = (int)atoi(list_get(lista_pr,0));
				char* mens = list_get(lista_pr,1);
				if(list_size(lista) == 3){
					char* mens2 = list_get(lista_pr,2);
					//strcat(mens, mens2);
					log_info(logger, "tid %d  %s%s",tripulante_id, mens, mens2);
				}else{
					log_info(logger, "tid %d  %s",tripulante_id, mens);
				}

				char* mensaje = "ok";
				enviar_mensaje(mensaje, socket_cliente);

				list_clean_and_destroy_elements(lista_pr, (void*)destruir_lista_paquete);
				list_destroy(lista_pr);
				break;

			case TAREA_MONGO:;
				t_list* lista_pru = recibir_paquete(socket_cliente);
				char* nombre_tarea = list_get(lista_pru,0);
				int parametro = (int)atoi(list_get(lista_pru,1));

				log_info(logger, "tarea %s  param %d", nombre_tarea, parametro);

				if(strcmp(nombre_tarea, "GENERAR_OXIGENO") == 0){

				}else if(strcmp(nombre_tarea, "CONSUMIR_OXIGENO") == 0){

				}else if(strcmp(nombre_tarea, "GENERAR_COMIDA") == 0){

				}else if(strcmp(nombre_tarea, "CONSUMIR_COMIDA") == 0){

				}else if(strcmp(nombre_tarea, "GENERAR_BASURA") == 0){

				}else if(strcmp(nombre_tarea, "DESCARTAR_BASURA") == 0){

				}else{
					log_debug(logger, "se recibio una tarea distinta a las 6 de E/S");
				}

				char* mensaje2 = "ok";
				enviar_mensaje(mensaje2, socket_cliente);

				list_clean_and_destroy_elements(lista_pru, (void*)destruir_lista_paquete);
				list_destroy(lista_pru);

				break;

			case FSCK:
				log_info(logger, "en fsck");
				char* mensaje3 = "ok";
				enviar_mensaje(mensaje3, socket_cliente);

				break;

			case PRUEBA://para probar mongo
				log_info(logger, "PRUEBA");

				char* mensaje4 = configuracion.posiciones_sabotaje[cant_sabotaje];
				enviar_mensaje(mensaje4, socket_cliente);

				cant_sabotaje++;

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
					list_destroy_and_destroy_elements(tabla_espacios_de_memoria, (void*)destruir_espacio_memoria);
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

			int nro_marcos =(config_servidor->tamanio_memoria)/(config_servidor->tamanio_pag);
			config_servidor->cant_marcos = nro_marcos;
			printf("\nNumero de marco es: %d nro_marcos\n", nro_marcos);

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

void ordenar_memoria(){
    bool espacio_anterior(espacio_de_memoria* espacio_menor, espacio_de_memoria* espacio_mayor) {
        return espacio_menor->base < espacio_mayor->base;
    }
    sem_wait(&MUTEX_TABLA_MEMORIA);
    list_sort(tabla_espacios_de_memoria, (void*) espacio_anterior);
    sem_post(&MUTEX_TABLA_MEMORIA);
}

void unir_espacios_contiguos_libres(){
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
            	return espacio;
            }
            list_add(espacios_libres, espacio);
        }
    }

    int espacios_libres_size = list_size(espacios_libres);

    if(espacios_libres_size != 0){
    	/*espacio_de_memoria* espacio_best_fit;
        int best_fit_diff = 999999; //revisar esto

        for(int i=0; i < espacios_libres_size; i++){
        	espacio_de_memoria* y = list_get(espacios_libres, i);
            int diff = y->tam - tam;

            if(best_fit_diff > diff){
                best_fit_diff = diff;
                espacio_best_fit = y;
            }
        }*/
    	espacio_de_memoria* espacio_best_fit = list_get(espacios_libres, 0);
    	for(int i=1; i < espacios_libres_size; i++){
			espacio_de_memoria* espacio_a_comparar = list_get(espacios_libres, i);

			if(espacio_a_comparar->tam  <  espacio_best_fit->tam){
				espacio_best_fit = espacio_a_comparar;
			}
		}

        //log_info(logger, "El espacio de memoria encontrado con Best Fit (base:%d)", espacio_best_fit->base);
    	sem_post(&MUTEX_TABLA_MEMORIA);
    	return espacio_best_fit;

    }else{
        log_warning(logger, "No hay espacios de memoria libres");
        sem_post(&MUTEX_TABLA_MEMORIA);
        return NULL;
    }
}

espacio_de_memoria* asignar_espacio_de_memoria(size_t tam) {
	espacio_de_memoria *espacio_libre = buscar_espacio_de_memoria_libre(tam);

	if(espacio_libre == NULL){ //NO PROBE ESTE IF
		log_info(logger, "No hay espacio libre de memoria. Hacemos Compactacion");
		compactar_memoria();
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
            unir_espacios_contiguos_libres();

            return nuevo_espacio_de_memoria;
        }

    } else {
        log_warning(logger, "No se pudo asignar espacio de memoria");
        return NULL;
    }
}

bool patota_segmentacion(int pid, uint32_t cantidad_tripulantes, char* tarea, t_list* lista){
	pcbPatota* pcb_patota = crear_pcb(pid);
	espacio_de_memoria* espacio_de_memoria_pcb_patota = asignar_espacio_de_memoria(tamanio_PCB);
	if (espacio_de_memoria_pcb_patota == NULL){
		return false;
	}

	espacio_de_memoria* espacio_de_memoria_tareas = asignar_espacio_de_memoria(strlen(tarea)+1);
	if (espacio_de_memoria_tareas == NULL){
		eliminar_espacio_de_memoria(espacio_de_memoria_pcb_patota->base);
		return false;
	}
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
		//tcbTripulante* tripulante = malloc(tamanio_TCB);
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
					sem_post(&MUTEX_LISTA_TABLAS_SEGMENTOS);

					unir_espacios_contiguos_libres();
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

void compactar_memoria(){
    log_info(logger, "Empieza compactacion");
    ordenar_memoria();
    unir_espacios_contiguos_libres();

    bool compacto_algo = false;

    sem_wait(&MUTEX_TABLA_MEMORIA);
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
        	//list_remove(tabla_espacios_de_memoria, i);
        	list_remove_and_destroy_element(tabla_espacios_de_memoria, i, (void*)destruir_espacio_memoria);

        	for(int j = i; j < list_size(tabla_espacios_de_memoria); j++){
        		espacio_de_memoria* espacio_temp = list_get(tabla_espacios_de_memoria, j);
        		espacio_temp->base -= espacio->tam;

        		if(j == list_size(tabla_espacios_de_memoria)-1){ //esta recorriendo el ultimo espacio de memoria
        			espacio_temp->tam += espacio->tam; //entonces le amplia el tamanio porque es el espacio libre
        		}
        	}
        	sem_post(&MUTEX_TABLA_MEMORIA);
            imprimir_tabla_espacios_de_memoria();
        }
    }
    if(compacto_algo == false){
    	sem_post(&MUTEX_TABLA_MEMORIA);
    	log_info(logger, "No hay nada para compactar");
    }

    log_info(logger, "Termina compactacion");
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
	tabla_segmentacion* tabla_segmentos = buscar_tabla_segmentos(pid);

	if(tabla_segmentos != NULL){
		int seg_a_buscar = tid - tabla_segmentos->primer_tripulante;

		for(int k=2; k < list_size(tabla_segmentos->lista_segmentos); k++){
			segmento* segmento = list_get(tabla_segmentos->lista_segmentos, k);

			if(seg_a_buscar == segmento->numero_segmento -2){ //CREO QUE ESTA BIEN PERO NO PROBE MUCHO
				espacio_de_memoria* espacio = buscar_espacio(segmento); //encontro la base en memoria del tcb del tripulante
				tcbTripulante* tripulante = espacio->contenido;
				tripulante->posicionX = posx;
				tripulante->posicionY = posy;
				espacio->contenido = tripulante;
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
        compactar_memoria();
    }
    if(signum == SIGUSR1){
    	log_info(logger, "SIGUSR1\n");
    	dump_memoria_segmentacion();
    }
}

void dump_memoria_segmentacion(){
	char buff1[50];
	char buff2[50];
	char namebuff[100];
	time_t now = time(0);
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

void destruir_lista_paquete(char* contenido){
	free(contenido);
}
void destruir_segmentos(segmento* seg){
	free(seg);
}
void destruir_espacio_memoria(espacio_de_memoria* espacio){
	free(espacio->contenido);
	free(espacio);
}
void destruir_tabla_segmentacion(tabla_segmentacion* tabla){ //ESTARA BIEN???
	list_destroy_and_destroy_elements(tabla->lista_segmentos, (void*)destruir_segmentos);
	//free(tabla->lista_segmentos);
	free(tabla);
}




//Funcion cliente para paginacion
int funcion_cliente_paginacion(int socket_cliente){
	t_list* lista = list_create();
	pcbPatota* patota;
	tcbTripulante* tripulante;
	t_paquete* paquete;
	int tipoMensajeRecibido = -1;
	printf("Se conecto este socket a mi %d\n",socket_cliente);
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

				int cuantos_marcos_necesito = cuantos_marcos(cantidad_tripulantes,strlen(tarea)+1,&configuracion);
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
						posicion_libre = posicion_marco(&configuracion);
						log_info(logger, "Asigne esta posicion %d\n", posicion_libre);
						marco* marco_nuevo = malloc (sizeof(marco));
						marco_nuevo->id_marco=posicion_libre;
						marco_nuevo->ubicacion=MEM_PRINCIPAL;
						marco_nuevo->bit_uso=1;
						marco_nuevo->ultimo_uso = time(0);
						list_add(una_tabla->lista_marcos,marco_nuevo);
					}
					char* mensaje = "Memoria asignada";
					enviar_mensaje(mensaje, socket_cliente);

					//int cols, rows;



					almacenar_informacion(&configuracion, una_tabla, lista);
					//leer_informacion(&configuracion,  una_tabla, lista);

					//actualizar_tripulante( tripulante3, 1);

					//leer_informacion(&configuracion,  una_tabla, lista);
					//mem_hexdump(configuracion.posicion_inicial, configuracion.tamanio_pag*sizeof(char));


					log_info(logger,"Tarea %s\n",obtener_tarea(borrar, borrar+1));
					borrar+=borrar;
					imprimir_ocupacion_marcos(&configuracion);
					imprimir_tabla_paginacion();
					dump_memoria_paginacion();
					/*fflush(stdout);



					void* contenidoAEscribir = malloc(configuracion.tamanio_pag*sizeof(char));
					memcpy(contenidoAEscribir,configuracion.posicion_inicial,configuracion.tamanio_pag*sizeof(char));
					free(configuracion.posicion_inicial);
					swap_pagina(contenidoAEscribir,0);
					memcpy(configuracion.posicion_inicial,(recuperar_pag_swap(0)),configuracion.tamanio_pag*sizeof(char));
					mem_hexdump(configuracion.posicion_inicial, configuracion.tamanio_pag*sizeof(char));
					leer_informacion(&configuracion,  una_tabla, lista);

*/

				}


				break;


			case LISTAR_TRIPULANTES:;
				t_paquete* paquete = crear_paquete(LISTAR_TRIPULANTES);
				tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
				agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));
				enviar_paquete(paquete,socket_cliente);
				eliminar_paquete(paquete);
				break;


			/*case INICIAR_PLANIFICACION:
				log_info(logger, "Iniciando planificacion");
				t_paquete* tarea_a_enviar;
				tarea* tarea1 = crear_tarea(GENERAR_OXIGENO,5,2,2,5);
				agregar_a_paquete(tarea_a_enviar, tarea1, sizeof(tarea));
				enviar_paquete(tarea_a_enviar,socket_cliente);
				eliminar_paquete(tarea_a_enviar);
				break;*/

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


void leer_informacion(config_struct* config_servidor, tabla_paginacion* una_tabla, t_list* lista){


	uint32_t offset = 0;
	uint32_t indice_marco=0;



	marco* marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);
	printf("MARCO %d\n",marcos->id_marco);
	fflush(stdout);
	printf("OFFSET %d\n",offset);
	fflush(stdout);
	uint32_t pid = (uint32_t) leer_atributo(offset,marcos->id_marco, config_servidor);
	offset+= sizeof(uint32_t);
	printf("ID magico %d\n",pid);
	fflush(stdout);
	printf("MARCO %d\n",marcos->id_marco);
	fflush(stdout);
	printf("OFFSET %d\n",offset);
	fflush(stdout);
	indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
	marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
	uint32_t puntero_tarea = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
	offset+=sizeof(uint32_t);
	printf("Puntero magico %d\n",puntero_tarea);
	fflush(stdout);


	for(int i=0; i < una_tabla->cant_tripulantes;i++){

		printf("MARCO %d\n",marcos->id_marco);
		fflush(stdout);
		printf("OFFSET %d\n",offset);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*) list_get(una_tabla->lista_marcos,indice_marco);
		uint32_t tid = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);
		printf("TID magico %d\n",tid);
		fflush(stdout);

		printf("MARCO %d\n",marcos->id_marco);
		fflush(stdout);
		printf("OFFSET %d\n",offset);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos =  (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		char estado =*(char*)leer_atributo_char(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(char);
        printf("Estado magico %c\n",estado);
		fflush(stdout);


		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		uint32_t pos_x = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);

		printf("Pos x magico %d\n",pos_x);
		fflush(stdout);


		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*) list_get(una_tabla->lista_marcos,indice_marco);
		uint32_t pos_y = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);

		printf("Pos y magico %d\n",pos_y);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*) list_get(una_tabla->lista_marcos,indice_marco);
		uint32_t puntero_pcb = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);

		printf("Puntero magico  %d\n",puntero_pcb);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*) list_get(una_tabla->lista_marcos,indice_marco);
		uint32_t prox_in = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);


		printf("Prox instruccion %d\n",prox_in);
		fflush(stdout);

	}

	for(int j=0; j <(una_tabla->long_tareas);j++){

		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		char valor =*(char*)leer_atributo_char(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(char);
		printf("%c",valor);
		fflush(stdout);

	}
}



void almacenar_informacion(config_struct* config_servidor, tabla_paginacion* una_tabla, t_list* lista ){

	uint32_t offset = 0;
	uint32_t indice_marco=0;
	uint32_t pid = (uint32_t)atoi(list_get(lista,0));



	//tabla_paginacion* una_tabla = list_get(una_tabla,posicion_patota(pid, una_tabla));
	marco* marcos =(marco*)list_get(una_tabla->lista_marcos,indice_marco);
	actualizar_lru(marcos);
	offset +=escribir_atributo(pid,offset,marcos->id_marco, config_servidor); //Completar con puntero a tareas

	uint32_t puntero_tarea = 0;
	uint32_t offset_pcb = offset;
	indice_marco += alcanza_espacio(&offset, config_servidor->tamanio_pag, sizeof(uint32_t));
	marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);
	actualizar_lru(marcos);
	uint32_t marco_pcb = indice_marco;
	offset +=escribir_atributo(puntero_tarea,offset,marcos->id_marco, config_servidor);

	uint32_t cantidad_tripulantes = (uint32_t)atoi((list_get(lista,1)));

	char* tarea=(char*)list_get(lista, cantidad_tripulantes+3);

	for(int i =2; i<cantidad_tripulantes+2;i++){
		tcbTripulante* tripulante= (tcbTripulante*) list_get(lista,i);



		uint32_t tid = tripulante->tid;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos =(marco*) list_get(una_tabla->lista_marcos,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(tid,offset,marcos->id_marco, config_servidor);


		char estado = tripulante->estado;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo_char(tripulante,offset,marcos->id_marco, config_servidor);


		uint32_t pos_x = tripulante->posicionX;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(pos_x,offset,marcos->id_marco, config_servidor);

		uint32_t pos_y =tripulante->posicionY;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(pos_y,offset,marcos->id_marco, config_servidor);


		uint32_t prox_i = tripulante->prox_instruccion;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(prox_i,offset,marcos->id_marco, config_servidor);

		uint32_t p_pcb =tripulante->puntero_pcb;
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		actualizar_lru(marcos);
	    offset +=escribir_atributo(p_pcb,offset,marcos->id_marco, config_servidor);



	}

	//Terminar de escribir patota

	puntero_tarea=((indice_marco)*(config_servidor->tamanio_pag))+offset;

	log_info(logger,"EL puntero a tareas escrito es %d y el offset es %d", puntero_tarea, offset);
	escribir_atributo(puntero_tarea,offset_pcb,marco_pcb, config_servidor);

	for(int i=0;i<strlen(tarea)+1;i++){
		printf("%c", tarea[i]);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->lista_marcos,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_char_tarea(tarea[i],offset,marcos->id_marco, config_servidor);


}

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




int escribir_atributo(uint32_t dato, int offset, int nro_marco, config_struct* config_s){
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,&dato,sizeof(int));
	return sizeof(int);
}

void* leer_atributo(int offset, int nro_marco, config_struct* config_s){
	void* dato =malloc(sizeof(int));
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(&dato,p+desplazamiento,sizeof(int));
	return dato;
}



int escribir_atributo_char(tcbTripulante* tripulante, int offset, int nro_marco, config_struct* config_s){
	char* estado= malloc(sizeof(char));
	sprintf(estado,"%c",tripulante->estado);
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,estado,sizeof(char));
	return sizeof(char);
}

int escribir_char_tarea(char caracter, int offset, int nro_marco, config_struct* config_s){
	char* valor= malloc(sizeof(char));
	sprintf(valor,"%c",caracter);
	void* p = config_s->posicion_inicial;
	int desplazamiento=nro_marco*(config_s->tamanio_pag)+offset;
	memcpy(p+desplazamiento,valor,sizeof(char));
	return sizeof(char);
}


void* leer_atributo_char(int offset, int nro_marco, config_struct* config_s){
	void* dato =malloc(sizeof(char));
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
	sem_wait(&MUTEX_MEM_PPAL);
	int contador_marcos = 0;
	for(int i=0;i<config_servidor->cant_marcos;i++){
		if((int)(list_get(config_servidor->marcos_libres, i))==0){
			contador_marcos++;
		}

	}
	sem_post(&MUTEX_MEM_PPAL);
	return contador_marcos;
}

void imprimir_ocupacion_marcos(config_struct* configuracion){
	sem_wait(&MUTEX_MEM_PPAL);
	printf("Marcos ocupados:\n");
	for(int i=0; i<(configuracion->cant_marcos);i++){
			printf("|%d|",(int) list_get(configuracion->marcos_libres,i));
		}
		printf("\n");
		sem_post(&MUTEX_MEM_PPAL);
}

int posicion_patota(int id_buscado,t_list* tabla_aux){
	sem_wait(&MUTEX_MEM_PPAL);
	for(int i=0; i<list_size(tabla_aux);i++){
		tabla_paginacion* auxiliar= (tabla_paginacion*)list_get(tabla_aux, i);
		if(auxiliar->id_patota==id_buscado){
			sem_post(&MUTEX_MEM_PPAL);
			return i;
		}
	}
	sem_post(&MUTEX_MEM_PPAL);
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

int cuantos_marcos(int cuantos_tripulantes, int longitud_tarea,config_struct* config_servidor){

	int tamanio_marco =  config_servidor->tamanio_pag;
	int nuevo_marco  = tamanio_marco ;

	int cantidad_marcos = 1;

	cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(uint32_t), cantidad_marcos);

	cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(uint32_t), cantidad_marcos);

	for(int i=0; i < cuantos_tripulantes; i++){

		cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(uint32_t), cantidad_marcos);

		cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(char), cantidad_marcos);

		cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(uint32_t), cantidad_marcos);

		cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(uint32_t), cantidad_marcos);

		cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(uint32_t), cantidad_marcos);

		cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(uint32_t), cantidad_marcos);

	}

	for(int i=0; i < longitud_tarea; i++){

		cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, sizeof(char), cantidad_marcos);

	}


    return cantidad_marcos;
}

void dump_memoria_paginacion(){
	char buff1[50];
	char buff2[50];
	char namebuff[100];
	time_t now = time(0);
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
				strftime(buff1, 100, "%d/%m/%Y %H:%M:%S"  , localtime(&un_marco->ultimo_uso));
				//printf("Ultimo uso %s\n", buff1);
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
		printf("Patota %d\n",una_tabla->id_patota);
		for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
			marco* un_marco=list_get(una_tabla->lista_marcos,j);
			printf("ID MARCO %d\n",un_marco->id_marco );
			printf("Uso %d\n",un_marco->bit_uso );
			//printf("Libre %d\n",un_marco->libre );
			printf("Ubicacion %d\n",un_marco->ubicacion );
			//printf("Time %f\n",un_marco->ultimo_uso);
			}

	}
	sem_post(&MUTEX_LISTA_TABLAS_PAGINAS);
}


void swap_pagina(void* contenidoAEscribir,int numDeBloque){
	FILE* swapfile;
	swapfile = fopen("swap.bin","w+");
	fseek(swapfile,numDeBloque*configuracion.tamanio_pag*sizeof(char),SEEK_SET);
	//char* aux = malloc(configuracion.tamanio_pag*sizeof(char));
	//aux=completarBloque(contenidoAEscribir);
	fwrite(contenidoAEscribir,sizeof(char)*configuracion.tamanio_pag,1,swapfile);
	//free(aux);
	fclose(swapfile);
}

int swap_a_memoria(int numBloque){

	int nuevo_marco=posicion_marco();
	memcpy(configuracion.posicion_inicial+(nuevo_marco*configuracion.tamanio_pag),(recuperar_pag_swap(numBloque)),configuracion.tamanio_pag*sizeof(char));

	return nuevo_marco;

}


void* recuperar_pag_swap(int numDeBloque){
	FILE* swapfile = fopen("swap.bin","r+");
	void* leido = malloc(configuracion.tamanio_pag*sizeof(char));
	fseek(swapfile,(numDeBloque*configuracion.tamanio_pag)*sizeof(char),SEEK_SET);
	fread(leido,configuracion.tamanio_pag*sizeof(char),1,swapfile);
	//char* aux = vaciar_bloque(leido);
	fclose(swapfile);
	//free(leido);
	return leido;
}



int reemplazo_lru(){
	printf("Aqui llega?1");
	   int nro_marco;
	   time_t lru_actual = time(0);
	   marco* lru_m=malloc(sizeof(marco));
	   sem_wait(&MUTEX_MEM_PPAL);
	   for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
		   printf("Aqui llega?2");
		   tabla_paginacion* una_tabla =(tabla_paginacion*)list_get(tabla_paginacion_ppal,i);
		   	   for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
		   		   marco* un_marco=(marco*)list_get(una_tabla->lista_marcos,j);
		   		   if(un_marco->ultimo_uso < lru_actual && un_marco->ubicacion==MEM_PRINCIPAL){
		   		               lru_actual = un_marco->ultimo_uso;
		   		               lru_m = un_marco;
		   		           }
		   	   }
	   }

	   sem_post(&MUTEX_MEM_PPAL);
	   printf("Aqui llega?");
	   lru_m->ubicacion=MEM_SECUNDARIA;
	   nro_marco=lru_m->id_marco;

	   void* contenidoAEscribir = malloc(configuracion.tamanio_pag);

	   memcpy(contenidoAEscribir,configuracion.posicion_inicial + nro_marco*(configuracion.tamanio_pag) ,configuracion.tamanio_pag);
	   lru_m->id_marco=lugar_swap_libre();
	   swap_pagina(contenidoAEscribir,lru_m->id_marco);

	   return 1;
}


void actualizar_lru(marco* un_marco){
	un_marco->ultimo_uso= time(0);


}

int lugar_swap_libre(){
	sem_wait(&MUTEX_MEM_SEC);
	for(int i=0;i<configuracion.cant_lugares_swap;i++){
		if(list_get(configuracion.swap_libre,i)==0){
			int valor = 1;
			list_add_in_index(configuracion.swap_libre,i,(void*)valor);
			sem_post(&MUTEX_MEM_SEC);
			return i;
		}
	}
	sem_post(&MUTEX_MEM_SEC);
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

		   printf("MARCO ENTREGADO POR CLOCK %d \n",  nro_marco);
		   fflush(stdout);
		   return nro_marco;

}


/*
int reemplazo_clock(){
			printf("Estoy en el clock\n");
			int nro_marco;
			int flag =0;
			marco* clock_m=malloc(sizeof(marco));
			sem_wait(&MUTEX_LISTA_TABLAS_PAGINAS);
			for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
				tabla_paginacion* una_tabla = (tabla_paginacion*)list_get(tabla_paginacion_ppal,i);
				while(flag !=1){
					for(int j=0; j<list_size(una_tabla->lista_marcos);j++){
						printf("Henos qui");
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

		   printf("Aun no rompi");
		   clock_m->ubicacion=MEM_SECUNDARIA;
		   nro_marco=clock_m->id_marco;

		   void* contenidoAEscribir = malloc(configuracion.tamanio_pag);
		   memcpy(contenidoAEscribir,configuracion.posicion_inicial + nro_marco*(configuracion.tamanio_pag) ,configuracion.tamanio_pag);
		   printf("SI llegue aqui");
		   clock_m->id_marco=lugar_swap_libre();
		   printf("Aqui no");
		   swap_pagina(contenidoAEscribir,clock_m->id_marco);

		   printf("MARCO ENTREGADO POR CLOCK %d \n",  nro_marco);
		   fflush(stdout);
		   return nro_marco;

}
*/

void actualizar_tripulante(tcbTripulante* tripulante, int id_patota){


	int offset=0;
	int nro_marco=0;
	int pos;
	pos=posicion_patota(id_patota, tabla_paginacion_ppal);
	tabla_paginacion* auxiliar= (tabla_paginacion*)list_get(tabla_paginacion_ppal, pos);
	//PID
	nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(int));
	offset+=sizeof(int);
	//PunteroTareas

	nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(int));
	offset+=sizeof(int);
	for (int i=0; i<3;i++){
		//TID
		nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(int));
		offset+=sizeof(int);

		//Estado
		nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(char));
		offset+=sizeof(char);

		//Pos x
		nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(int));
		offset+=sizeof(int);

		//POs y
		nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(int));
		offset+=sizeof(int);

		//Prox inst
		nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(int));
		offset+=sizeof(int);

		//Puntero PCB
		nro_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(int));
		offset+=sizeof(int);

	}



			uint32_t tid = tripulante->tid;

			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marco* marcos;
			marcos =(marco*) list_get(auxiliar->lista_marcos,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(tid,offset,marcos->id_marco, &configuracion);


			char estado = tripulante->estado;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(char));
			marcos =(marco*) list_get(auxiliar->lista_marcos,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo_char(tripulante,offset,marcos->id_marco, &configuracion);


			uint32_t pos_x = tripulante->posicionX;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->lista_marcos,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(pos_x,offset,marcos->id_marco, &configuracion);

			uint32_t pos_y =tripulante->posicionY;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->lista_marcos,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(pos_y,offset,marcos->id_marco, &configuracion);


			uint32_t prox_i = tripulante->prox_instruccion;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->lista_marcos,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(prox_i,offset,marcos->id_marco, &configuracion);

			uint32_t p_pcb =tripulante->puntero_pcb;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->lista_marcos,nro_marco);
			actualizar_lru(marcos);
		    offset +=escribir_atributo(p_pcb,offset,marcos->id_marco, &configuracion);



}

/* Pseudocodigo direccion logica en todos los tripulantes
 *
 * obtener_tarea_2(pid, dire_logica)
 * buscar_tabla_paginacion_patota
 *
 */

char* obtener_tarea(int id_patota, int nro_tarea){
	char* una_tarea;
	int contador_tarea=0;
	char* tarea=malloc(sizeof(char));
	for(int i=0; i<list_size(tabla_paginacion_ppal);i++){
		tabla_paginacion* una_tabla = list_get(tabla_paginacion_ppal,i);
		if(una_tabla->id_patota==id_patota){
			char* tarea_nueva=malloc(una_tabla->long_tareas + 1);
			marco* un_marco=(marco*)list_get(una_tabla->lista_marcos,0);
			uint32_t puntero_tareas = (uint32_t)leer_atributo(sizeof(uint32_t),un_marco->id_marco, &configuracion);
			printf("El punterin %d\n", puntero_tareas);
			fflush(stdout);
			float indice_marco = (float)puntero_tareas/configuracion.tamanio_pag;
			printf("El indice %f\n", indice_marco);
			int parte_entera = puntero_tareas/configuracion.tamanio_pag;
			float parte_decimal = (indice_marco - parte_entera);
			indice_marco=(int)parte_entera;
			printf("El indice %f\n", parte_decimal);
			fflush(stdout);
			uint32_t offset = configuracion.tamanio_pag*(parte_decimal);
			printf("El offset %d\n", offset);
			fflush(stdout);

			//Traigo el marco al que apunta el puntero de las tareas a memoria

			for(int letra=0; letra<una_tabla->long_tareas;letra++){

		    marco* otro_marco=(marco*)list_get(una_tabla->lista_marcos,indice_marco);

		    if(otro_marco->ubicacion == MEM_PRINCIPAL){
		    	tarea=(char*)leer_atributo_char(offset,otro_marco->id_marco, &configuracion);
		    	offset+=sizeof(char);
		    	tarea_nueva=strcat(tarea_nueva,tarea);

		    }else{
		    	int nuevo_marco = swap_a_memoria(otro_marco->id_marco);
		    	otro_marco->ubicacion=MEM_PRINCIPAL;
		    	otro_marco->id_marco=nuevo_marco;
		    	tarea=(char*)leer_atributo_char(offset,otro_marco->id_marco, &configuracion);
		    	tarea_nueva=strcat(tarea_nueva,tarea);
		    	offset+=sizeof(char);

		    }
		    if(tarea==";"){
		    	contador_tarea++;
		    }
		    indice_marco+=alcanza_espacio(&offset,configuracion.tamanio_pag, sizeof(char));
    	}

			//tarea_nueva=strcat(tarea_nueva,'\0');
			char** tareas = string_split(tarea_nueva, "-");
			una_tarea = tareas[nro_tarea];
			//printf("La tarea nro %d es %s", nro_tarea, una_tarea);

	}


}
	return una_tarea;
}
