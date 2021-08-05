#include "discordiador.h"
#include <semaphore.h>


config_discordiador configuracion;
t_config * archConfig;
//config_struct configuracion;
sem_t AGREGAR_NUEVO_A_READY;
sem_t NUEVO_READY;
sem_t HABILITA_EJECUTAR;
sem_t HABILITA_GRADO_MULTITAREA;
sem_t PASA_A_BLOQUEADO;
sem_t MUTEX_LISTA_READY;
sem_t MUTEX_LISTA_TRABAJANDO;
sem_t MUTEX_LISTA_BLOQUEADO;
sem_t MUTEX_LISTA_EXIT;
sem_t CONTINUAR_PLANIFICACION;
sem_t ORDENA_FUNCION_QUANTUM;
sem_t FINALIZA_HILOS;
sem_t LISTA_READY_NO_VACIA;
sem_t TERMINO;
int id_tripulante = 0;
t_list* lista_tripulantes_nuevo;
t_list* lista_tripulantes_ready;
t_list* lista_tripulantes_bloqueado;
t_list* lista_tripulantes_trabajando;
t_list* lista_tripulantes_exit;
t_list* lista_bloq_emergencia;
t_list* lista_aux;
int flag_sabotaje;
int flag_fin;
int conexionMiRam;
int conexionMongoStore;
int conexionMongoSabotaje;


bool sigue_ejecutando(int quantums_ejecutados){
	if(strcmp(configuracion.algoritmo, "FIFO") ==0){
		return true;
	}else if(quantums_ejecutados < configuracion.quantum){ //RR
		return true;
	}
	return false;
}

void termina_quantum(int* quantums_ejecutados, tcbTripulante* tripulante){ //pensar otro nombre para la funcion
	bool flag_sigue_ejecutando = sigue_ejecutando(*quantums_ejecutados);
	if(flag_sigue_ejecutando == false){
		sem_wait(&ORDENA_FUNCION_QUANTUM);
		printf("hilo %d, FIN DE Q \n", tripulante->tid);

		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		remover_tripulante_de_lista(tripulante, lista_tripulantes_trabajando);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'R';
		cambiar_estado(tripulante->socket_miram, tripulante, 'R');

		sem_wait(&MUTEX_LISTA_READY);
		list_add(lista_tripulantes_ready, tripulante);
		sem_post(&MUTEX_LISTA_READY);

		sem_post(&LISTA_READY_NO_VACIA);

		sem_post(&ORDENA_FUNCION_QUANTUM);

		sem_post(&HABILITA_GRADO_MULTITAREA);
		sem_wait(&(tripulante->semaforo_tripulante));
		sem_post(&HABILITA_EJECUTAR);
		cambiar_estado(tripulante->socket_miram, tripulante, 'E');
		*quantums_ejecutados = 0;
	}
}

void planificacion_pausada_o_no(){
	sem_wait(&CONTINUAR_PLANIFICACION);
	sem_post(&CONTINUAR_PLANIFICACION);
}

void planificacion_pausada_o_no_exec(tcbTripulante* tripulante, int* quantums_ejecutados){
	sem_wait(&CONTINUAR_PLANIFICACION);
	sem_post(&CONTINUAR_PLANIFICACION);

	if(flag_sabotaje != 0){ //si esta en 0 es que NO hubo sabotaje
		flag_sabotaje--;

		*quantums_ejecutados = 0;
		sem_post(&HABILITA_GRADO_MULTITAREA);
		sem_post(&HABILITA_EJECUTAR);
		sem_wait(&(tripulante->semaforo_tripulante));
		//sem_post(&HABILITA_EJECUTAR);
	}
}

