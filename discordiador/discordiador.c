#include "discordiador.h"
#include <semaphore.h>


config_discordiador configuracion;
//config_struct configuracion;
sem_t AGREGAR_NUEVO_A_READY;
sem_t NUEVO_READY;
sem_t HABILITA_EJECUTAR;
sem_t HABILITA_GRADO_MULTITAREA;
sem_t PASA_A_BLOQUEADO;
sem_t MUTEX_LISTA_READY;
sem_t MUTEX_LISTA_TRABAJANDO;
sem_t MUTEX_LISTA_BLOQUEADO;
sem_t CONTINUAR_PLANIFICACION;
int id_tripulante = 0;
t_list* lista_tripulantes_nuevo;
t_list* lista_tripulantes_ready;
t_list* lista_tripulantes_bloqueado;
t_list* lista_tripulantes_trabajando;
t_list* lista_tripulantes_exit;
t_list* lista_bloq_emergencia;


bool sigue_ejecutando(int quantums_ejecutados){
	if(strcmp(configuracion.algoritmo, "FIFO") ==0){
		return true;
	}else if(list_size(lista_tripulantes_ready) == 0){//O SEA QUE HAY 0 EN READY, ENTONCES NO TIENE QUE BLOQUEARSE POR FIN DE QUANTUM
		return true;
	}else if(quantums_ejecutados < configuracion.quantum){ //RR
		return true;
	}
	return false;
}

void termina_quantum(int* quantums_ejecutados, tcbTripulante* tripulante){ //pensar otro nombre para la funcion
	bool flag_sigue_ejecutando = sigue_ejecutando(*quantums_ejecutados);
	if(flag_sigue_ejecutando == false ){
		planificacion_pausada_o_no();

		printf("hilo %d, FIN DE Q \n", tripulante->tid);

		planificacion_pausada_o_no();

		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		remover_tripulante_de_lista(tripulante, lista_tripulantes_trabajando);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'R';
		cambiar_estado(tripulante->socket_miram, tripulante, 'R');

		sem_wait(&MUTEX_LISTA_READY);
		list_add(lista_tripulantes_ready, tripulante);
		sem_post(&MUTEX_LISTA_READY);

		sem_post(&HABILITA_GRADO_MULTITAREA);
		sem_post(&HABILITA_EJECUTAR);
		sem_wait(&(tripulante->semaforo_tripulante));
		cambiar_estado(tripulante->socket_miram, tripulante, 'E');
		*quantums_ejecutados = 0;
	}
}

tarea* pedir_tarea(int conexion_miram, tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(PEDIR_TAREA);

	char* tid_char = malloc(sizeof(char));
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* numero_patota_char = malloc(sizeof(char));
	sprintf(numero_patota_char, "%d", tripulante->puntero_pcb);
	agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);

	enviar_paquete(paquete, conexion_miram);

	char* tarea_recibida_miram = recibir_mensaje(conexion_miram);
	log_info(logger, "TRIPU %d  tarea recibida: %s", tripulante->tid, tarea_recibida_miram);

	if(strcmp("no hay mas tareas", tarea_recibida_miram) ==0){
		return NULL;
	}

	tarea* tarea_posta = transformar_char_tarea(tarea_recibida_miram);

	tripulante->prox_instruccion++;

	eliminar_paquete(paquete);
	free(numero_patota_char);
	free(tid_char);

	return tarea_posta;
}

void cambiar_estado(int conexion_miram, tcbTripulante* tripulante, char nuevo_estado){
	t_paquete* paquete = crear_paquete(CAMBIAR_DE_ESTADO);

	char* tid_char = malloc(sizeof(char));
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* estado_char = malloc(sizeof(char));
	sprintf(estado_char, "%c", nuevo_estado);
	agregar_a_paquete(paquete, estado_char, strlen(estado_char)+1);

	char* numero_patota_char = malloc(sizeof(char));
	sprintf(numero_patota_char, "%d", tripulante->puntero_pcb);
	agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);

	enviar_paquete(paquete, conexion_miram);

	char* mensaje_recibido = recibir_mensaje(conexion_miram);
	if(strcmp(mensaje_recibido, "fallo cambio de estado") ==0){
		log_info(logger, "fallo cambio estado");
	}
	//log_info(logger, "TRIPU %d  cambio estado: %s", tripulante->tid, mensaje_recibido);

	eliminar_paquete(paquete);
	free(tid_char);
	free(estado_char);
	free(numero_patota_char);
	free(mensaje_recibido);
}

