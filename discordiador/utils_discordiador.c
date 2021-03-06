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



tcbTripulante* crear_tripulante(uint32_t tid, char estado, uint32_t posicionX, uint32_t posicionY, uint32_t puntero_pcb, int cantidad_tripulantes){
	tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
	tripulante->tid = tid;
	tripulante->estado = estado;
	tripulante->posicionX = posicionX;
	tripulante->posicionY = posicionY;
	tripulante->prox_instruccion = 0;
	tripulante->puntero_pcb = puntero_pcb;
	sem_init(&(tripulante->semaforo_tripulante),0,0);
	tripulante->socket_miram = 0;
	tripulante->socket_mongo = 0;
	//tripulante->tarea_posta = malloc(sizeof(tarea));
	tripulante->tarea_posta = NULL;
	tripulante->fui_expulsado = false;
	tripulante->cant_tripus_patota = cantidad_tripulantes;
	return tripulante;
}

//pcbPatota* crear_patota(uint32_t pid, uint32_t tareas){
//	pcbPatota* patota = malloc(sizeof(pcbPatota));
//	patota->pid = pid;
//	patota->tareas = tareas;
//	return patota;
//}

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
	free(paquete);
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
			limpiar_array(codigo);
			return conversionProceso[j].valor;
		}
	}
	limpiar_array(codigo);
	return -1;
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

char* recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	return buffer;
}

/*Operaciciones para mostrar en discordiador*/

/*t_list* recibir_lista_tripulantes(int tipoMensaje, int conexionMiRam, t_log* logger){
	tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
	t_list* lista;

	if (tipoMensaje == 1){
		printf("recibi el paquete indicado y se guardara en la lista de ready");
		//list_add(lista, 1);
		//list_add(lista, 2);
		//list_add(lista, 3);
		//list_add(lista, 4);
		//lista = recibir_paquete(conexionMiRam);
		/*tripulante = (nuevoTripulante*)list_get(lista, 0);
		printf("\n ID: %d \n", tripulante->id );
		printf("Posicion X: %d \n", tripulante->posicionX );
		printf("Posicion Y: %d \n", tripulante->posicionY );
		printf("Pertenece a Patota: %d \n", tripulante->numeroPatota );
		printf("\n");
	}else {
		mensajeError(logger);
	}
	free(tripulante);
	return lista;
}*/

/*TAMA??O DE LAS DIFERENTES ESTRUCTURAS*/

size_t tamanio_tcb (tcbTripulante* tripulante){
	size_t tamanio = sizeof(uint32_t)*5;
	tamanio += sizeof(char);
	tamanio += sizeof(sem_t);
	return tamanio;
}

size_t tamanio_pcb(pcbPatota* patota){
	size_t tamanio = sizeof(uint32_t)*2;
	return tamanio;
}


/*void leer_tareas(char* archTarea, char* *tareas){
	   FILE *fp;
	   char *item;
	   char linea[200]; //reever este 200
	   strcpy(*tareas, "");
	   fp = fopen(archTarea, "r");
	   if (fp == NULL)
	     {
	        perror("Error al abrir el archivo.\n");
	        exit(EXIT_FAILURE);
	     }
	   tarea* leida = malloc(sizeof(tarea));
	   int contador_tareas =1;
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

		   char* string_tarea = imprimirTarea(leida);
		   *tareas = realloc(*tareas, (strlen(string_tarea)*contador_tareas)+1);
		   strcat (*tareas, string_tarea);

		   contador_tareas++;
		   //free(mensaje); Aca liberar ya que antes hice malloc(20)
	   }

	   printf("Las tareas son %s\n",*tareas);
}*/