void tripulante_hilo (tcbTripulante* tripulante){
	sem_wait(&(tripulante->semaforo_tripulante));

	tripulante->socket_miram = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	tripulante->socket_mongo = crear_conexion(configuracion.ip_mongostore,configuracion.puerto_mongostore);

	tripulante->tarea_posta = pedir_tarea(tripulante->socket_miram, tripulante);

	printf("hola soy el hilo %d, estoy listo para ejecutar \n", tripulante->tid);
	sem_post(&NUEVO_READY);

	fflush(stdout);
	sem_wait(&(tripulante->semaforo_tripulante));
	cambiar_estado(tripulante->socket_miram, tripulante, 'E');
	sem_post(&HABILITA_EJECUTAR);

	int contador_ciclos_trabajando = 0;
	int quantums_ejecutados = 0;

	while(tripulante->tarea_posta != NULL){

		while(tripulante->posicionX != tripulante->tarea_posta->pos_x || tripulante->posicionY != tripulante->tarea_posta->pos_y){

			if(tripulante->posicionX != tripulante->tarea_posta->pos_x){ // muevo en X
				planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

				if(tripulante->fui_expulsado == false){
					printf("hilo %d, me estoy moviendo X \n", tripulante->tid);

					int x_viejo = tripulante->posicionX;
					if(tripulante->posicionX > tripulante->tarea_posta->pos_x){ //mayor a la posicion de la tarea -> se mueve para la izq
						tripulante->posicionX--;
					} else {
						tripulante->posicionX++;
					}

					informar_movimiento(tripulante->socket_miram, tripulante);
					informar_movimiento_mongo_X (tripulante, x_viejo);
				}

				sleep(configuracion.retardo_cpu);
				fflush(stdout);

				quantums_ejecutados++;
				planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

				if(tripulante->fui_expulsado == true){
					tripulante->posicionX = tripulante->tarea_posta->pos_x;
					tripulante->posicionY = tripulante->tarea_posta->pos_y;
					tripulante->estado = 'X';
				}else{
					if(quantums_ejecutados > 0){ //si es =0 significa que hubo sabotaje, entonces no me fijo si termino quantum porque se reinicia
						termina_quantum(&quantums_ejecutados, tripulante);
					}
				}
			}

			if(tripulante->posicionY != tripulante->tarea_posta->pos_y){ // muevo en y
				planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

				if(tripulante->fui_expulsado == false){
					printf("hilo %d, me estoy moviendo Y \n", tripulante->tid);

					int y_viejo = tripulante->posicionY;
					if(tripulante->posicionY > tripulante->tarea_posta->pos_y){ //mayor a la posicion de la tarea -> se mueve para abajo
						tripulante->posicionY--;
					} else{
						tripulante->posicionY++;
					}

					informar_movimiento(tripulante->socket_miram, tripulante);
					informar_movimiento_mongo_Y (tripulante, y_viejo);
				}

				sleep(configuracion.retardo_cpu);
				fflush(stdout);

				quantums_ejecutados++;
				planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

				if(tripulante->fui_expulsado == true){
					tripulante->posicionX = tripulante->tarea_posta->pos_x;
					tripulante->posicionY = tripulante->tarea_posta->pos_y;
					tripulante->estado = 'X';
				}else{
					if(quantums_ejecutados > 0){ //si es =0 significa que hubo sabotaje, entonces no me fijo si termino quantum porque se reinicia
						termina_quantum(&quantums_ejecutados, tripulante);
					}
				}
			}
		}


		if(tripulante->fui_expulsado == false){
			if(tripulante->tarea_posta->parametro != -1){ //tarea de E/S
				planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

				printf("hilo %d, inicio tarea e/s \n", tripulante->tid);
				mongo_tarea(tripulante);
				sleep(configuracion.retardo_cpu); //sleep para eso de iniciar tarea E/S (simula peticion al SO)

				planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

				if(tripulante->fui_expulsado == true){
					free(tripulante->tarea_posta->tarea);
					free(tripulante->tarea_posta);
					tripulante->tarea_posta = NULL;
					tripulante->estado = 'X';
					sem_post(&HABILITA_GRADO_MULTITAREA);
				}else{
					planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);
					quantums_ejecutados = 0;

					if(tripulante->posicionX == tripulante->tarea_posta->pos_x && tripulante->posicionY == tripulante->tarea_posta->pos_y){//veo si esta en la posicion de tarea por si resolvio sabotaje
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

						planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

						if(tripulante->estado != 'X'){ // o sea que tiene mas tareas
							cambiar_estado(tripulante->socket_miram, tripulante, 'E');
							//tarea = tripulante->tarea_posta;

							sem_post(&HABILITA_EJECUTAR);
						}else{ // vino de bloq sin mas tareas
							tripulante->tarea_posta = NULL;
						}
					}
				}

			} else { //tarea normal
				informar_inicio_tarea(tripulante);
				int flag_no_esta_en_posicion = 0;
				while(contador_ciclos_trabajando < tripulante->tarea_posta->tiempo && flag_no_esta_en_posicion ==0){

					planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

					if(tripulante->posicionX == tripulante->tarea_posta->pos_x && tripulante->posicionY == tripulante->tarea_posta->pos_y){//veo si esta en la posicion de tarea por si resolvio sabotaje

						if(tripulante->fui_expulsado == true){
							contador_ciclos_trabajando = tripulante->tarea_posta->tiempo; //para cortar el while
							tripulante->estado = 'X';
						}else{
							printf("hilo %d, estoy trabajando \n", tripulante->tid);
							sleep(configuracion.retardo_cpu);
							contador_ciclos_trabajando++;
							quantums_ejecutados++;

							if(contador_ciclos_trabajando != tripulante->tarea_posta->tiempo){ //todavia no termino la tarea
								planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados); //NO LO PENSE MUCHO
								if(quantums_ejecutados > 0){ //si es =0 significa que hubo sabotaje, entonces no me fijo si termino quantum porque se reinicia
									termina_quantum(&quantums_ejecutados, tripulante);
								}
//								termina_quantum(&quantums_ejecutados, tripulante);
							}
						}

					}else{
						flag_no_esta_en_posicion = 1;
					}
				}

				planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

				if(tripulante->fui_expulsado == true){
					tripulante->estado = 'X';
					free(tripulante->tarea_posta->tarea);
					free(tripulante->tarea_posta);
					tripulante->tarea_posta = NULL;
					sem_post(&HABILITA_GRADO_MULTITAREA);
				}else{
					planificacion_pausada_o_no_exec(tripulante, &quantums_ejecutados);

					if(flag_no_esta_en_posicion == 0){ // termino la tarea. Si esta en 1 -> va a moverse de nuevo
						informar_fin_tarea(tripulante);
						free(tripulante->tarea_posta->tarea);
						free(tripulante->tarea_posta);
						tripulante->tarea_posta = pedir_tarea(tripulante->socket_miram, tripulante);

						if(tripulante->tarea_posta != NULL){ // tiene mas tareas
							termina_quantum(&quantums_ejecutados, tripulante);
						}

						contador_ciclos_trabajando = 0;
					}
				}
			}
		}else{ //si tripu fue expulsado
			free(tripulante->tarea_posta->tarea);
			free(tripulante->tarea_posta);
			tripulante->tarea_posta = NULL;
			sem_post(&HABILITA_GRADO_MULTITAREA);
		}
	}

	//free(tarea);
	//free(tripulante->tarea_posta);

	if(tripulante->estado != 'X'){
		sem_post(&HABILITA_GRADO_MULTITAREA);

		//planificacion_pausada_o_no_exec(tripulante);

		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		remover_tripulante_de_lista(tripulante, lista_tripulantes_trabajando);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'X';

		sem_wait(&MUTEX_LISTA_EXIT);
		list_add(lista_tripulantes_exit, tripulante);
		sem_post(&MUTEX_LISTA_EXIT);
	}
	printf("hilo %d, EXIT\n", tripulante->tid);
	close(tripulante->socket_miram);

	int tripu_misma_patota_exit = 0;
	sem_wait(&MUTEX_LISTA_EXIT);
	for(int i=0; i < list_size(lista_tripulantes_exit); i++){
		tcbTripulante* tripu = (tcbTripulante*)list_get(lista_tripulantes_exit, i);
		if(tripu->puntero_pcb == tripulante->puntero_pcb){
			tripu_misma_patota_exit++;
		}
	}
	sem_post(&MUTEX_LISTA_EXIT);

	log_info(logger, "tripu_misma_patota_exit %d", tripu_misma_patota_exit);
	for(int i=0; i < list_size(lista_tripulantes_exit); i++){
		tcbTripulante* tripu = (tcbTripulante*)list_get(lista_tripulantes_exit, i);
		printf("tid %d\n", tripu->tid);
	}

	if(tripulante->cant_tripus_patota == tripu_misma_patota_exit){ //o sea ya estan en exit todos los tripus de esta misma patota
		int nro_patota = tripulante->puntero_pcb;
		sem_wait(&MUTEX_LISTA_EXIT);
		for(int i=0; i < list_size(lista_tripulantes_exit); i++){
			tcbTripulante* tripu = (tcbTripulante*)list_get(lista_tripulantes_exit, i);

			if(tripu->puntero_pcb == nro_patota){
				list_remove(lista_tripulantes_exit, i);
				free(tripu);
				i--;
			}
		}
		log_info(logger, "size exit %d", list_size(lista_tripulantes_exit));
		sem_post(&MUTEX_LISTA_EXIT);
	}
	enviar_header(FIN_HILO_TRIPULANTE, tripulante->socket_mongo);
}

