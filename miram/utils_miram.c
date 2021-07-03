#include "utils_miram.h"
config_struct configuracion;

t_list* memoria_aux;
int variable_servidor = -1;
int socket_servidor;
NIVEL* nivel;
int err;
int numero_mapa = 0;

void iniciar_servidor(config_struct* config_servidor)
{



	memoria_aux=list_create();
	//Reinicio el anterior y arranco uno nuevo
	FILE* archivo = fopen("miram-rq.log","w");
	fclose(archivo);
	logger = log_create("MiRam.log","Miram-RQ",1,LOG_LEVEL_INFO);
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

	printf("Llegue");


	int hilo;
	while(variable_servidor != 0){

		socket_cliente = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);

		if(socket_cliente>0){
			hilo ++ ;
			log_info(logger, "Estableciendo conexión desde %d\n", dir_cliente.sin_port);
			log_info(logger, "Creando hilo");
			fflush(stdout);
			pthread_t hilo_cliente=(char)hilo;
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente ,(void*)socket_cliente);
			pthread_detach(hilo_cliente);
		}
	}

	printf("Me fui\n");
	list_clean(memoria_aux);
	list_destroy(memoria_aux);


}


int funcion_cliente(int socket_cliente){
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
			case PRUEBA:;
				lista=recibir_paquete(socket_cliente);
				int pid = (int)atoi(list_get(lista,0));
				printf("el numero de la patota es %d\n", pid);
				fflush(stdout);
				patota = crear_patota(pid,0);

				int cantidad_tripulantes = (int)atoi((list_get(lista,1)));
				printf("cant tripu %d\n", cantidad_tripulantes);
				fflush(stdout);



				for(int i=2; i < cantidad_tripulantes +2; i++){
					tripulante=(tcbTripulante*)list_get(lista,i);
					mostrar_tripulante(tripulante,patota);
					printf("\n");
					fflush(stdin);
				}

				char* tarea=(char*)list_get(lista, cantidad_tripulantes+2);
				printf("Las tareas serializadas son: %s \n", tarea);
				fflush(stdout);
				int cuantos_marcos_necesito = cuantos_marcos(cantidad_tripulantes,strlen(tarea)+1,&configuracion);
				fflush(stdout);
				printf("\n Necesito estos marcos pablin %d", cuantos_marcos_necesito);
				fflush(stdout);
				printf("\n Marcos sin un carajo: %d \n", cuantos_marcos_libres(&configuracion));
				fflush(stdout);
				if(cuantos_marcos_necesito>cuantos_marcos_libres(&configuracion)){
					printf(" \n No vas a poder poner un choto\n");
					fflush(stdout);
					//ack a discordiador
				}else{
					int posicion_libre = -1;
					printf("Guardando info.....");
					fflush(stdout);
					tabla_paginacion* una_tabla=malloc(sizeof(tabla_paginacion));
					una_tabla->id_patota = pid;
					una_tabla->cant_tripulantes= cantidad_tripulantes;
					una_tabla->long_tareas = strlen(tarea)+1;
					una_tabla->marco_inicial=list_create();
					list_add(memoria_aux, una_tabla);

					for(int i=0 ; i<cuantos_marcos_necesito; i++){
						posicion_libre = posicion_marco(&configuracion);
						marco* marco_nuevo = malloc (sizeof(marco));
						marco_nuevo->id_marco=posicion_libre;
						marco_nuevo->ubicacion=MEM_PRINCIPAL;
						marco_nuevo->bit_uso=1;
						list_add(una_tabla->marco_inicial,marco_nuevo);
					}


					int cols, rows;


					tcbTripulante* tripulante3 =crear_tripulante(3,'N',23,23,23,23);
					almacenar_informacion(&configuracion, una_tabla, lista);
					printf("\n");
					leer_informacion(&configuracion,  una_tabla, lista);

					actualizar_tripulante( tripulante3, 1);

					leer_informacion(&configuracion,  una_tabla, lista);
					//mem_hexdump(configuracion.posicion_inicial, configuracion.tamanio_pag*sizeof(char));
					fflush(stdout);
					printf("\n");

					cols=25;
					rows=25;

					nivel_gui_inicializar();

					nivel_gui_get_area_nivel(&cols, &rows);

					nivel = nivel_crear("AMong-OS");

					char letra = 'A';

					err = personaje_crear(nivel, letra, 10, 12);
					ASSERT_CREATE(nivel, letra, err);

					letra = 'B';

					err = personaje_crear(nivel, letra, 3, 4);
					ASSERT_CREATE(nivel, letra, err);

					letra = 'C';

					err = personaje_crear(nivel, letra, 20, 10);
					ASSERT_CREATE(nivel, letra, err);



					item_desplazar(nivel, 'A', 0, -1);
					sleep(1);
					item_desplazar(nivel, 'A', -2, 0);
					sleep(1);
					item_desplazar(nivel, 'A', 0, -2);
					sleep(1);
					item_desplazar(nivel, 'A', 0, +3);
					sleep(1);
					/*fflush(stdout);
					dump_memoria();


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
				nivel_destruir(nivel);
				nivel_gui_terminar();
				variable_servidor = 0;
				numero_mapa=1;
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


void iniciar_miram(config_struct* config_servidor){



	config_servidor->posicion_inicial= malloc((config_servidor->tamanio_memoria));

	if(strcmp(config_servidor->squema_memoria,"PAGINACION")==0){

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
		//Inicializo Segmentacion
	}
}

void finalizar_miram(config_struct* config_servidor){
	free(config_servidor->posicion_inicial);
	for(int i=0; i<config_servidor->cant_marcos;i++){
				free(config_servidor->marcos[i]);
			}
}




void imprimir_seg(t_list* tabla_aux){
	for(int i=0; i<list_size(tabla_aux);i++){
		tabla_segmentacion* auxiliar = list_get(tabla_aux,i);
		printf("Tabla segmentacion correspondiente a patota %d\n", auxiliar->id_patota);
		for(int j=0;j<list_size(auxiliar->segmento_inicial);j++){
			segmento* segmento_leido=list_get(auxiliar->segmento_inicial,j);
			printf("Base %d\n", segmento_leido->base);
			printf("Tamanio %d\n", segmento_leido->tamanio);
		}
	}
}



void leer_informacion(config_struct* config_servidor, tabla_paginacion* una_tabla, t_list* lista){


	uint32_t offset = 0;
	uint32_t indice_marco=0;



	marco* marcos =(marco*) list_get(una_tabla->marco_inicial,indice_marco);
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
	marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
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
		marcos = (marco*) list_get(una_tabla->marco_inicial,indice_marco);
		uint32_t tid = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);
		printf("TID magico %d\n",tid);
		fflush(stdout);

		printf("MARCO %d\n",marcos->id_marco);
		fflush(stdout);
		printf("OFFSET %d\n",offset);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos =  (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		char estado =*(char*)leer_atributo_char(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(char);
        printf("Estado magico %c\n",estado);
		fflush(stdout);


		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		uint32_t pos_x = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);

		printf("Pos x magico %d\n",pos_x);
		fflush(stdout);


		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*) list_get(una_tabla->marco_inicial,indice_marco);
		uint32_t pos_y = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);

		printf("Pos y magico %d\n",pos_y);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*) list_get(una_tabla->marco_inicial,indice_marco);
		uint32_t puntero_pcb = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);

		printf("Puntero magico  %d\n",puntero_pcb);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*) list_get(una_tabla->marco_inicial,indice_marco);
		uint32_t prox_in = (uint32_t)leer_atributo(offset,marcos->id_marco, config_servidor);
		offset+=sizeof(uint32_t);


		printf("Prox instruccion %d\n",prox_in);
		fflush(stdout);

	}

	printf("Marco %d\n", indice_marco);
	fflush(stdout);
	printf("Marco id %d\n", marcos->id_marco);
	fflush(stdout);
	printf("Offset %d\n", marcos->id_marco);
	fflush(stdout);



	printf("Debe iterar %d\n", una_tabla->long_tareas);
	fflush(stdout);
	for(int j=0; j <(una_tabla->long_tareas);j++){

		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
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
	marco* marcos =(marco*)list_get(una_tabla->marco_inicial,indice_marco);
	actualizar_lru(marcos);
	offset +=escribir_atributo(pid,offset,marcos->id_marco, config_servidor); //Completar con puntero a tareas

	uint32_t puntero_tarea = 0;
	uint32_t offset_pcb = offset;
	indice_marco += alcanza_espacio(&offset, config_servidor->tamanio_pag, sizeof(uint32_t));
	marcos =(marco*) list_get(una_tabla->marco_inicial,indice_marco);
	actualizar_lru(marcos);
	uint32_t marco_pcb = indice_marco;
	offset +=escribir_atributo(puntero_tarea,offset,marcos->id_marco, config_servidor);

	uint32_t cantidad_tripulantes = (uint32_t)atoi((list_get(lista,1)));

	char* tarea=(char*)list_get(lista, cantidad_tripulantes+2);

	for(int i =2; i<cantidad_tripulantes+2;i++){
		tcbTripulante* tripulante= (tcbTripulante*) list_get(lista,i);



		uint32_t tid = tripulante->tid;
		printf("\n TID %d",tid);

		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos =(marco*) list_get(una_tabla->marco_inicial,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(tid,offset,marcos->id_marco, config_servidor);


		char estado = tripulante->estado;
		printf("\n Estado %c", estado);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo_char(tripulante,offset,marcos->id_marco, config_servidor);


		uint32_t pos_x = tripulante->posicionX;
		printf("\n Pos x %d",pos_x);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(pos_x,offset,marcos->id_marco, config_servidor);

		uint32_t pos_y =tripulante->posicionY;
		printf("\n Pos y %d",pos_y);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(pos_y,offset,marcos->id_marco, config_servidor);


		uint32_t prox_i = tripulante->prox_instruccion;
		printf("\nProx instruccion %d", prox_i);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_atributo(prox_i,offset,marcos->id_marco, config_servidor);

		uint32_t p_pcb =tripulante->puntero_pcb;
		printf("\n Este es el puntero al pcb  %d\n",p_pcb);
		fflush(stdout);
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(uint32_t));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		actualizar_lru(marcos);
	    offset +=escribir_atributo(p_pcb,offset,marcos->id_marco, config_servidor);



	}

	//indice_marco+=alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));

	//puntero_tarea=0;
	//Terminar de escribir patota


	//escribir_atributo(&puntero_tarea,offset_pcb,marco_pcb, config_servidor, sizeof(int));

	for(int i=0;i<strlen(tarea)+1;i++){
		indice_marco += alcanza_espacio(&offset, (config_servidor->tamanio_pag), sizeof(char));
		marcos = (marco*)list_get(una_tabla->marco_inicial,indice_marco);
		actualizar_lru(marcos);
		offset +=escribir_char_tarea(tarea[i],offset,marcos->id_marco, config_servidor);
		printf("%c",tarea[i]);
		fflush(stdout);


}

}
/*
void buscar_tripulante(int id_patota, int nro_tripulante, tabla_paginacion tabla_pag, config_struct* config){
  //Leer tripulante de memoria
  tabla_paginacion* aux= list_get(tabla_pag, posicion_patota(id_patota, tabla_pag));
  int offset= 2*sizeof(int);
  int indice_calculado = offset/(atoi(config->tamanio_pag));
  marco* marco_leido = list_get(aux->marco_inicial,indice_calculado);

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



int posicion_marco(config_struct* config_servidor){
	for(int i=0;i<config_servidor->cant_marcos;i++){
		if((int)(list_get(config_servidor->marcos_libres, i))==0){
			int valor = 1;
			list_add_in_index(config_servidor->marcos_libres, i, (void*)valor);
			return i;
		}

	}
	return -1;
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
	printf("Marcos ocupados:\n");
	for(int i=0; i<(configuracion->cant_marcos);i++){
			printf("|%d|",(int) list_get(configuracion->marcos_libres,i));
		}
		printf("\n");
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


pcbPatota* crear_pcb(uint32_t numero_patota){
	pcbPatota* pcb = malloc(sizeof(pcbPatota));
	pcb->pid = numero_patota;
	//pcb->tareas = NULL;

	return pcb;
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

void dump_memoria(){
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
	for(int i=0; i<list_size(memoria_aux);i++){
		tabla_paginacion* una_tabla = list_get(memoria_aux,i);
		for(int j=0; j<list_size(una_tabla->marco_inicial);j++){
			marco* un_marco=list_get(una_tabla->marco_inicial,j);
			if(un_marco->id_marco==id_marco && un_marco->ubicacion==MEM_PRINCIPAL){
				char buff1[100];
				strftime(buff1, 100, "%d/%m/%Y %H:%M:%S"  , localtime(&un_marco->ultimo_uso));
				printf("Ultimo uso %s\n", buff1);
				 *estado = 1;
				 *proceso=una_tabla->id_patota;
				 *pagina=j;
				 break;
			}
				*estado=0;
				*proceso=0;
				*pagina=0;

			}

	}
}

char* obtener_tarea(int id_patota, int nro_tarea){
	for(int i=0; i<list_size(memoria_aux);i++){
		tabla_paginacion* una_tabla = list_get(memoria_aux,i);
		if(una_tabla->id_patota==id_patota){
			una_tabla->marco_inicial;
		}
	}
	char* tarea;
	return tarea;

}


void swap_pagina(void* contenidoAEscribir,int numDeBloque){
	FILE* swapfile = fopen("swap.bin","rb+");
	fseek(swapfile,numDeBloque*configuracion.tamanio_pag*sizeof(char),SEEK_SET);
	//char* aux = malloc(configuracion.tamanio_pag*sizeof(char));
	//aux=completarBloque(contenidoAEscribir);
	fwrite(contenidoAEscribir,sizeof(char)*configuracion.tamanio_pag,1,swapfile);
	//free(aux);
	fclose(swapfile);
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

char* vaciar_bloque(char* bloqueAVaciar){
	char* bloque;// = string_new();
	int i;
	for (i = 0; i < string_length(bloqueAVaciar); ++i) {
		if (bloqueAVaciar[i] == '#')
			break;
	}
	bloque = string_substring_until(bloqueAVaciar,i);
	return bloque;
}

void borrarBloque(int numeroDeBloque, int tamanioDeBloque){
	swap_pagina("",numeroDeBloque);
}

char* completarBloque(char* bloqueACompletar){
	int cantACompletar = configuracion.tamanio_pag - string_length(bloqueACompletar);
	printf("Cant a completar %d\n", cantACompletar);
	fflush(stdout);
	char* aux = malloc(sizeof(configuracion.tamanio_pag));
	aux = string_duplicate(bloqueACompletar);
	string_append(&aux,string_repeat('#',cantACompletar));
	fflush(stdout);
	printf("%s",aux);
	fflush(stdout);

	return aux;
}


int reemplazo_lru(){
	   int nro_marco;
	   time_t lru_actual = time(0);
		marco* lru_m=malloc(sizeof(marco));
	   for(int i=0; i<list_size(memoria_aux);i++){
		   tabla_paginacion* una_tabla = list_get(memoria_aux,i);
		   	   for(int j=0; j<list_size(una_tabla->marco_inicial);j++){
		   		   marco* un_marco=(marco*)list_get(una_tabla->marco_inicial,j);
		   		   if(un_marco->ultimo_uso < lru_actual && un_marco->ubicacion==MEM_PRINCIPAL){
		   		               lru_actual = un_marco->ultimo_uso;
		   		               lru_m = un_marco;
		   		           }
		   	   }
	   }

	   lru_m->ubicacion=MEM_SECUNDARIA;
	   nro_marco=lru_m->id_marco;

	   void* contenidoAEscribir = malloc(configuracion.tamanio_pag*sizeof(char));
	   memcpy(contenidoAEscribir,configuracion.posicion_inicial + nro_marco*(configuracion.tamanio_pag) ,configuracion.tamanio_pag);
	   lru_m->id_marco=lugar_swap_libre();
	   swap_pagina(contenidoAEscribir,lru_m->id_marco);

	   return nro_marco;
}


void actualizar_lru(marco* un_marco){
	un_marco->ultimo_uso= time(0);


}

int lugar_swap_libre(){
	for(int i=0;i<configuracion.cant_lugares_swap;i++){
		if(list_get(configuracion.swap_libre,i)==0){
			int valor = 1;
			list_add_in_index(configuracion.swap_libre,i,(void*)valor);
			return i;
		}
	}
	return -1;
}

int espacios_swap_libres(config_struct* config_servidor){
	int contador_swap = 0;
	for(int i=0;i<config_servidor->cant_lugares_swap;i++){
		if((int)(list_get(config_servidor->swap_libre, i))==0){
			contador_swap++;
		}

	}
	return contador_swap;
}

int reemplazo_clock(){
			int nro_marco;
			marco* clock_m=malloc(sizeof(marco));
				for(int i=0; i<list_size(memoria_aux);i++){
					tabla_paginacion* una_tabla = list_get(memoria_aux,i);
					while(1){
						for(int j=0; j<list_size(una_tabla->marco_inicial);j++){
							marco* un_marco=list_get(una_tabla->marco_inicial,j);
							if(un_marco->bit_uso==1 && un_marco->ubicacion==MEM_PRINCIPAL){
			   		            un_marco->bit_uso=0;
			   		           }
							if(un_marco->bit_uso==0 && un_marco->ubicacion==MEM_PRINCIPAL){
								clock_m = un_marco;
								break;
							}
			   	   }
		   }
		}

		   clock_m->ubicacion=MEM_SECUNDARIA;
		   nro_marco=clock_m->id_marco;

		   void* contenidoAEscribir = malloc(configuracion.tamanio_pag*sizeof(char));
		   memcpy(contenidoAEscribir,configuracion.posicion_inicial + nro_marco*(configuracion.tamanio_pag) ,configuracion.tamanio_pag);
		   clock_m->id_marco=lugar_swap_libre();
		   swap_pagina(contenidoAEscribir,clock_m->id_marco);

		   return nro_marco;

}


void actualizar_tripulante(tcbTripulante* tripulante, int id_patota){


	int offset=0;
	int nro_marco=0;
	int pos;
	pos=posicion_patota(id_patota, memoria_aux);
	tabla_paginacion* auxiliar= (tabla_paginacion*)list_get(memoria_aux, pos);
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
			marcos =(marco*) list_get(auxiliar->marco_inicial,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(tid,offset,marcos->id_marco, &configuracion);


			char estado = tripulante->estado;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(char));
			marcos =(marco*) list_get(auxiliar->marco_inicial,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo_char(tripulante,offset,marcos->id_marco, &configuracion);


			uint32_t pos_x = tripulante->posicionX;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->marco_inicial,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(pos_x,offset,marcos->id_marco, &configuracion);

			uint32_t pos_y =tripulante->posicionY;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->marco_inicial,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(pos_y,offset,marcos->id_marco, &configuracion);


			uint32_t prox_i = tripulante->prox_instruccion;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->marco_inicial,nro_marco);
			actualizar_lru(marcos);
			offset +=escribir_atributo(prox_i,offset,marcos->id_marco, &configuracion);

			uint32_t p_pcb =tripulante->puntero_pcb;
			fflush(stdout);
			nro_marco += alcanza_espacio(&offset, configuracion.tamanio_pag, sizeof(uint32_t));
			marcos =(marco*) list_get(auxiliar->marco_inicial,nro_marco);
			actualizar_lru(marcos);
		    offset +=escribir_atributo(p_pcb,offset,marcos->id_marco, &configuracion);



}

void* crear_mapa(){
	while (numero_mapa!=1) {
			nivel_gui_dibujar(nivel);



	}
}