char* leer_tareas_archivo(char* archTarea){
	   FILE *fp;
	   char ultimo_caracter;
	   char* tareass = malloc(1);
	   strcpy(tareass, "");
	   fp = fopen(archTarea, "r");
	   if (fp == NULL)
	     {
	        perror("Error al abrir el archivo.\n");
	        exit(EXIT_FAILURE);
	     }

	   char *linea_leida = NULL;
	   size_t linea_leida_tam = 0;
	   int line_count = 0;
	   ssize_t line_size;

	   line_size = getline(&linea_leida, &linea_leida_tam, fp);
	   while (line_size >= 0){
		   line_count++;

		   ultimo_caracter = linea_leida[strlen(linea_leida)-1];
		   linea_leida[strlen(linea_leida)-1] = '-';
		   tareass = realloc(tareass, strlen(linea_leida)+strlen(tareass)+1);
		   strcat(tareass, linea_leida);

		   line_size = getline(&linea_leida, &linea_leida_tam, fp);
	   }
	   tareass[strlen(tareass)-1] = ultimo_caracter;
	   printf("Las tareas son %s\n",tareass);

	   free(linea_leida);
	   linea_leida = NULL;

	   return tareass;
}

tarea* transformar_char_tarea(char* char_tarea){
	   tarea* tarea_leida = malloc(sizeof(tarea));

	   char** partes = string_split(char_tarea,";");

	   if(strchr(partes[0], ' ') != NULL){ // tiene un espacio => tiene parametro => es tarea E/S
		   tarea_leida->tarea = strtok(partes[0]," ");
		   tarea_leida->parametro = atoi(strtok(NULL,""));
	   }else{ //no tiene parametro => es tarea normal
		   tarea_leida->tarea = partes[0];
		   tarea_leida->parametro = NULL;
	   }

	   tarea_leida->pos_x = atoi(partes[1]);
	   tarea_leida->pos_y = atoi(partes[2]);
	   tarea_leida->tiempo = atoi(partes[3]);

	   limpiar_array(partes);
	   return tarea_leida;
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

/*char* imprimirTarea(tarea* aimprimir){

	char *mensaje = malloc(sizeof(char*)); //20 por las duads. Habitualmente se utilizan 10.

	sprintf(mensaje ,"%d", aimprimir->tarea);
	strcat (mensaje, "-");
	sprintf(mensaje  + strlen(mensaje),"%i", aimprimir->parametro);
	strcat (mensaje, "-");
	sprintf(mensaje  + strlen(mensaje),"%d", aimprimir->pos_x);
	strcat (mensaje, "-");
	sprintf(mensaje  + strlen(mensaje),"%d", aimprimir->pos_y);
	strcat (mensaje, "-");
	sprintf(mensaje  + strlen(mensaje),"%d", aimprimir->tiempo);
	strcat (mensaje, ";");
	//printf("El mensaje es: %s", mensaje);
	return mensaje;
}*/

//tarea* crear_tarea(tarea_tripulante cod_tarea,int parametro,int pos_x,int pos_y,int tiempo){
/*tarea* crear_tarea(char* cod_tarea,int parametro,int pos_x,int pos_y,int tiempo){
	tarea* tarea_recibida = malloc(sizeof(tarea));
	tarea_recibida->tarea=cod_tarea;
	tarea_recibida->parametro=parametro;
	tarea_recibida->pos_x=pos_x;
	tarea_recibida->pos_y=pos_y;
	tarea_recibida->tiempo=tiempo;
	return tarea_recibida;
}*/

/*tcbTripulante* hacer_tarea(tcbTripulante* tripulante,tarea* tarea_recibida){
	tripulante->posicionX=tarea_recibida->pos_x;
	tripulante->posicionY=tarea_recibida->pos_y;
	ejecutar_tarea(tarea_recibida->tarea,tarea_recibida->parametro);
	sleep(tarea_recibida->tiempo);
	return tripulante; //Enviar a MiRam (Actualizar tcb)
}*/

/*void ejecutar_tarea(tarea_tripulante cod_tarea,int parametro){

	switch(cod_tarea){

	case GENERAR_OXIGENO:
			for(;parametro>0;parametro--){
				printf("O");
			}
			printf("\n");
			break;

	case GENERAR_COMIDA:
			for(;parametro>0;parametro--){
				printf("C");
			}
			printf("\n");
			break;

	case GENERAR_BASURA:
			for(;parametro>0;parametro--){
				printf("B");
			}
			printf("\n");
			break;

	default:
		printf("No existe codigo de tarea\n");
		break;

	}
}*/

tarea* pedir_tarea(int conexion_miram, tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(PEDIR_TAREA);

	char* tid_char = malloc(sizeof(char)+1);
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* numero_patota_char = malloc(sizeof(char)+1);
	sprintf(numero_patota_char, "%d", tripulante->puntero_pcb);
	agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);

	enviar_paquete(paquete, conexion_miram);

	char* tarea_recibida_miram = recibir_mensaje(conexion_miram);
	log_info(logger, "TRIPU %d  tarea recibida: %s", tripulante->tid, tarea_recibida_miram);

	if(strcmp("no hay mas tareas", tarea_recibida_miram) ==0){
		return NULL;
	}

	tripulante->tarea_posta = transformar_char_tarea(tarea_recibida_miram);

	tripulante->prox_instruccion++;

	eliminar_paquete(paquete);
	free(numero_patota_char);
	free(tid_char);
	free(tarea_recibida_miram);

	return tripulante->tarea_posta;
}