void ready_exec() {
	//tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
	tcbTripulante* tripulante;
	while(flag_fin == 0){
		sem_wait(&LISTA_READY_NO_VACIA); //para que no haya espera activa
		if (list_size(lista_tripulantes_ready) >0){
			sem_wait(&HABILITA_GRADO_MULTITAREA);
			sem_wait(&HABILITA_EJECUTAR);

			planificacion_pausada_o_no();//CAPAZ NO HACE FALTA
			sem_wait(&MUTEX_LISTA_READY);
			tripulante = (tcbTripulante*)list_remove(lista_tripulantes_ready, 0);
			sem_post(&MUTEX_LISTA_READY);

			tripulante->estado = 'E';

			sem_wait(&MUTEX_LISTA_TRABAJANDO);
			list_add(lista_tripulantes_trabajando, tripulante);
			sem_post(&MUTEX_LISTA_TRABAJANDO);

			sem_post(&(tripulante->semaforo_tripulante));
		}
	}
	log_info(logger, "fin hilo ready_exec");
	//sem_post(&FINALIZA_HILOS);
	//liberar_memoria_tripu(tripulante);
}

void nuevo_ready() {
	//tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
	tcbTripulante* tripulante;
	while(flag_fin == 0){
		sem_wait(&AGREGAR_NUEVO_A_READY);

		planificacion_pausada_o_no();

		if(list_size(lista_tripulantes_nuevo) > 0){ //lo hago para liberar el hilo en el FIN
			tripulante = (tcbTripulante*)list_remove(lista_tripulantes_nuevo, 0);
			sem_post(&(tripulante->semaforo_tripulante));
			sem_wait(&NUEVO_READY);
			tripulante->estado = 'R';

			sem_wait(&MUTEX_LISTA_READY);
			list_add(lista_tripulantes_ready, tripulante);
			sem_post(&MUTEX_LISTA_READY);

			sem_post(&LISTA_READY_NO_VACIA);
		}
	}
	log_info(logger, "fin hilo nuevo_ready");
	//sem_post(&FINALIZA_HILOS);
	//liberar_memoria_tripu(tripulante);
}