void informar_movimiento(int conexion_miram, tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(INFORMAR_MOVIMIENTO);

	char* tid_char = malloc(sizeof(char));
	sprintf(tid_char, "%d", tripulante->tid);
	agregar_a_paquete(paquete, tid_char, strlen(tid_char)+1);

	char* posx_char = malloc(sizeof(char));
	sprintf(posx_char, "%d", tripulante->posicionX);
	agregar_a_paquete(paquete, posx_char, strlen(posx_char)+1);

	char* posy_char = malloc(sizeof(char));
	sprintf(posy_char, "%d", tripulante->posicionY);
	agregar_a_paquete(paquete, posy_char, strlen(posy_char)+1);

	char* numero_patota_char = malloc(sizeof(char));
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

void planificacion_pausada_o_no(){
	sem_wait(&CONTINUAR_PLANIFICACION);
	sem_post(&CONTINUAR_PLANIFICACION);
}

void tripulante_hilo (tcbTripulante* tripulante){
	sem_wait(&(tripulante->semaforo_tripulante));

	tripulante->socket_miram = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	tarea* tarea = pedir_tarea(tripulante->socket_miram, tripulante);

	printf("hola soy el hilo %d, estoy listo para ejecutar \n", tripulante->tid);
	sem_post(&NUEVO_READY);

	fflush(stdout);
	sem_wait(&(tripulante->semaforo_tripulante));
	cambiar_estado(tripulante->socket_miram, tripulante, 'E');
	sem_post(&HABILITA_EJECUTAR);

	int contador_ciclos_trabajando = 0;
	int quantums_ejecutados = 0;

	while(tarea != NULL){

		while(tripulante->posicionX != tarea->pos_x){ // muevo en X
			planificacion_pausada_o_no();
			printf("hilo %d, me estoy moviendo X \n", tripulante->tid);

			if(tripulante->posicionX > tarea->pos_x){ //mayor a la posicion de la tarea -> se mueve para la izq
				tripulante->posicionX--;
			} else {
				tripulante->posicionX++;
			}

			informar_movimiento(tripulante->socket_miram, tripulante);
			sleep(1); // retardo ciclo cpu VA?  EN TODOS LOS SLEEP IRIA ESO?
			fflush(stdout);
			quantums_ejecutados++;
			termina_quantum(&quantums_ejecutados, tripulante);
		}

		while(tripulante->posicionY != tarea->pos_y){ // muevo en y
			planificacion_pausada_o_no();
			printf("hilo %d, me estoy moviendo Y \n", tripulante->tid);

			if(tripulante->posicionY > tarea->pos_y){ //mayor a la posicion de la tarea -> se mueve para abajo
				tripulante->posicionY--;
			} else{
				tripulante->posicionY++;
			}

			informar_movimiento(tripulante->socket_miram, tripulante);
			sleep(1); // retardo ciclo cpu VA?  EN TODOS LOS SLEEP IRIA ESO?
			fflush(stdout);
			quantums_ejecutados++;
			termina_quantum(&quantums_ejecutados, tripulante);
		}


		if((void*)tarea->parametro != NULL){ //TAREA DE I/O
			quantums_ejecutados++;
			termina_quantum(&quantums_ejecutados, tripulante);
			sleep(1); // retardo ciclo cpu VA? //sleep para eso de iniciar tarea I/O (simula peticion al SO). Nose si va en esta linea
			quantums_ejecutados = 0;

			planificacion_pausada_o_no();

			sem_wait(&MUTEX_LISTA_TRABAJANDO);
			remover_tripulante_de_lista(tripulante, lista_tripulantes_trabajando);
			sem_post(&MUTEX_LISTA_TRABAJANDO);

			tripulante->estado = 'B';

			sem_wait(&MUTEX_LISTA_BLOQUEADO);
			list_add(lista_tripulantes_bloqueado, tripulante);
			sem_post(&MUTEX_LISTA_BLOQUEADO);

			printf("hilo %d, me bloquie \n", tripulante->tid);
			cambiar_estado(tripulante->socket_miram, tripulante, 'B');
			sem_post(&PASA_A_BLOQUEADO);
			sem_post(&HABILITA_GRADO_MULTITAREA);
			sem_wait(&(tripulante->semaforo_tripulante));

			planificacion_pausada_o_no();

			cambiar_estado(tripulante->socket_miram, tripulante, 'E');

			if(tripulante->estado != 'X'){ // o sea que tiene mas tareas
				planificacion_pausada_o_no();
				tripulante->prox_instruccion--;
				tarea = pedir_tarea(tripulante->socket_miram, tripulante);

				sem_post(&HABILITA_EJECUTAR);

			} else{ // no tiene mas tareas
				tarea = NULL;
			}

		} else { //TAREA NORMAL
			while(contador_ciclos_trabajando < tarea->tiempo){
				planificacion_pausada_o_no();

				printf("hilo %d, estoy trabajando \n", tripulante->tid);
				sleep(1); // retardo ciclo cpu VA?
				contador_ciclos_trabajando++;
				quantums_ejecutados++;

				if(contador_ciclos_trabajando == tarea->tiempo){//ESTE IF ESTA PARA QUE SI YA TERMINO LA TAREA, SE FIJE SI TIENE OTRA MAS
					planificacion_pausada_o_no();
					tarea = pedir_tarea(tripulante->socket_miram, tripulante);

					if(tarea != NULL){ // TIENE MAS TAREAS
						termina_quantum(&quantums_ejecutados, tripulante);
					}
				}else{
					termina_quantum(&quantums_ejecutados, tripulante);
				}
			}
			contador_ciclos_trabajando = 0;
		}
	}

	if(tripulante->estado != 'X'){
		sem_post(&HABILITA_GRADO_MULTITAREA);

		planificacion_pausada_o_no();

		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		remover_tripulante_de_lista(tripulante, lista_tripulantes_trabajando);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'X';
		list_add(lista_tripulantes_exit, tripulante);
	}
	printf("hilo %d, EXIT\n", tripulante->tid);
	close(tripulante->socket_miram);
}

void ready_exec() {
	tcbTripulante* tripulante1 = malloc(sizeof(tcbTripulante));
	int lista_size;
	while(1){
		lista_size = list_size(lista_tripulantes_ready);
		if (lista_size >0){ //PARA QUE NO HAYA ESPERA ACTIVA PODRIA CAMBIAR ESTO POR UN SEMAFORO?
			sem_wait(&HABILITA_GRADO_MULTITAREA);
			sem_wait(&HABILITA_EJECUTAR);
			planificacion_pausada_o_no();
			tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_ready, 0);
			tripulante1->estado = 'E';

			sem_wait(&MUTEX_LISTA_TRABAJANDO);
			list_add(lista_tripulantes_trabajando, tripulante1);
			sem_post(&MUTEX_LISTA_TRABAJANDO);

			sem_post(&(tripulante1->semaforo_tripulante));
		}
	}
	free(tripulante1);
}