void cambiar_estado(int conexion_miram, tcbTripulante* tripulante, char nuevo_estado){
	t_paquete* paquete = crear_paquete(CAMBIAR_DE_ESTADO);

	char* tid_char = malloc(sizeof(char)+1);
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* estado_char = malloc(sizeof(char)+1);
	sprintf(estado_char, "%c", nuevo_estado);
	agregar_a_paquete(paquete, estado_char, strlen(estado_char)+1);

	char* numero_patota_char = malloc(sizeof(char)+1);
	sprintf(numero_patota_char, "%d", tripulante->puntero_pcb);
	agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);

	enviar_paquete(paquete, conexion_miram);

	char* mensaje_recibido = recibir_mensaje(conexion_miram);
	if(strcmp(mensaje_recibido, "fallo cambio de estado") ==0){
		log_info(logger, "fallo cambio estado");
	}

	eliminar_paquete(paquete);
	free(tid_char);
	free(estado_char);
	free(numero_patota_char);
	free(mensaje_recibido);
}

void informar_movimiento(int conexion_miram, tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(INFORMAR_MOVIMIENTO);

	char* tid_char = malloc(sizeof(char)+1);
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* posx_char = malloc(sizeof(char)+1);
	sprintf(posx_char, "%d", tripulante->posicionX);
	agregar_a_paquete(paquete, posx_char, strlen(posx_char)+1);

	char* posy_char = malloc(sizeof(char)+1);
	sprintf(posy_char, "%d", tripulante->posicionY);
	agregar_a_paquete(paquete, posy_char, strlen(posy_char)+1);

	char* numero_patota_char = malloc(sizeof(char)+1);
	sprintf(numero_patota_char, "%d", tripulante->puntero_pcb);
	agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);

	enviar_paquete(paquete, conexion_miram);

	char* mensaje_recibido = recibir_mensaje(conexion_miram);
	//log_info(logger, "TRIPU %d  %s", tripulante->tid, mensaje_recibido);

	eliminar_paquete(paquete);
	free(tid_char);
	free(posx_char);
	free(posy_char);
	free(numero_patota_char);
	free(mensaje_recibido);
}

void informar_movimiento_mongo_X (tcbTripulante* tripulante, int x_viejo){
	t_paquete* paquete = crear_paquete(INFORMAR_BITACORA_MOVIMIENTO);

	char* tid_char = malloc(sizeof(char)+1);
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* posicion_vieja [3];

	char* x_viejo_char = malloc(sizeof(char)+1);
	sprintf(x_viejo_char, "%d", x_viejo);
	strcpy(posicion_vieja, x_viejo_char);

	strcat(posicion_vieja, "|");

	char* y_actual = malloc(sizeof(char)+1);
	sprintf(y_actual, "%d", tripulante->posicionY);
	strcat(posicion_vieja, y_actual);

	agregar_a_paquete(paquete, posicion_vieja, strlen(posicion_vieja)+1);


	char* posicion_actual [3];

	char* x_actual = malloc(sizeof(char)+1);
	sprintf(x_actual, "%d", tripulante->posicionX);
	strcpy(posicion_actual, x_actual);

	strcat(posicion_actual, "|");
	strcat(posicion_actual, y_actual);

	agregar_a_paquete(paquete, posicion_actual, strlen(posicion_actual)+1);

	//enviar_paquete(paquete, tripulante->socket_mongo);
	enviar_paquete(paquete, tripulante->socket_miram);

	eliminar_paquete(paquete);
	free(tid_char);
	free(x_viejo_char);
	free(y_actual);
	free(x_actual);
}