void bloqueado_ready() {
	//tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
	tcbTripulante* tripulante;
	while(flag_fin == 0){
		sem_wait(&PASA_A_BLOQUEADO);

		if(list_size(lista_tripulantes_bloqueado) > 0){ //lo hago para liberar el hilo en el FIN
			sem_wait(&MUTEX_LISTA_BLOQUEADO);
			tripulante = (tcbTripulante*)list_get(lista_tripulantes_bloqueado, 0);
			sem_post(&MUTEX_LISTA_BLOQUEADO);

			planificacion_pausada_o_no();

			printf("hilo %d, empieza tiempo en Bloq \n", tripulante->tid);
			informar_inicio_tarea(tripulante);

			int ciclos_consumidos = 0;
			while(ciclos_consumidos < tripulante->tarea_posta->tiempo){
				sleep(configuracion.retardo_cpu);
				planificacion_pausada_o_no();

				if(tripulante->fui_expulsado == true){
					ciclos_consumidos = tripulante->tarea_posta->tiempo; //para cortar el while
				}

				ciclos_consumidos++;
			}

			planificacion_pausada_o_no();

			if(tripulante->fui_expulsado == true){
				tripulante->estado = 'X';

				sem_post(&(tripulante->semaforo_tripulante));
			}else{
				informar_fin_tarea(tripulante);
				printf("hilo %d, termina tiempo en Bloq \n", tripulante->tid);
				fflush(stdout);

				planificacion_pausada_o_no();

				sem_wait(&MUTEX_LISTA_BLOQUEADO);
				tripulante = (tcbTripulante*)list_remove(lista_tripulantes_bloqueado, 0);
				sem_post(&MUTEX_LISTA_BLOQUEADO);

				//tarea* tarea = pedir_tarea(tripulante->socket_miram, tripulante);
				free(tripulante->tarea_posta->tarea);
				free(tripulante->tarea_posta);
				tripulante->tarea_posta = pedir_tarea(tripulante->socket_miram, tripulante);

				//if(tarea != NULL){ // tiene mas tareas
				if(tripulante->tarea_posta != NULL){
					tripulante->estado = 'R';
					cambiar_estado(tripulante->socket_miram, tripulante, 'R');
					sem_wait(&MUTEX_LISTA_READY);
					list_add(lista_tripulantes_ready, tripulante);
					sem_post(&MUTEX_LISTA_READY);

					sem_post(&LISTA_READY_NO_VACIA);
				} else{ // no tiene mas tareas
					tripulante->estado = 'X';

					sem_wait(&MUTEX_LISTA_EXIT);
					list_add(lista_tripulantes_exit, tripulante);
					sem_post(&MUTEX_LISTA_EXIT);

					sem_post(&(tripulante->semaforo_tripulante));
				}
				//free(tarea);
			}
		}
	}
	log_info(logger, "fin hilo bloq");
	//liberar_memoria_tripu(tripulante);
	//sem_post(&FINALIZA_HILOS);
}


int main(int argc, char* argv[]) {
	//config_struct configuracion;

	//Reinicio el anterior y arranco uno nuevo
	FILE* archivo = fopen("discordiador.log","w");
	fclose(archivo);
	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);

	leer_config();

	conexionMiRam = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	conexionMongoStore = crear_conexion(configuracion.ip_mongostore, configuracion.puerto_mongostore);

	conexionMongoSabotaje = crear_conexion(configuracion.ip_mongostore,"5003");

	pthread_t hilo_principal;
	pthread_t hilo_sabotaje;
	pthread_create(&hilo_principal,NULL,(void*)menu_discordiador,NULL);
	pthread_detach(hilo_principal);
	pthread_create(&hilo_sabotaje,NULL,(void*)funcion_sabotaje,NULL);
	pthread_detach(hilo_sabotaje);

	sem_init(&TERMINO, 0,0);

	sem_wait(&TERMINO);

	return 0;
}