void nuevo_ready() {
	tcbTripulante* tripulante1 = malloc(sizeof(tcbTripulante));
	while(1){
		sem_wait(&AGREGAR_NUEVO_A_READY);

		planificacion_pausada_o_no();

		tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_nuevo, 0);
		sem_post(&(tripulante1->semaforo_tripulante));
		sem_wait(&NUEVO_READY);
		tripulante1->estado = 'R';

		sem_wait(&MUTEX_LISTA_READY);
		list_add(lista_tripulantes_ready, tripulante1);
		sem_post(&MUTEX_LISTA_READY);
	}
	free(tripulante1);

}

void bloqueado_ready() {
	tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
	while(1){
		sem_wait(&PASA_A_BLOQUEADO);

		sem_wait(&MUTEX_LISTA_BLOQUEADO);
		tripulante = (tcbTripulante*)list_get(lista_tripulantes_bloqueado, 0);
		sem_post(&MUTEX_LISTA_BLOQUEADO);

		planificacion_pausada_o_no();

		printf("hilo %d, haciendo I/O \n", tripulante->tid);
		sleep(1); // retardo ciclo cpu VA?

		planificacion_pausada_o_no();

		printf("hilo %d, termino I/O \n", tripulante->tid);
		fflush(stdout);

		planificacion_pausada_o_no();

		tarea* tarea = pedir_tarea(tripulante->socket_miram, tripulante);

		planificacion_pausada_o_no();

		sem_wait(&MUTEX_LISTA_BLOQUEADO);
		tripulante = (tcbTripulante*)list_remove(lista_tripulantes_bloqueado, 0);
		sem_post(&MUTEX_LISTA_BLOQUEADO);

		if(tarea != NULL){ // TIENE MAS TAREAS
			tripulante->estado = 'R';
			cambiar_estado(tripulante->socket_miram, tripulante, 'R');
			sem_wait(&MUTEX_LISTA_READY);
			list_add(lista_tripulantes_ready, tripulante);
			sem_post(&MUTEX_LISTA_READY);
		} else{ // NO TIENE MAS TAREAS
			tripulante->estado = 'X';
			list_add(lista_tripulantes_exit, tripulante);
			sem_post(&(tripulante->semaforo_tripulante));
		}

	}
	//free(tripulante);
}


