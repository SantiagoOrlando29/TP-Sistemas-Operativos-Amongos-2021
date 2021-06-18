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
			case PRUEBA:
				lista=recibir_paquete(socket_cliente);

				uint32_t pid = (uint32_t*)atoi(list_get(lista,0));
				printf("el numero de la patota es %d", pid);
				patota = crear_patota(pid,0);

				uint32_t cantidad_tripulantes = (uint32_t*)atoi(list_get(lista,1));
				printf("cant tripu %d\n", cantidad_tripulantes);

				for(int i=2; i < cantidad_tripulantes +2; i++){
					tripulante=(tcbTripulante*)list_get(lista,i);
					mostrar_tripulante(tripulante,patota);
					printf("\n");
				}

				char* tarea = malloc((uint32_t*)atoi(list_get(lista, cantidad_tripulantes+2)));
				tarea=(char*)list_get(lista, cantidad_tripulantes+3);
				printf("Las tareas serializadas son: %s \n", tarea);

				//---------------------------SEGMENTACION--------------------------------------------------
				/*
				pcbPatota* pcb_patota = crear_pcb(pid);
				espacio_de_memoria* espacio_de_memoria_pcb_patota = asignar_espacio_de_memoria(tamanio_pcb(pcb_patota));

				espacio_de_memoria* espacio_de_memoria_tareas = asignar_espacio_de_memoria(strlen(tarea)+1);
				pcb_patota->tareas = espacio_de_memoria_tareas->base;

				segmento* segmento_pcb = malloc(sizeof(segmento));
				segmento* segmento_tareas = malloc(sizeof(segmento));

				segmento_pcb->numero_segmento = 0;
				segmento_pcb->base = espacio_de_memoria_pcb_patota->base;
				segmento_pcb->tamanio = espacio_de_memoria_pcb_patota->tam;

				segmento_tareas->numero_segmento = 1;
				segmento_tareas->base = espacio_de_memoria_tareas->base;
				segmento_tareas->tamanio = espacio_de_memoria_tareas->tam;

				tabla_segmentacion* tabla_segmentos_patota = malloc(sizeof(tabla_segmentacion));

				tabla_segmentos_patota->id_patota = 1;
				tabla_segmentos_patota->segmento_inicial = list_create();

				list_add(tabla_segmentos_patota->segmento_inicial, segmento_pcb);
				list_add(tabla_segmentos_patota->segmento_inicial, segmento_tareas);

				for(int i=2; i < cantidad_tripulantes +2; i++){
					tripulante=(tcbTripulante*)list_get(lista,i);

					espacio_de_memoria* espacio_de_memoria_tcb_tripulante = asignar_espacio_de_memoria(tamanio_tcb(tripulante));

					segmento* segmento_tcb = malloc(sizeof(segmento));
					segmento_tcb->numero_segmento = i;
					segmento_tcb->base = espacio_de_memoria_tcb_tripulante->base;
					segmento_tcb->tamanio = espacio_de_memoria_tcb_tripulante->tam;

					list_add(tabla_segmentos_patota->segmento_inicial, segmento_tcb);
				}

				imprimir_tabla_espacios_de_memoria();

				imprimir_tabla_segmentos_patota(tabla_segmentos_patota);

				log_info(logger, "elimino");
				eliminar_segmento(5, tabla_segmentos_patota);

				imprimir_tabla_espacios_de_memoria();

				imprimir_tabla_segmentos_patota(tabla_segmentos_patota);

				*/
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
				variable_servidor = 0;
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