void funcion_sabotaje(){
	log_info(logger, "in funcion_sabotaje");

	tcbTripulante* tripulante;

	while(1){
		char* posicion_sabotaje_char = recibir_mensaje(conexionMongoSabotaje);
		log_info(logger, "posicion_sabotaje_char %s", posicion_sabotaje_char);
		sem_wait(&CONTINUAR_PLANIFICACION);

		char* posicion_sabotaje_char_2 = malloc(strlen(posicion_sabotaje_char)+1);
		strcpy(posicion_sabotaje_char_2, posicion_sabotaje_char);

		int posx = atoi(strtok(posicion_sabotaje_char,"|"));
		int posy = atoi(strtok(NULL,""));

		flag_sabotaje = configuracion.grado_multitarea;

		if(list_size(lista_tripulantes_trabajando) > 0){
			//lista_bloq_emergencia = list_take_and_remove(lista_tripulantes_trabajando, list_size(lista_tripulantes_trabajando));
			for(int i=0; i < list_size(lista_tripulantes_trabajando); i++){
				tripulante = (tcbTripulante*)list_remove(lista_tripulantes_trabajando, i);
				list_add(lista_bloq_emergencia,tripulante);
				i = -1;
			}
		}

		bool tid_anterior(tcbTripulante* tid_menor, tcbTripulante* tid_mayor) {
			return tid_menor->tid < tid_mayor->tid;
		}

		list_sort(lista_bloq_emergencia, (void*) tid_anterior);

		for(int i=0; i < list_size(lista_tripulantes_ready); i++){
			tripulante = (tcbTripulante*)list_remove(lista_tripulantes_ready, i);
			list_add(lista_aux,tripulante);
			i = -1;
		}

		list_sort(lista_aux, (void*) tid_anterior);

		list_add_all(lista_bloq_emergencia, lista_aux);
		list_clean(lista_aux);


		if(list_size(lista_bloq_emergencia) > 0){
			log_info(logger, "lista bloq emergencia");
			for(int i=0; i < list_size(lista_bloq_emergencia); i++){
				tripulante = (tcbTripulante*)list_get(lista_bloq_emergencia, i);
				log_info(logger, "tid %d  posx %d  posy %d", tripulante->tid, tripulante->posicionX, tripulante->posicionY);
				cambiar_estado(tripulante->socket_miram, tripulante, 'B');
			}

			tcbTripulante* tripu1;
			tripu1 = (tcbTripulante*)list_get(lista_bloq_emergencia, 0);
			tcbTripulante* tripu2;
			int dif_x = abs(tripu1->posicionX - posx);
			int dif_y = abs(tripu1->posicionY - posy);

			for(int i=1; i < list_size(lista_bloq_emergencia); i++){
				tripu2 = (tcbTripulante*)list_get(lista_bloq_emergencia, i);

				int dif_x_2 = abs(tripu2->posicionX - posx);
				int dif_y_2 = abs(tripu2->posicionY - posy);

				if( dif_x + dif_y >= dif_x_2 + dif_y_2 ){ //esta mas cerca el segundo
					dif_x = dif_x_2;
					dif_y = dif_y_2;
					tripu1 = tripu2;
				}

			}
			log_info(logger, "Tripu elegido: tid %d  posx %d  posy %d", tripu1->tid, tripu1->posicionX, tripu1->posicionY);
			informar_atencion_sabotaje(tripu1, posicion_sabotaje_char_2);

			remover_tripulante_de_lista(tripu1, lista_bloq_emergencia);
			list_add(lista_bloq_emergencia, tripu1);
			log_info(logger, "lista bloq emergencia actualizada");
			for(int i=0; i < list_size(lista_bloq_emergencia); i++){
				tripulante = (tcbTripulante*)list_get(lista_bloq_emergencia, i);
				log_info(logger, "tid %d  posx %d  posy %d", tripulante->tid, tripulante->posicionX, tripulante->posicionY);
			}

			//mover al tripu1 que es el mas cercano
			while(tripu1->posicionX != posx){ //muevo en X
				int x_viejo = tripulante->posicionX;
				if(tripu1->posicionX > posx){ //mayor a la posicion del sabotaje -> se mueve para la izq
					tripu1->posicionX--;
				} else{
					tripu1->posicionX++;
				}
				informar_movimiento(tripu1->socket_miram, tripu1);
				informar_movimiento_mongo_X (tripu1, x_viejo);
				sleep(configuracion.retardo_cpu);
			}

			while(tripu1->posicionY != posy){ //muevo en Y
				int y_viejo = tripulante->posicionY;
				if(tripu1->posicionY > posy){ //mayor a la posicion del sabotaje -> se mueve para abajo
					tripu1->posicionY--;
				} else{
					tripu1->posicionY++;
				}
				informar_movimiento(tripu1->socket_miram, tripu1);
				informar_movimiento_mongo_Y (tripu1, y_viejo);
				sleep(configuracion.retardo_cpu);
			}

			log_info(logger, "comienza tiempo sabotaje");
			sleep(configuracion.duracion_sabotaje);
			log_info(logger, "finaliza tiempo sabotaje");
			informar_sabotaje_resuelto(tripu1);

			enviar_header(FSCK, tripu1->socket_mongo);

			char* mensaje_recibido = recibir_mensaje(tripu1->socket_mongo);
			free(mensaje_recibido);

			if(configuracion.grado_multitarea > list_size(lista_bloq_emergencia)){
				flag_sabotaje = list_size(lista_bloq_emergencia);
			}

			//lista_tripulantes_ready = list_take_and_remove(lista_bloq_emergencia, list_size(lista_bloq_emergencia));
			for(int i=0; i < list_size(lista_bloq_emergencia); i++){
				tripulante = (tcbTripulante*)list_remove(lista_bloq_emergencia, i);
				list_add(lista_tripulantes_ready,tripulante);
				i = -1;
			}

			for(int i=0; i < list_size(lista_tripulantes_ready); i++){
				tripulante = (tcbTripulante*)list_get(lista_tripulantes_ready, i);
				cambiar_estado(tripulante->socket_miram, tripulante, 'R');
				sem_post(&LISTA_READY_NO_VACIA);
			}

			free(posicion_sabotaje_char_2);
		}else{
			log_info(logger, "no hay tripulantes disponibles para resolver sabotaje");
			flag_sabotaje = 0;
		}

		free(posicion_sabotaje_char);
		sem_post(&CONTINUAR_PLANIFICACION);
	}
}