int main(int argc, char* argv[]) {
	//config_struct configuracion;


	//Reinicio el anterior y arranco uno nuevo
	FILE* archivo = fopen("discordiador.log","w");
	fclose(archivo);
	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);

	/*char* archTarea = "tareas.txt";
	leer_tareas(archTarea);
	tarea* tarea_recibida1 = crear_tarea(GENERAR_OXIGENO,5,2,2,5);
	tarea* tarea_recibida2 = crear_tarea(GENERAR_COMIDA,7,2,2,5);
	tarea* tarea_recibida3 = crear_tarea(GENERAR_BASURA,9,2,2,5);
	hacer_tarea(tripulante,tarea_recibida1);
	hacer_tarea(tripulante,tarea_recibida2);
	hacer_tarea(tripulante,tarea_recibida3);*/


	leer_config();
	//leer numeros random
	//leer_tareas("tareas.txt");
	int conexionMiRam = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	int conexionMongoStore = crear_conexion(configuracion.ip_mongostore, configuracion.puerto_mongostore);

	menu_discordiador(conexionMiRam, conexionMongoStore,logger);

	terminar_discordiador(conexionMiRam, conexionMongoStore, logger);


	return 0;


}

int menu_discordiador(int conexionMiRam, int conexionMongoStore,  t_log* logger) {

	sem_init(&HABILITA_EJECUTAR, 0,1);
	sem_init(&NUEVO_READY, 0,0);
	sem_init(&AGREGAR_NUEVO_A_READY, 0,0);
	sem_init(&HABILITA_GRADO_MULTITAREA, 0,configuracion.grado_multitarea);
	sem_init(&PASA_A_BLOQUEADO, 0,0);
	sem_init(&MUTEX_LISTA_READY, 0,1);
	sem_init(&MUTEX_LISTA_TRABAJANDO, 0,1);
	sem_init(&MUTEX_LISTA_BLOQUEADO, 0,1);
	sem_init(&CONTINUAR_PLANIFICACION, 0,1);


	lista_tripulantes_nuevo = list_create();
	lista_tripulantes_ready = list_create();
	lista_tripulantes_bloqueado = list_create();
	lista_tripulantes_trabajando = list_create();
	lista_tripulantes_exit = list_create();
	lista_bloq_emergencia = list_create();

	bool pausar_planificacion_activado = false;
	uint32_t numero_patota = 0;

	pthread_t ready;
	pthread_t nuevo;
	pthread_t bloqueado;
	pthread_create(&ready,NULL,(void*)ready_exec,NULL);
	pthread_detach(&ready);
	pthread_create(&nuevo,NULL,(void*)nuevo_ready,NULL);
	pthread_detach(&nuevo);
	pthread_create(&bloqueado,NULL,(void*)bloqueado_ready,NULL);
	pthread_detach(&bloqueado);

	uint32_t tid =1;
	uint32_t posx = 0;
	uint32_t posy = 0;


	while(1){
		t_paquete* paquete;
		tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
		//char* nombreHilo = "";
		char* leido = readline("Iniciar consola: ");
		printf("\n");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:
				paquete = crear_paquete(INICIAR_PATOTA);
				numero_patota++;

				//agregar_entero_a_paquete(paquete, numero_patota, sizeof(uint32_t));
				char* numero_patota_char = malloc(sizeof(char));
				sprintf(numero_patota_char, "%d", numero_patota);
				agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);

				char** parametros = string_split(leido," ");
				uint32_t cantidad_tripulantes  = atoi(parametros[1]);

				char* cantidad_tripulantes_char = malloc(sizeof(char));
				sprintf(cantidad_tripulantes_char, "%d", cantidad_tripulantes);
				//agregar_entero_a_paquete(paquete, cantidad_tripulantes, sizeof(uint32_t));
				agregar_a_paquete(paquete, cantidad_tripulantes_char, strlen(cantidad_tripulantes_char)+1);

				//char* tareas = malloc(sizeof(char));
				//leer_tareas(parametros[2], &tareas);
				//printf("Las tareas recibidas por parametro son: %s\n", tareas);

				char* tareas = malloc(sizeof(char));
				tareas = leer_tareas_archivo(parametros[2]);
				printf("tareasssss %s \n", tareas);
/*
INICIAR_PATOTA 5 tareas.txt 300|4 10|20 4|500
INICIAR_PATOTA 5 tareas_corta.txt 300|4 10|20 4|500
*/
				bool hay_mas_parametros = true;
				for(int i = 0; i < cantidad_tripulantes ; i++){
					if (hay_mas_parametros == true){
						if (parametros[i+3] == NULL){
							hay_mas_parametros = false;
						} else {
							char* posiciones = parametros[i+3];
							char nuevo [8];
							char* item;
							item = strtok(posiciones,"|");
							posx = atoi(item);
							strcpy(nuevo,item);

							item = strtok(NULL,"");
							posy = atoi(item);

							strcat(nuevo,"|");
							strcat(nuevo,item);

							strcpy(parametros[i+3],nuevo);

						}
					}

					printf("El tripulante %d tiene posx %d y posy %d\n", tid, posx,posy);
					tripulante = crear_tripulante(tid,'N',posx,posy,0,numero_patota);
					agregar_a_paquete(paquete, tripulante, tamanio_TCB);
					posx =0;
					posy =0;
					tid++;
				}

				char* largo_tarea = malloc(sizeof(char));
				sprintf(largo_tarea, "%d", strlen(tareas)+1);
				agregar_a_paquete(paquete, largo_tarea, strlen(largo_tarea)+1);
				agregar_a_paquete(paquete, tareas, strlen(tareas)+1);
				//free(tareas);

				enviar_paquete(paquete, conexionMiRam);


				char* mensaje_recibido = malloc(17);
				mensaje_recibido = recibir_mensaje(conexionMiRam);
				log_info(logger, mensaje_recibido);

				if(strcmp(mensaje_recibido, "Memoria asignada") ==0){//VER SI HAY OTRA MANERA DE CREAR TRIPULANTES PARA NO REPETIR LO DE ARRIBA
					hay_mas_parametros = true;
					tid -= cantidad_tripulantes;
					for(int i = 0; i < cantidad_tripulantes ; i++){
						if (hay_mas_parametros == true){
							if (parametros[i+3] == NULL){
								hay_mas_parametros = false;
							} else {
								char* posiciones = parametros[i+3];
								char* item;
								item = strtok(posiciones,"|");
								posx = atoi(item);
								item = strtok(NULL,"");
								posy = atoi(item);
							}
						}

						//printf("El tripulante %d tiene posx %d y posy %d\n", tid, posx,posy);
						tripulante = crear_tripulante(tid,'N',posx,posy,0,numero_patota);
						pthread_t nombreHilo = (char*)(tid);
						pthread_create(&nombreHilo,NULL,(void*)tripulante_hilo,tripulante);
						pthread_detach(&nombreHilo);
						list_add(lista_tripulantes_nuevo, tripulante);
						sem_post(&AGREGAR_NUEVO_A_READY);

						posx =0;
						posy =0;
						tid++;
					}
				} else {
					log_warning(logger, "no se q tiene q pasar cuando miram no asigna memoria, pero no empieza x ahora");
				}


				eliminar_paquete(paquete);

				break;

			case LISTAR_TRIPULANTES:
				enviar_header(LISTAR_TRIPULANTES, conexionMiRam);
				int tipo_mensaje = recibir_operacion(conexionMiRam);
				if(tipo_mensaje == 11){  //NO_HAY_NADA_PARA_LISTAR
					log_info(logger, "No hay tripulantes en memoria para listar");

				}else{
					t_list* lista_tripulantes = recibir_paquete(conexionMiRam);

					for(int i=0; i < list_size(lista_tripulantes); i+=2){
						tcbTripulante* tripulante_recibido = (tcbTripulante*)list_get(lista_tripulantes, i);
						int numero_patota = (int)atoi(list_get(lista_tripulantes,i+1));
						printf("Tripulante: %d     Patota: %d     Status: %c \n", tripulante_recibido->tid, numero_patota, tripulante_recibido->estado);
					}

				}

				break;

			case INICIAR_PLANIFICACION:
				if (pausar_planificacion_activado == true){ //PARA QUE NO HAGA NADA SI NO PAUSE PLANIFICACION ANTES
					sem_post(&CONTINUAR_PLANIFICACION);
					pausar_planificacion_activado = false;
				}

				break;

			case PAUSAR_PLANIFICACION:
				if (pausar_planificacion_activado == false){ //PARA QUE NO PUEDA HACER DOS PAUSAR_PLANIFICACION SEGUIDOS SIN HACER UN INICIAR_PLANIFICACION
					sem_wait(&CONTINUAR_PLANIFICACION);
					pausar_planificacion_activado = true;
				}

				break;
/*
INICIAR_PATOTA 5 tareas.txt 300|4 10|20 4|500
INICIAR_PATOTA 5 tareas_corta.txt 3|4 10|20 4|5
*/

			case OBTENER_BITACORA:
				enviar_header(OBTENER_BITACORA, conexionMongoStore);
				int tipoMensaje = recibir_operacion(conexionMongoStore);
				log_info(logger, "tipoMensaje %d", tipoMensaje);
				t_list* lista = recibir_paquete(conexionMongoStore);
				char* mensaje = (char*)list_get(lista, 0);
				log_info(logger, mensaje);
				list_destroy(lista);
				break;

			case EXPULSAR_TRIPULANTE:
				paquete = crear_paquete(EXPULSAR_TRIPULANTE);
				char** parametro = string_split(leido," ");
				char* tripulante_id = parametro[1];
				agregar_a_paquete(paquete, tripulante_id, strlen(tripulante_id)+1);
				enviar_paquete(paquete, conexionMiRam);
				eliminar_paquete(paquete);
				break;
/*
INICIAR_PATOTA 5 tareas.txt 300|4 10|20 4|500
INICIAR_PATOTA 5 tareas_corta.txt 3|4 10|2 4|5
*/
			case PRUEBA: //para probar el sabotaje por ahora
				sem_wait(&CONTINUAR_PLANIFICACION);

				lista_bloq_emergencia = list_take_and_remove(lista_tripulantes_trabajando, list_size(lista_tripulantes_trabajando));

				for(int i=0; i < list_size(lista_tripulantes_ready); i++){
					tcbTripulante* tripulante_de_lista = malloc(sizeof(tcbTripulante));
					tripulante_de_lista = (tcbTripulante*)list_remove(lista_tripulantes_ready, i);
					list_add(lista_bloq_emergencia,tripulante_de_lista);
					//log_info(logger, "tid %d  posx %d  posy %d", tripulante_de_lista->tid, tripulante_de_lista->posicionX, tripulante_de_lista->posicionY);
					i = -1;
				}

				log_info(logger, "lista bloq emer");
				for(int i=0; i < list_size(lista_bloq_emergencia); i++){
					tcbTripulante* tripulante_de_listaa = malloc(sizeof(tcbTripulante));
					tripulante_de_listaa = (tcbTripulante*)list_get(lista_bloq_emergencia, i);
					log_info(logger, "tid %d  posx %d  posy %d", tripulante_de_listaa->tid, tripulante_de_listaa->posicionX, tripulante_de_listaa->posicionY);
				}

				tcbTripulante* tripu1 = malloc(sizeof(tcbTripulante));
				tripu1 = (tcbTripulante*)list_get(lista_bloq_emergencia, 0);
				tcbTripulante* tripu2 = malloc(sizeof(tcbTripulante));
				int dif_x = abs(tripu1->posicionX - 5);//NO ES 5, ES LO Q RECIBA DE POSICION
				int dif_y = abs(tripu1->posicionY - 5);

				for(int i=1; i < list_size(lista_bloq_emergencia); i++){
					tripu2 = (tcbTripulante*)list_get(lista_bloq_emergencia, i);

					int dif_x_2 = abs(tripu2->posicionX - 5);
					int dif_y_2 = abs(tripu2->posicionY - 5);

					if( dif_x + dif_y >= dif_x_2 + dif_y_2 ){ //esta mas cerca el segundo
						dif_x = dif_x_2;
						dif_y = dif_y_2;
						tripu1 = tripu2;
					}

				}
				log_info(logger,"aaaaaa");
				log_info(logger, "tid %d  posx %d  posy %d", tripu1->tid, tripu1->posicionX, tripu1->posicionY);

				//mover al tripu1 que es el mas cercano
				while(tripu1->posicionX != 5){//muevo en x
					if(tripu1->posicionX > 5){ //mayor a la posicion del sabotaje -> se mueve para la izq
						tripu1->posicionX--;
						//supongo que iran sleep()
					} else{
						tripu1->posicionX++;
					}
				}

				while(tripu1->posicionY != 5){//muevo en y
					if(tripu1->posicionY > 5){ //mayor a la posicion del sabotaje -> se mueve para abajo
						tripu1->posicionY--;
					} else{
						tripu1->posicionY++;
					}
				}

				sleep(configuracion.duracion_sabotaje);

				//devuelvo los tripulantes a sus correspondientes listas
				/*for(int i=0; i < list_size(lista_bloq_emergencia); i++){
					tcbTripulante* tripulante_de_listaa = malloc(sizeof(tcbTripulante));
					tripulante_de_listaa = (tcbTripulante*)list_remove(lista_bloq_emergencia, i);

					if(tripulante->estado = 'E'){

					}
				}*/

				sem_post(&CONTINUAR_PLANIFICACION);
				break;

			case FIN:
				paquete = crear_paquete(FIN);
				enviar_paquete(paquete, conexionMiRam);
				enviar_paquete(paquete, conexionMongoStore);
				eliminar_paquete(paquete);
				return EXIT_FAILURE;

			default:
				mensajeError(logger);
				break;
		}
		free(leido);

	}
}

void leer_config(void){
	t_config * archConfig = config_create("discordiador.config");

	configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
	configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");
	configuracion.ip_mongostore = config_get_string_value(archConfig, "IP_I_MONGO_STORE");
	configuracion.puerto_mongostore = config_get_string_value(archConfig, "PUERTO_I_MONGO_STORE");
	configuracion.grado_multitarea= config_get_int_value(archConfig, "GRADO_MULTITAREA");
	configuracion.algoritmo= config_get_string_value(archConfig, "ALGORITMO");
	configuracion.quantum = config_get_int_value(archConfig, "QUANTUM");
	configuracion.duracion_sabotaje = config_get_int_value(archConfig, "DURACION_SABOTAJE");
	configuracion.retardo_cpu = config_get_int_value(archConfig, "RETARDO_CICLO_CPU");
}

void terminar_discordiador (int conexionMiRam, int conexionMongoStore, t_log* logger){

	log_destroy(logger);
	liberar_conexion(conexionMiRam);
	liberar_conexion(conexionMongoStore);
}