void informar_movimiento_mongo_Y (tcbTripulante* tripulante, int y_viejo){
	t_paquete* paquete = crear_paquete(INFORMAR_BITACORA_MOVIMIENTO);

	char* tid_char = malloc(sizeof(char)+1);
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* posicion_vieja [3];

	char* x_actual = malloc(sizeof(char)+1);
	sprintf(x_actual, "%d", tripulante->posicionX);
	strcpy(posicion_vieja, x_actual);

	strcat(posicion_vieja, "|");

	char* y_viejo_char = malloc(sizeof(char)+1);
	sprintf(y_viejo_char, "%d", y_viejo);
	strcat(posicion_vieja, y_viejo_char);

	agregar_a_paquete(paquete, posicion_vieja, strlen(posicion_vieja)+1);


	char* posicion_actual [3];

	strcpy(posicion_actual, x_actual);
	strcat(posicion_actual, "|");

	char* y_actual = malloc(sizeof(char)+1);
	sprintf(y_actual, "%d", tripulante->posicionY);
	strcat(posicion_actual, y_actual);

	agregar_a_paquete(paquete, posicion_actual, strlen(posicion_actual)+1);

	//enviar_paquete(paquete, tripulante->socket_mongo);
	enviar_paquete(paquete, tripulante->socket_miram);

	eliminar_paquete(paquete);
	free(tid_char);
	free(x_actual);
	free(y_viejo_char);
	free(y_actual);
}

void informar_inicio_tarea(tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(INFORMAR_BITACORA);

	char* tid_char = malloc(sizeof(char)+1);
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	//char* mensaje = malloc(7);
	//mensaje = "inicio";
	//memset(mensaje, '\0', strlen(mensaje); //NOSE SI VA ESTO

	//int largo_mensaje = strlen(mensaje)+1;
	//char* largo_mensaje_char = malloc(sizeof(char));
	//sprintf(largo_mensaje_char, "%d", largo_mensaje);
	//agregar_a_paquete(paquete, largo_mensaje_char, strlen(largo_mensaje_char)+1);
	//agregar_a_paquete(paquete, mensaje, largo_mensaje);

	char* mensaje_bitacora_char = malloc(sizeof(char)+1);
	sprintf(mensaje_bitacora_char, "%d", INICIO_TAREA);
	agregar_a_paquete(paquete, mensaje_bitacora_char, strlen(mensaje_bitacora_char)+1);

	//enviar_paquete(paquete, tripulante->socket_mongo);
	enviar_paquete(paquete, tripulante->socket_miram);

	//char* mensaje_recibido = recibir_mensaje(tripulante->socket_mongo);

	eliminar_paquete(paquete);
	free(tid_char);
	free(mensaje_bitacora_char);
	//free(mensaje);
	//free(largo_mensaje_char);
	//free(mensaje_recibido);
}