void menu_discordiador() {
	log_info(logger, "in menu_discordiador");

	sem_init(&HABILITA_EJECUTAR, 0,1);
	sem_init(&NUEVO_READY, 0,0);
	sem_init(&AGREGAR_NUEVO_A_READY, 0,0);
	sem_init(&HABILITA_GRADO_MULTITAREA, 0,configuracion.grado_multitarea);
	sem_init(&PASA_A_BLOQUEADO, 0,0);
	sem_init(&MUTEX_LISTA_READY, 0,1);
	sem_init(&MUTEX_LISTA_TRABAJANDO, 0,1);
	sem_init(&MUTEX_LISTA_BLOQUEADO, 0,1);
	sem_init(&MUTEX_LISTA_EXIT, 0,1);
	sem_init(&CONTINUAR_PLANIFICACION, 0,0);
	sem_init(&ORDENA_FUNCION_QUANTUM, 0,1);
	sem_init(&FINALIZA_HILOS, 0,0);
	sem_init(&LISTA_READY_NO_VACIA, 0,0);

	lista_tripulantes_nuevo = list_create();
	lista_tripulantes_ready = list_create();
	lista_tripulantes_bloqueado = list_create();
	lista_tripulantes_trabajando = list_create();
	lista_tripulantes_exit = list_create();
	lista_bloq_emergencia = list_create();
	lista_aux = list_create();

	bool pausar_planificacion_activado = true;
	uint32_t numero_patota = 0;

	pthread_t ready;
	pthread_t nuevo;
	pthread_t bloqueado;
	pthread_create(&ready,NULL,(void*)ready_exec,NULL);
	pthread_detach(ready);
	pthread_create(&nuevo,NULL,(void*)nuevo_ready,NULL);
	pthread_detach(nuevo);
	pthread_create(&bloqueado,NULL,(void*)bloqueado_ready,NULL);
	pthread_detach(bloqueado);

	uint32_t tid =1;
	uint32_t posx = 0;
	uint32_t posy = 0;
	flag_sabotaje = 0;
	flag_fin = 0;
	tcbTripulante* tripulante;

	while(1){
		t_paquete* paquete;
		//char* nombreHilo = "";
		char* leido = readline("Iniciar consola: ");
		printf("\n");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:
				paquete = crear_paquete(INICIAR_PATOTA);
				numero_patota++;

				char* numero_patota_char = malloc(sizeof(char)+1);
				sprintf(numero_patota_char, "%d", numero_patota);
				agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);

				char** parametros = string_split(leido," ");
				int cantidad_tripulantes  = atoi(parametros[1]);

				char* cantidad_tripulantes_char = malloc(sizeof(char)+1);
				sprintf(cantidad_tripulantes_char, "%d", cantidad_tripulantes);
				agregar_a_paquete(paquete, cantidad_tripulantes_char, strlen(cantidad_tripulantes_char)+1);

				//char* tareas = malloc(sizeof(char)+1);
				char* tareas = leer_tareas_archivo(parametros[2]);

				bool hay_mas_parametros = true;
				for(int i = 0; i < cantidad_tripulantes ; i++){
					if (hay_mas_parametros == true){
						if (parametros[i+3] == NULL){
							hay_mas_parametros = false;
						} else {
							char* posiciones = parametros[i+3];
							char nuevo [3];
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
					tripulante = crear_tripulante(tid,'N',posx,posy,numero_patota, cantidad_tripulantes);
					agregar_a_paquete(paquete, tripulante, tamanio_TCB);
					posx =0;
					posy =0;
					tid++;
					liberar_memoria_tripu(tripulante);
				}

				char* largo_tarea = malloc((3*sizeof(char))+1);
				sprintf(largo_tarea, "%d", strlen(tareas)+1);
				agregar_a_paquete(paquete, largo_tarea, strlen(largo_tarea)+1);
				agregar_a_paquete(paquete, tareas, strlen(tareas)+1);

				enviar_paquete(paquete, conexionMiRam);


				//char* mensaje_recibido = malloc(17);
				char* mensaje_recibido = recibir_mensaje(conexionMiRam);
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

						tripulante = crear_tripulante(tid,'N',posx,posy,numero_patota, cantidad_tripulantes);
						pthread_t nombreHilo = (char*)(tid);
						pthread_create(&nombreHilo,NULL,(void*)tripulante_hilo,tripulante);
						pthread_detach(nombreHilo);
						list_add(lista_tripulantes_nuevo, tripulante);
						sem_post(&AGREGAR_NUEVO_A_READY);

						posx = 0;
						posy = 0;
						tid++;
					}
				} else {
					log_warning(logger, "Memoria no asignada ==> no empieza");
				}

				eliminar_paquete(paquete);
				limpiar_array(parametros);
				free(numero_patota_char);
				free(cantidad_tripulantes_char);
				free(tareas);
				free(largo_tarea);
				free(mensaje_recibido);

				break;

			case LISTAR_TRIPULANTES:
				enviar_header(LISTAR_TRIPULANTES, conexionMiRam);
				int tipo_mensaje = recibir_operacion(conexionMiRam);
				if(tipo_mensaje == 11){  //NO_HAY_NADA_PARA_LISTAR
					log_info(logger, "No hay tripulantes en memoria para listar");

				}else{
					t_list* lista_tripulantes = recibir_paquete(conexionMiRam);

					for(int i=0; i < list_size(lista_tripulantes); i+=2){
						tripulante = (tcbTripulante*)list_get(lista_tripulantes, i);
						int numero_patota = (int)atoi(list_get(lista_tripulantes,i+1));
						switch(tripulante->estado){
							case 'N':
								printf("Tripulante: %d     Patota: %d     Status: NEW \n", tripulante->tid, numero_patota);
								break;
							case 'R':
								printf("Tripulante: %d     Patota: %d     Status: READY \n", tripulante->tid, numero_patota);
								break;
							case 'E':
								printf("Tripulante: %d     Patota: %d     Status: EXEC \n", tripulante->tid, numero_patota);
								break;
							case 'B':
								printf("Tripulante: %d     Patota: %d     Status: BLOCK I/O \n", tripulante->tid, numero_patota);
								break;
						}
					}

					void destruir_tripu(tcbTripulante* tripu){
						//free(tripu->tarea_posta);
						free(tripu);
					}
					list_destroy_and_destroy_elements(lista_tripulantes, (void*)destruir_tripu);
				}

				break;

			case INICIAR_PLANIFICACION:
				if (pausar_planificacion_activado == true){
					sem_post(&CONTINUAR_PLANIFICACION);
					pausar_planificacion_activado = false;
				}

				break;

			case PAUSAR_PLANIFICACION:
				flag_sabotaje = 0;
				if (pausar_planificacion_activado == false){ //PARA QUE NO PUEDA HACER DOS PAUSAR_PLANIFICACION SEGUIDOS SIN HACER UN INICIAR_PLANIFICACION
					sem_wait(&CONTINUAR_PLANIFICACION);
					pausar_planificacion_activado = true;
				}

				break;
/*
INICIAR_PATOTA 5 tareas.txt 3|4 1|2 4|5
INICIAR_PATOTA 5 tareas_corta.txt 3|4 9|2 4|5
INICIAR_PATOTA 3 tareas_corta.txt 3|4 5|2 4|5
INICIAR_PLANIFICACION
----- prueba segmentacion:
INICIAR_PATOTA 4 SEG_PatotaA.txt
INICIAR_PATOTA 1 SEG_PatotaC.txt
INICIAR_PLANIFICACION
LISTAR_TRIPULANTES
DUMP
 esperar 10 ciclos de CPU y ejecutar los siguientes comandos.
EXPULSAR_TRIPULANTE 1 1
EXPULSAR_TRIPULANTE 3 1
LISTAR_TRIPULANTES
INICIAR_PATOTA 2 SEG_PatotaB.txt
LISTAR_TRIPULANTES
DUMP
Esperar el fin de las tareas de los tripulantes.
DUMP
compactar
DUMP
*/
/*
INICIAR_PATOTA 3 ES3_Patota1.txt 9|9 0|0 5|5
INICIAR_PATOTA 3 ES3_Patota2.txt 4|0 2|6 8|2
INICIAR_PATOTA 3 ES3_Patota3.txt 2|3 5|8 5|3
INICIAR_PATOTA 3 ES3_Patota4.txt 0|9 4|4 9|0
INICIAR_PATOTA 3 ES3_Patota5.txt 0|2 9|6 3|5
INICIAR_PLANIFICACION
LISTAR_TRIPULANTES
 */
			case OBTENER_BITACORA:
				paquete = crear_paquete(OBTENER_BITACORA);
				char** array_parametro = string_split(leido," ");
				char* tripu_id = array_parametro[1];

				agregar_a_paquete(paquete, tripu_id, strlen(tripu_id)+1);
				enviar_paquete(paquete, conexionMongoStore);

				char* message = recibir_mensaje(conexionMongoStore);
				log_info(logger, "bitacora del tripulante: %s\n%s", tripu_id, message);

				eliminar_paquete(paquete);
				limpiar_array(array_parametro);
				free(message);

				break;

			case EXPULSAR_TRIPULANTE:
				paquete = crear_paquete(EXPULSAR_TRIPULANTE);
				char** parametro = string_split(leido," ");
				char* tripulante_id = parametro[1];
				int int_tid = atoi(parametro[1]);
				char* pid_char = parametro[2];
				bool encontre_tripu = false;

				//primero expulso de discordiador:

				if(pausar_planificacion_activado == false){
					sem_wait(&CONTINUAR_PLANIFICACION);
				}

				for(int i=0; i < list_size(lista_tripulantes_ready); i++){
					tripulante = (tcbTripulante*)list_get(lista_tripulantes_ready, i);
					if(tripulante->tid == int_tid){
						list_remove(lista_tripulantes_ready, i);
						tripulante->fui_expulsado = true;
						encontre_tripu = true;
						i = list_size(lista_tripulantes_ready); //para cortar el for
					}
				}

				for(int i=0; i < list_size(lista_tripulantes_trabajando) && encontre_tripu == false; i++){
					tripulante = (tcbTripulante*)list_get(lista_tripulantes_trabajando, i);
					if(tripulante->tid == int_tid){
						list_remove(lista_tripulantes_trabajando, i);
						tripulante->fui_expulsado = true;
						encontre_tripu = true;
						i = list_size(lista_tripulantes_trabajando); //para cprtar el for
					}
				}

				for(int i=0; i < list_size(lista_tripulantes_bloqueado) && encontre_tripu == false; i++){
					tripulante = (tcbTripulante*)list_get(lista_tripulantes_bloqueado, i);
					if(tripulante->tid == int_tid){
						list_remove(lista_tripulantes_bloqueado, i);
						tripulante->fui_expulsado = true;
						encontre_tripu = true;
						i = list_size(lista_tripulantes_bloqueado); //para cprtar el for
					}
				}

				if(encontre_tripu == false){
					log_info(logger, "no se encontro a ese tripulante para expulsar");
				}else{
					log_info(logger, "tripu expulsado: %d ", tripulante->tid);
				}

				list_add(lista_tripulantes_exit, tripulante);

				if(pausar_planificacion_activado == false){
					sem_post(&CONTINUAR_PLANIFICACION);
				}

				if(tripulante->estado == 'R'){
					close(tripulante->socket_miram);
					free(tripulante->tarea_posta->tarea);
					free(tripulante->tarea_posta);

					int tripu_misma_patota_exit = 0;
					sem_wait(&MUTEX_LISTA_EXIT);
					for(int i=0; i < list_size(lista_tripulantes_exit); i++){
						tcbTripulante* tripu = (tcbTripulante*)list_get(lista_tripulantes_exit, i);
						if(tripu->puntero_pcb == tripulante->puntero_pcb){
							tripu_misma_patota_exit++;
						}
					}
					sem_post(&MUTEX_LISTA_EXIT);

					log_info(logger, "tripu_misma_patota_exit %d", tripu_misma_patota_exit);

					if(tripulante->cant_tripus_patota == tripu_misma_patota_exit){ //o sea ya estan en exit todos los tripus de esta misma patota
						sem_wait(&MUTEX_LISTA_EXIT);
						for(int i=0; i < list_size(lista_tripulantes_exit); i++){
							tcbTripulante* tripu = (tcbTripulante*)list_get(lista_tripulantes_exit, i);
							if(tripu->puntero_pcb == tripulante->puntero_pcb){
								list_remove(lista_tripulantes_exit, i);
								//free(tripu->tarea_posta);
								free(tripu);
								i--;
							}
						}
						log_info(logger, "size exit %d", list_size(lista_tripulantes_exit));
						sem_post(&MUTEX_LISTA_EXIT);
					}
				}

				//luego expulso de la memoria miram
				agregar_a_paquete(paquete, tripulante_id, strlen(tripulante_id)+1);
				agregar_a_paquete(paquete, pid_char, strlen(pid_char)+1);
				enviar_paquete(paquete, conexionMiRam);
				eliminar_paquete(paquete);
				limpiar_array(parametro);

				break;
/*
INICIAR_PATOTA 5 tareas.txt 3|4 1|2 4|5
INICIAR_PATOTA 5 tareas_corta.txt 3|4 9|2 4|5
INICIAR_PATOTA 3 tareas_corta.txt 2|2 2|2 4|5
INICIAR_PLANIFICACION
*/
			case FIN:
				paquete = crear_paquete(FIN);
				enviar_paquete(paquete, conexionMiRam);
				enviar_paquete(paquete, conexionMongoStore);
				eliminar_paquete(paquete);
				//enviar_header(SALIR_MONGO, conexionMongoStore);
				flag_fin = 1;   //VER SI HACE FALTA TODO ESTO. LO HAGO POR LO DE LA MEMORIA PERO CAPAZ NI HACE FALTA.
				/*sem_post(&HABILITA_GRADO_MULTITAREA);
				sem_post(&HABILITA_EJECUTAR);
				sem_post(&AGREGAR_NUEVO_A_READY);
				sem_post(&NUEVO_READY);
				sem_post(&PASA_A_BLOQUEADO);
				sem_wait(&FINALIZA_HILOS);
				sem_wait(&FINALIZA_HILOS);
				sem_wait(&FINALIZA_HILOS);*/

				void destruir_tripu(tcbTripulante* tripu){
					free(tripu->tarea_posta);
					free(tripu);
				}
				list_destroy_and_destroy_elements(lista_tripulantes_bloqueado, (void*)destruir_tripu);
				list_destroy_and_destroy_elements(lista_tripulantes_ready, (void*)destruir_tripu);
				list_destroy_and_destroy_elements(lista_tripulantes_trabajando, (void*)destruir_tripu);
				list_destroy_and_destroy_elements(lista_tripulantes_exit, (void*)destruir_tripu);
				list_destroy_and_destroy_elements(lista_tripulantes_nuevo, (void*)destruir_tripu);
				list_destroy_and_destroy_elements(lista_bloq_emergencia, (void*)destruir_tripu);
				list_destroy_and_destroy_elements(lista_aux, (void*)destruir_tripu);


				pthread_cancel(nuevo);
				pthread_cancel(ready);
				pthread_cancel(bloqueado);
				free(leido);
				//free(tripulante);
				config_destroy(archConfig);
				log_info(logger, "aaaaaaaa");
				terminar_discordiador(conexionMiRam, conexionMongoStore, logger);
				//return EXIT_FAILURE;
				sem_post(&TERMINO);
				break;

			default:
				mensajeError(logger);
				break;
		}
		free(leido);

	}
}

void leer_config(void){
	archConfig = config_create("discordiador.config");

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
