#include "utils_miram.h"


int variable_servidor = -1;
int socket_servidor;

void iniciar_servidor(config_struct* config_servidor)
{
	t_log* logg;
	logg = log_create("MiRam1.log", "MiRam1", 1, LOG_LEVEL_DEBUG);
	log_info(logg, "Servidor iniciando");

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

	printf("Me fui\n");

}


int funcion_cliente(int socket_cliente){
	t_list* lista = list_create();
	pcbPatota* patota;
	tcbTripulante* tripulante;
	t_paquete* paquete;
	int tipoMensajeRecibido = -1;
	printf("Se conecto este socket a mi %d\n",socket_cliente);
	while(1){
		tipoMensajeRecibido = recibir_operacion(socket_cliente);

		switch(tipoMensajeRecibido)
		{
			case INICIAR_PATOTA:
				lista = recibir_paquete(socket_cliente);

				uint32_t pid = (uint32_t)atoi(list_get(lista,0));
				printf("el numero de la patota es %d", pid);
				patota = crear_patota(pid,0);

				uint32_t cantidad_tripulantes = (uint32_t)atoi(list_get(lista,1));
				printf("cant tripu %d\n", cantidad_tripulantes);

				for(int i=2; i < cantidad_tripulantes +2; i++){
					tripulante = (tcbTripulante*)list_get(lista,i);
					mostrar_tripulante(tripulante, patota);
					printf("\n");
				}

				char* tarea = malloc((uint32_t)atoi(list_get(lista, cantidad_tripulantes+2)));
				tarea = (char*)list_get(lista, cantidad_tripulantes+3);
				printf("Las tareas serializadas son: %s \n", tarea);


				if(strcmp(configuracion.squema_memoria,"SEGMENTACION")==0){
					bool todo_ok = patota_segmentacion(pid, cantidad_tripulantes, tarea, lista);
					if (todo_ok == false){
						log_warning(logger, "no se puedo asignar espacio de memoria a todo");
						char* mensaje = malloc(20);
						mensaje = "Memoria NO asignada";
						enviar_mensaje(mensaje, socket_cliente);
					} else {
						log_info(logger, "Se asigno espacio de memoria a todo");
						char* mensaje = malloc(17);
						mensaje = "Memoria asignada";
						enviar_mensaje(mensaje, socket_cliente);
					}
				}
				break;


			case LISTAR_TRIPULANTES:
				if(strcmp(configuracion.squema_memoria,"SEGMENTACION")==0){
					for(int i=0; i < list_size(lista_tablas_segmentos); i++){
						tabla_segmentacion* tabla_segmentos = (tabla_segmentacion*)list_get(lista_tablas_segmentos, i);
						int pid = tabla_segmentos->id_patota;

						for(int j=2; j < list_size(tabla_segmentos->segmento_inicial); j++){
							segmento* segmento = list_get(tabla_segmentos->segmento_inicial, j);

							for(int k=0; k < list_size(tabla_espacios_de_memoria); k++){
								espacio_de_memoria* espacio = (espacio_de_memoria*)list_get(tabla_espacios_de_memoria, k);
								if(espacio->base == segmento->base){
									tcbTripulante* tripulante = espacio->contenido;
									printf("Tripulante: %d     Patota: %d     Status: %c \n", tripulante->tid, pid, tripulante->estado);
									k = list_size(tabla_espacios_de_memoria); //para que corte el for
								}
							}
						}
					}
				}


				/*t_paquete* paquete = crear_paquete(LISTAR_TRIPULANTES);
				tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
				agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));
				enviar_paquete(paquete,socket_cliente);
				eliminar_paquete(paquete);*/
				break;


			case EXPULSAR_TRIPULANTE:
				lista = recibir_paquete(socket_cliente);
				uint32_t tripulante_id = (uint32_t)atoi(list_get(lista,0));

				if(strcmp(configuracion.squema_memoria,"SEGMENTACION")==0){
					bool tripulante_expulsado_con_exito = funcion_expulsar_tripulante(tripulante_id);
					if(tripulante_expulsado_con_exito == false){
						log_warning(logger, "No se pudo eliminar ese tripulante");
					}
				}

				break;

			case PEDIR_TAREA:
				log_info(logger, "PEDIR_TAREA");//PONGO ESTA LINEA PQ SI NO LA PONGO TIRA ERROR. CORREGIR.
				t_list* lista_tripulante = recibir_paquete(socket_cliente);
				tcbTripulante* tripulante = (tcbTripulante*)list_get(lista_tripulante,0);
				log_info(logger, "tripulante %d quiere tarea", tripulante->tid);
				int numero_patota = (int)atoi(list_get(lista_tripulante,1));
				log_info(logger, "tripu patota %d", numero_patota);

				sem_wait(&MUTEX_PEDIR_TAREA);
				for(int i=0; i < list_size(lista_tablas_segmentos); i++){
					tabla_segmentacion* tabla_segmentos = list_get(lista_tablas_segmentos, i);

					if(tabla_segmentos->id_patota == numero_patota){ //suponiendo que el punteropcb es numero de patota
						segmento* segmento = list_get(tabla_segmentos->segmento_inicial, 1); // 1 es el segmento de tareas

						//HACER FUNCION BUSCAR ESPACIO DE MEMORIA
						for(int j=0; j < list_size(tabla_espacios_de_memoria); j++){
							espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, j);

							if(espacio->base == segmento->base){ //encontro la base en memoria de las tareas
								//buscar_tarea();
								log_info(logger, "tripu %d    contenido %s", tripulante->tid ,espacio->contenido);



								break;
							}
						}

					}

				}
				log_info(logger, "tripu %d VA A HACER EL POST", tripulante->tid);
				sem_post(&MUTEX_PEDIR_TAREA);

				//espacio_de_memoria_tareas->contenido = tarea;

				//enviar_header(PEDIR_TAREA, socket_cliente);

				//return 0;
				break;

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


void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + sizeof(int);

	void* a_enviar = malloc(bytes);
	int desplazamiento = 0;

	memcpy(a_enviar + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(a_enviar + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

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

	config_servidor->posicion_inicial= malloc(sizeof(atoi(config_servidor->tamanio_memoria)));
	printf("%d\n",atoi(config_servidor->tamanio_memoria));
	printf("Pos inicial %d\n", (int)config_servidor->posicion_inicial);

	if(strcmp(config_servidor->squema_memoria,"PAGINACION")==0){

		config_servidor->cant_marcos=atoi(config_servidor->tamanio_memoria)/atoi(config_servidor->tamanio_pag);
		config_servidor->marcos= malloc(config_servidor->cant_marcos);

		//Inicializo el array en 0, me indica la ocupacion de los marcos
		for(int i=0; i<config_servidor->cant_marcos;i++){
			config_servidor->marcos[i]=malloc(sizeof(int));
			config_servidor->marcos[i]= 0;
		}

	}else{
		tabla_espacios_de_memoria = list_create();
		espacio_de_memoria* memoria_principal = crear_espacio_de_memoria(0, atoi(config_servidor->tamanio_memoria), true);
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


void agregar_segmentos(t_list* lista_aux_seg){

	tabla_segmentacion* tabla1= malloc(sizeof(tabla_segmentacion));
	tabla_segmentacion* tabla2= malloc(sizeof(tabla_segmentacion));

	segmento* segmento1= malloc(sizeof(segmento));
	segmento* segmento2= malloc(sizeof(segmento));

	tabla1->id_patota =1;
	tabla2->id_patota =2;

	segmento1->base=100;
	segmento1->tamanio=200;

	segmento2->base=300;
	segmento2->tamanio=400;

	tabla1->segmento_inicial=list_create();
	tabla2->segmento_inicial=list_create();

	list_add(tabla1->segmento_inicial, segmento1);
	list_add(tabla2->segmento_inicial, segmento2);

	list_add(lista_aux_seg, tabla1);
	list_add(lista_aux_seg, tabla2);


}



void agregar_memoria_aux(t_list* lista_aux_memoria,config_struct* config_servidor ){

	tabla_paginacion* tabla1=malloc(sizeof(tabla_paginacion));
	tabla_paginacion* tabla2=malloc(sizeof(tabla_paginacion));
	marco* marco1= malloc(sizeof(marco));
	marco* marco2= malloc(sizeof(marco));
	marco* marco3= malloc(sizeof(marco));
	marco* marco4= malloc(sizeof(marco));
	marco1->libre=0;
	marco2->libre=0;
	marco3->libre=1;
	marco4->libre=1;
	marco1->id_marco=posicion_marco(config_servidor);
	marco2->id_marco=posicion_marco(config_servidor);
	marco3->id_marco=posicion_marco(config_servidor);
	marco4->id_marco=posicion_marco(config_servidor);
	tabla1->id_patota =1;
	tabla2->id_patota =2;
	tabla1->ubicacion = MEM_PRINCIPAL;
	tabla1->marco_inicial=list_create();
	tabla2->marco_inicial=list_create();
	list_add(tabla1->marco_inicial, marco1);
	list_add(tabla1->marco_inicial, marco2);
	list_add(tabla1->marco_inicial, marco3);
	list_add(tabla2->marco_inicial, marco4);
	list_add(lista_aux_memoria, tabla1);
	list_add(lista_aux_memoria, tabla2);

}

void agregar_tripulante_marco(tcbTripulante* tripulante, int id_patota, t_list* tabla_aux, config_struct* configuracion){

	tabla_paginacion* una_tabla = (tabla_paginacion*)list_get(tabla_aux, posicion_patota(id_patota, tabla_aux));
	int contador_tablas= 0;
	int marco_disponible = 0;

	while(contador_tablas<list_size(una_tabla->marco_inicial) && marco_disponible != 1){
		marco* marco_leido = list_get(una_tabla->marco_inicial,contador_tablas);
		if(marco_leido->libre == 0){
			printf("hay lugares libres");
			escribir_tripulante(tripulante, (marco_leido->id_marco)*(atoi(configuracion->tamanio_pag)) +  (configuracion->posicion_inicial));
			marco_disponible = 1;
		}
		contador_tablas++;
	}
	if (marco_disponible == 0){
		printf("No hay lugares en los marcos");
	}

}


void imprimir_memoria(t_list* tabla_aux){
	for(int i=0; i<list_size(tabla_aux);i++){
		tabla_paginacion* auxiliar = (tabla_paginacion*)list_get(tabla_aux,i);
		printf("Tabla paginacion correspondiente a patota %d\n", auxiliar->id_patota);
		for(int j=0;j<list_size(auxiliar->marco_inicial);j++){
			marco* marco_leido=list_get(auxiliar->marco_inicial,j);
			printf("Marco en uso %d\n", marco_leido->id_marco);
		}
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

int posicion_marco(config_struct* config_servidor){
	for(int i=0;i<config_servidor->cant_marcos;i++){
		if(config_servidor->marcos[i]==0){
			config_servidor->marcos[i]=(void*)1;
			return i;
		}

	}
	return -1;
}

void imprimir_ocupacion_marcos(config_struct configuracion){
	printf("Marcos ocupados:\n");
	for(int i=0; i<(configuracion.cant_marcos);i++){
			printf("%d",(int)configuracion.marcos[i]);
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

int marco_tarea(int posicion_patota, t_list* tabla_aux, int nro_marco){
	tabla_paginacion* auxiliar= (tabla_paginacion*)list_get(tabla_aux, posicion_patota);
    marco* marco_leido=list_get(auxiliar->marco_inicial,nro_marco-1);
	return marco_leido->id_marco;

}


void escribir_tripulante(tcbTripulante* tripulante, void* posicion_inicial){
	int offset = 0 ;
	memcpy((posicion_inicial)+offset,&tripulante->tid,sizeof(int));
	offset += sizeof(int);
	memcpy((posicion_inicial)+offset,&tripulante->estado,sizeof(char));
	offset += sizeof(char);
	memcpy((posicion_inicial)+offset,&tripulante->posicionX,sizeof(int));
	offset += sizeof(int);
	memcpy((posicion_inicial)+offset,&tripulante->posicionY,sizeof(int));
	offset += sizeof(int);
	memcpy((posicion_inicial)+offset,&tripulante->prox_instruccion,sizeof(int));
	offset += sizeof(int);
	memcpy((posicion_inicial)+offset,&tripulante->puntero_pcb,sizeof(int));

}


// deserealizamos

tcbTripulante* obtener_tripulante(void* inicio_tripulantes){
	tcbTripulante* auxiliar=malloc(sizeof(tcbTripulante));
	int desplazamiento = 0;
	memcpy(&(auxiliar->tid), inicio_tripulantes+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(auxiliar->estado), inicio_tripulantes+desplazamiento, sizeof(char));
	desplazamiento += sizeof(char);
	memcpy(&(auxiliar->posicionX), inicio_tripulantes+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(auxiliar->posicionY), inicio_tripulantes+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(auxiliar->prox_instruccion), inicio_tripulantes+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(auxiliar->puntero_pcb), inicio_tripulantes+desplazamiento, sizeof(int));

	return auxiliar;
}

/*
pcbPatota* obtenerPCB(int id_patota){           //Recibe ID de patota y devuelve PCB alojado en memoria
	pcbPatota* leida = malloc(sizeof(pcbPatota));
	memcpy(&leida, marco_tarea(posicion_patota(id_patota)),sizeof(leida));

	return leida;
}




}

void ubicarTarea (patota_aux estructura_a,int n_tarea){

}


char* tarea_a_string(tarea* t){
	char* tarea_s =string_nombre_tarea(t->tarea);
	char* buffer;
	char buffer_c[50];
	char tarea[50];
	sprintf(buffer_c," %d;%d;%d;%d|",t->parametro,t->pos_x,t->pos_y,t->tiempo );

	size_t n = strlen(tarea_s) + 1;

	char *s = malloc(n );
	strcpy(s,tarea_s);
	strcat(s,buffer_c);
	return s;



}
/*

void escribirTripulante(tripulante, tamanio_pag-ocupacion){
	if(tamanio_pag-ocupacion>tamanio_id)
	copiar_id;
	ocupado++=copiar_id
    if(tamanio_pag-ocupado>tamanio_id)
	copiar_posicionx;
	if(....)
	copiar_posiciony;

	else{
	get_marco_vacio();
	ocupacion=0;
	}

}


*/
/*
void imprimir_tarea(int marco){

printf("Lo leido es %s\n",(configuracion.posicion_inicial+marco*atoi(configuracion.tamanio_pag)));

};*/

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

	int tamanio_marco = atoi( config_servidor->tamanio_pag);
	int nuevo_marco  = tamanio_marco;

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


	cantidad_marcos=por_atributo(nuevo_marco ,&tamanio_marco, longitud_tarea, cantidad_marcos);

    return cantidad_marcos;
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
    int size = list_size(tabla_espacios_de_memoria);
    printf("---------  MEMORIA  --------------------\n");

    for(int i=0; i < size; i++) {
    	espacio_de_memoria *espacio = list_get(tabla_espacios_de_memoria, i);
        printf("base: %d, tam: %d, libre: %s \n", espacio->base, espacio->tam, espacio->libre ? "true" : "false");
        //free(espacio);
    }
    printf("----------------------------------------\n");
}

void imprimir_tabla_segmentos_patota(tabla_segmentacion* tabla_segmentos_patota){
	printf("Tabla de segmentos correspondiente a patota %d\n", tabla_segmentos_patota->id_patota);

	for(int j=0; j < list_size(tabla_segmentos_patota->segmento_inicial); j++){
		segmento* segmento_leido = list_get(tabla_segmentos_patota->segmento_inicial, j);
		printf("Base %d   Tamanio %d\n", segmento_leido->base, segmento_leido->tamanio);
		//free(segmento_leido);
	}
	printf("---------------------------------------\n");
}


void eliminar_espacio_de_memoria(int base){
    for(int i = 0; i < list_size(tabla_espacios_de_memoria); i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

    	if(espacio->base == base){
            espacio->libre = true;
            //list_remove(tabla_espacios_de_memoria, i);
            //espacio_de_memoria* espacio_libre = crear_espacio_de_memoria(base, espacio->tam, true);
            //list_add(tabla_espacios_de_memoria, espacio_libre);

            //free(espacio_libre);
        }
    	//free(espacio);
    }
    ordenar_memoria();
}

void ordenar_memoria(){
    bool espacio_anterior(espacio_de_memoria* espacio_menor, espacio_de_memoria* espacio_mayor) {
        return espacio_menor->base < espacio_mayor->base;
    }

    list_sort(tabla_espacios_de_memoria, (void*) espacio_anterior);
}

void unir_espacios_contiguos_libres(){
    int size = list_size(tabla_espacios_de_memoria);

    for(int i=0; i < size-1; i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);
    	espacio_de_memoria* siguiente_espacio = list_get(tabla_espacios_de_memoria, i + 1);

        if (espacio->libre && siguiente_espacio->libre){
            espacio->tam += siguiente_espacio->tam;
            list_remove(tabla_espacios_de_memoria, i+1);
            free(siguiente_espacio);
            size = list_size(tabla_espacios_de_memoria);
            i = 0; //CREO QUE CON i-- ESTARIA BIEN
        }
    }
}

void eliminar_segmento(int nro_segmento, tabla_segmentacion* tabla_segmentos_patota){
	for(int i = 0; i < list_size(tabla_segmentos_patota->segmento_inicial); i++){
		segmento* segmento = list_get(tabla_segmentos_patota->segmento_inicial, i);

		if(segmento->numero_segmento == nro_segmento){
			list_remove(tabla_segmentos_patota->segmento_inicial, i);
			//list_remove_and_destroy_element(t_list *, int index, void(*element_destroyer)(void*));
			eliminar_espacio_de_memoria(segmento->base);
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
    for(int i=0; i < list_size(tabla_espacios_de_memoria); i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);
        if(espacio->libre == true && tam <= espacio->tam){
            return espacio;
        }
    }
    log_warning(logger, "No hay espacios de memoria libres");
    return NULL;
}

espacio_de_memoria* busqueda_best_fit(int tam){
    int size = list_size(tabla_espacios_de_memoria);
    t_list* espacios_libres = list_create();

    for(int i=0; i < size; i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

    	if(espacio->libre == true && tam <= espacio->tam){

            if(tam == espacio->tam){ //si encuentra uno justo de su tamanio
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
        return espacio_best_fit;

    }else{
        log_warning(logger, "No hay espacios de memoria libres");
        return NULL;
    }
}

espacio_de_memoria* asignar_espacio_de_memoria(size_t tam) { //falta
	espacio_de_memoria *espacio_libre = buscar_espacio_de_memoria_libre(tam);
	if (espacio_libre != NULL) {

		if (espacio_libre->tam == tam) {//Si el espacio libre encontrado es de igual tamanio al segmento a alojar no es necesario ordenar. CAPAZ NO HACE FALTA
        	espacio_libre->libre = false;
            return espacio_libre;
        } else { //Si no es de igual tamanio, debo crear un nuevo espacio con base en el libre y reacomodar la base y tamanio del libre.
            espacio_de_memoria* nuevo_espacio_de_memoria = crear_espacio_de_memoria(espacio_libre->base, tam, false);
            list_add(tabla_espacios_de_memoria, nuevo_espacio_de_memoria);

            //actualizo base y tamanio del espacio libre
            espacio_libre->base += tam; //NO ENTIENDO COMO FUNCIONA SI ESPACIO_LIBRE ES LOCAL
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

bool patota_segmentacion(uint32_t pid, uint32_t cantidad_tripulantes, char* tarea, t_list* lista){
	pcbPatota* pcb_patota = crear_pcb(pid);
	espacio_de_memoria* espacio_de_memoria_pcb_patota = asignar_espacio_de_memoria(tamanio_PCB);
	if (espacio_de_memoria_pcb_patota == NULL){
		return false;
	}

	espacio_de_memoria* espacio_de_memoria_tareas = asignar_espacio_de_memoria(strlen(tarea)+1);
	if (espacio_de_memoria_tareas == NULL){
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
	tabla_segmentos_patota->segmento_inicial = list_create();

	list_add(tabla_segmentos_patota->segmento_inicial, segmento_pcb);
	list_add(tabla_segmentos_patota->segmento_inicial, segmento_tareas);

	tcbTripulante* tripulante = malloc(tamanio_TCB);
	for(int i=2; i < cantidad_tripulantes +2; i++){
		tripulante = (tcbTripulante*)list_get(lista,i);

		if(i == 2){//es el primer tripulante de la lista
			tabla_segmentos_patota->primer_tripulante = tripulante->tid;
		}

		espacio_de_memoria* espacio_de_memoria_tcb_tripulante = asignar_espacio_de_memoria(tamanio_TCB);
		if (espacio_de_memoria_tcb_tripulante == NULL){
			return false;
		}

		tripulante->puntero_pcb = 0; //direccion logica del pcb. Segmento pcb es el 0
		tripulante->prox_instruccion = 0;
		espacio_de_memoria_tcb_tripulante->contenido = tripulante;

		segmento* segmento_tcb = malloc(sizeof(segmento));
		segmento_tcb->numero_segmento = i;
		segmento_tcb->base = espacio_de_memoria_tcb_tripulante->base;
		segmento_tcb->tamanio = espacio_de_memoria_tcb_tripulante->tam;

		list_add(tabla_segmentos_patota->segmento_inicial, segmento_tcb);

		if(i == cantidad_tripulantes +2 -1){ //es el ultimo tripulante de la lista
			tabla_segmentos_patota->ultimo_tripulante = tripulante->tid;
		}
	}

	imprimir_tabla_espacios_de_memoria();

	imprimir_tabla_segmentos_patota(tabla_segmentos_patota);

	list_add(lista_tablas_segmentos, tabla_segmentos_patota);

	return true;
}

bool funcion_expulsar_tripulante(uint32_t tripulante_id){
	for(int i=0; i < list_size(lista_tablas_segmentos); i++){
		tabla_segmentacion* tabla_segmentos = (tabla_segmentacion*)list_get(lista_tablas_segmentos, i);

		if(tripulante_id <= tabla_segmentos->ultimo_tripulante){
			for(int j=2; j < list_size(tabla_segmentos->segmento_inicial); j++){
				segmento* segmento = list_get(tabla_segmentos->segmento_inicial, j);

				if(segmento->numero_segmento -2 == (tripulante_id - tabla_segmentos->primer_tripulante)){
					eliminar_segmento(segmento->numero_segmento, tabla_segmentos);
					log_info(logger, "Se expulso segmento %d", segmento->numero_segmento);

					unir_espacios_contiguos_libres();
					imprimir_tabla_espacios_de_memoria();
					imprimir_tabla_segmentos_patota(tabla_segmentos);

					//compactar_memoria();  //ACA PARA PROBAR NOMAS

					return true;
				}
			}
		}
	}
	return false;
}

void compactar_memoria(){
    log_info(logger, "Empieza compactacion");
    ordenar_memoria(); //VER CUANDO ESTE TOODO HECHO SI HACEN FALTA ESTAS 2 LINEAS
    unir_espacios_contiguos_libres();

    bool compacto_algo = false;

    for(int i=0; i < list_size(tabla_espacios_de_memoria)-1; i++){ //-1 para que no se fije en el ultimo espacio que es el libre
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

        if(espacio->libre == true){
        	compacto_algo = true;
        	log_info(logger, "espacio libre econtrado %d", espacio->base);

        	for(int k=0; k < list_size(lista_tablas_segmentos); k++){
        		tabla_segmentacion* tabla_segmentos = list_remove(lista_tablas_segmentos, k);

        		for(int l=0; l < list_size(tabla_segmentos->segmento_inicial); l++){
        			segmento* segmento = list_get(tabla_segmentos->segmento_inicial, l);

            		if (segmento->base > espacio->base){
            			//para segmento->base < espacio->base no hace nada porque significa que la memoria ya los paso y estan antes que el espacio libre
            			segmento = list_remove(tabla_segmentos->segmento_inicial, l);
            			segmento->base -= espacio->tam;
            			list_add_in_index(tabla_segmentos->segmento_inicial, l, segmento);
            		}
        		}

        		list_add_in_index(lista_tablas_segmentos, k, tabla_segmentos);
        	}

        	list_remove(tabla_espacios_de_memoria, i);

        	for(int j = i; j < list_size(tabla_espacios_de_memoria); j++){
        		espacio_de_memoria* espacio_temp = list_remove(tabla_espacios_de_memoria, j);
        		espacio_temp->base -= espacio->tam;

        		if(j == list_size(tabla_espacios_de_memoria)){ //esta recorriendo el ultimo espacio de memoria
        			espacio_temp->tam += espacio->tam; //entonces le amplia el tamanio porque es el espacio libre
        			list_add_in_index(tabla_espacios_de_memoria, j, espacio_temp);
        		} else {
        			list_add_in_index(tabla_espacios_de_memoria, j, espacio_temp);
        		}

        	}
            imprimir_tabla_espacios_de_memoria();
        }
    }
    if(compacto_algo == false){
    	log_info(logger, "No hay nada para compactar");
    }

    log_info(logger, "Termina compactacion");
}