void informar_fin_tarea(tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(INFORMAR_BITACORA);

	char* tid_char = malloc(sizeof(char)+1);
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	//char* mensaje = malloc(4);
	//mensaje = "fin";
	//memset(mensaje, '\0', strlen(mensaje); //NOSE SI VA ESTO

	//int largo_mensaje = strlen(mensaje)+1;
	//char* largo_mensaje_char = malloc(sizeof(char));
	//sprintf(largo_mensaje_char, "%d", largo_mensaje);
	//agregar_a_paquete(paquete, largo_mensaje_char, strlen(largo_mensaje_char)+1);
	//agregar_a_paquete(paquete, mensaje, largo_mensaje);

	char* mensaje_bitacora_char = malloc(sizeof(char)+1);
	sprintf(mensaje_bitacora_char, "%d", FIN_TAREA);
	agregar_a_paquete(paquete, mensaje_bitacora_char, strlen(mensaje_bitacora_char)+1);

	//enviar_paquete(paquete, tripulante->socket_mongo);
	enviar_paquete(paquete, tripulante->socket_miram);

	//char* mensaje_recibido = recibir_mensaje(tripulante->socket_mongo);

	eliminar_paquete(paquete);
	free(tid_char);
	free(mensaje_bitacora_char);
	//free(mensaje);
	//free(largo_mensaje_char);
	//free(mensaje_recibido);
}

void remover_tripulante_de_lista(tcbTripulante* tripulante, t_list* lista){
	for(int i=0; i < list_size(lista); i++){
		tcbTripulante* tripulante_de_lista = (tcbTripulante*)list_get(lista, i);

		if(tripulante_de_lista->tid == tripulante->tid){
			list_remove(lista, i);
			i = list_size(lista); //para que corte el for
		}
	}
}

void informar_atencion_sabotaje(tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(INFORMAR_BITACORA);

	char* tid_char = malloc(sizeof(char));
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	//char* mensaje = malloc(17);
	//mensaje = "corro a sabotaje";
	//memset(mensaje, '\0', strlen(mensaje); //NOSE SI VA ESTO

	//int largo_mensaje = strlen(mensaje)+1;
	//char* largo_mensaje_char = malloc(sizeof(char));
	//sprintf(largo_mensaje_char, "%d", largo_mensaje);
	//agregar_a_paquete(paquete, largo_mensaje_char, strlen(largo_mensaje_char)+1);
	//agregar_a_paquete(paquete, mensaje, largo_mensaje);

	char* mensaje_bitacora_char = malloc(sizeof(char));
	sprintf(mensaje_bitacora_char, "%d", CORRER_A_SABOTAJE);
	agregar_a_paquete(paquete, mensaje_bitacora_char, strlen(mensaje_bitacora_char)+1);

	//enviar_paquete(paquete, tripulante->socket_mongo);
	enviar_paquete(paquete, tripulante->socket_miram);

	//char* mensaje_recibido = recibir_mensaje(tripulante->socket_mongo);

	eliminar_paquete(paquete);
	free(tid_char);
	free(mensaje_bitacora_char);
	//free(mensaje);
	//free(largo_mensaje_char);
	//free(mensaje_recibido);
}

void informar_sabotaje_resuelto(tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(INFORMAR_BITACORA);

	char* tid_char = malloc(sizeof(char));
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	//char* mensaje = malloc(9);
	//mensaje = "resuelto";
	//memset(mensaje, '\0', strlen(mensaje); //NOSE SI VA ESTO

	//int largo_mensaje = strlen(mensaje)+1;
	//char* largo_mensaje_char = malloc(sizeof(char));
	//sprintf(largo_mensaje_char, "%d", largo_mensaje);
	//agregar_a_paquete(paquete, largo_mensaje_char, strlen(largo_mensaje_char)+1);
	//agregar_a_paquete(paquete, mensaje, largo_mensaje);

	char* mensaje_bitacora_char = malloc(sizeof(char));
	sprintf(mensaje_bitacora_char, "%d", RESOLVER_SABOTAJE);
	agregar_a_paquete(paquete, mensaje_bitacora_char, strlen(mensaje_bitacora_char)+1);

	//enviar_paquete(paquete, tripulante->socket_mongo);
	enviar_paquete(paquete, tripulante->socket_miram);

	//char* mensaje_recibido = recibir_mensaje(tripulante->socket_mongo);

	eliminar_paquete(paquete);
	free(tid_char);
	free(mensaje_bitacora_char);
	//free(mensaje);
	//free(largo_mensaje_char);
	//free(mensaje_recibido);
}

void liberar_memoria_tripu(tcbTripulante* tripu){
	free(tripu->tarea_posta);
	free(tripu);
}
