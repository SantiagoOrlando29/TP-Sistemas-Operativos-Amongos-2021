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


tarea* pedir_tarea_IO(tcbTripulante* tripulante){
	tarea* tarea_recibida = crear_tarea(GENERAR_COMIDA,7,2,2,2);
	return(tarea_recibida);
}
tarea* pedir_tarea_normal(tcbTripulante* tripulante){
	if(tripulante->tid < 4){ // ESTO HACE Q VAYA A EXIT
		tarea* tarea_recibida = crear_tarea(GENERAR_OXIGENO,5,2,2,2);
		return(tarea_recibida);
	} else{
		return NULL;
	}
}

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
		sem_wait(&CONTINUAR_PLANIFICACION);
		sem_post(&CONTINUAR_PLANIFICACION);
		printf("hilo %d P%d, FIN DE Q \n", tripulante->tid, tripulante->puntero_pcb);

		sem_wait(&CONTINUAR_PLANIFICACION);
		sem_post(&CONTINUAR_PLANIFICACION);
		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		list_remove(lista_tripulantes_trabajando, 0);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'R';

		sem_wait(&MUTEX_LISTA_READY);
		list_add(lista_tripulantes_ready, tripulante);
		sem_post(&MUTEX_LISTA_READY);

		sem_post(&HABILITA_GRADO_MULTITAREA);
		sem_post(&HABILITA_EJECUTAR);
		sem_wait(&(tripulante->semaforo_tripulante));
		*quantums_ejecutados = 0;
	}
}

/*tarea* pedir_tarea(int conexion_miram, tcbTripulante* tripulante){
	t_paquete* paquete = crear_paquete(PEDIR_TAREA);
	agregar_a_paquete(paquete, tripulante, tamanio_TCB);
	enviar_paquete(paquete, conexion_miram);
}*/

void tripulante_hilo (tcbTripulante* tripulante){
	sem_wait(&(tripulante->semaforo_tripulante));

	tarea* tarea_recibida = crear_tarea(GENERAR_OXIGENO,5,2,2,4); //TAREA NORMAL

	int conexion_miram = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	t_paquete* paquete = crear_paquete(PEDIR_TAREA);
	agregar_a_paquete(paquete, tripulante, tamanio_TCB);
	char* numero_patota_char = malloc(sizeof(char));
	sprintf(numero_patota_char, "%d", tripulante->puntero_pcb);
	agregar_a_paquete(paquete, numero_patota_char, strlen(numero_patota_char)+1);
	enviar_paquete(paquete, conexion_miram);
	char* tarea_recibida_miram = recibir_mensaje(conexion_miram);
	log_info(logger, "tarea recibida: %s", tarea_recibida_miram);
	tripulante->prox_instruccion++;


	printf("hola soy el hilo %d, P%d, estoy listo para ejecutar \n", tripulante->tid, tripulante->puntero_pcb);
	sem_post(&NUEVO_READY);
	fflush(stdout);
	sem_wait(&(tripulante->semaforo_tripulante));
	sem_post(&HABILITA_EJECUTAR);
	int contador_movimientos = 0;
	int contador_ciclos_trabajando = 0;
	int quantums_ejecutados = 0;
	while(tripulante->prox_instruccion < 4){
		while(contador_movimientos < tarea_recibida->tiempo){
			sem_wait(&CONTINUAR_PLANIFICACION);
			sem_post(&CONTINUAR_PLANIFICACION);
			printf("hilo %d P%d, me estoy moviendo \n", tripulante->tid, tripulante->puntero_pcb);
			sleep(1); // retardo ciclo cpu VA?  EN TODOS LOS SLEEP IRIA ESO?
			fflush(stdout);
			contador_movimientos++;
			quantums_ejecutados++;
			termina_quantum(&quantums_ejecutados, tripulante);
		}
		contador_movimientos = 0;

		if(tarea_recibida->tarea == 2){ //TAREA DE I/O
			quantums_ejecutados++;
			termina_quantum(&quantums_ejecutados, tripulante);
			sleep(1); // retardo ciclo cpu //sleep para eso de iniciar tarea I/O (simula peticion al SO). Nose si va en esta linea
			quantums_ejecutados = 0;

			sem_wait(&CONTINUAR_PLANIFICACION);
			sem_post(&CONTINUAR_PLANIFICACION);
			sem_wait(&MUTEX_LISTA_TRABAJANDO);
			list_remove(lista_tripulantes_trabajando, 0);
			sem_post(&MUTEX_LISTA_TRABAJANDO);

			tripulante->estado = 'B';

			sem_wait(&MUTEX_LISTA_BLOQUEADO);
			list_add(lista_tripulantes_bloqueado, tripulante);
			sem_post(&MUTEX_LISTA_BLOQUEADO);

			printf("hilo %d P%d, me bloquie \n", tripulante->tid, tripulante->puntero_pcb);
			sem_post(&PASA_A_BLOQUEADO);
			sem_post(&HABILITA_GRADO_MULTITAREA);
			sem_wait(&(tripulante->semaforo_tripulante));
			if(tripulante->estado != 'X'){ // O SEA Q TIENE MAS TAREAS
				tarea_recibida = pedir_tarea_normal(tripulante); //esto en realidad no iria, pq se la tiene q pasar el bloq o algo asi
				sem_post(&HABILITA_EJECUTAR);
			} else{ // NO TIENE MAS TAREAS
				tripulante->prox_instruccion = 100;
			}
		} else { //TAREA NORMAL
			while(contador_ciclos_trabajando < tarea_recibida->tiempo){
				sem_wait(&CONTINUAR_PLANIFICACION);
				sem_post(&CONTINUAR_PLANIFICACION);
				printf("hilo %d P%d, estoy trabajando \n", tripulante->tid, tripulante->puntero_pcb);
				sleep(1);
				contador_ciclos_trabajando++;
				quantums_ejecutados++;
				if(contador_ciclos_trabajando == tarea_recibida->tiempo){//ESTE IF ESTA PARA QUE SI  YA TERMINO LA TAREA, SE FIJE SI TIENE OTRA MAS
					tarea_recibida = pedir_tarea_IO(tripulante); // I/O     // Y SI NO LA TIENE QUE VAYA DIRECTO A EXIT SIN BLOQUEARSE POR FIN DE QUANTUM
					if(tarea_recibida == NULL){ // NO TIENE MAS TAREAS
						tripulante->prox_instruccion = 200;
					}else{ //TIENE MAS TAREAS
						termina_quantum(&quantums_ejecutados, tripulante);
					}
				}else{
					termina_quantum(&quantums_ejecutados, tripulante);
				}
			}
			contador_ciclos_trabajando = 0;
		}

		tripulante->prox_instruccion++;
	}
	if(tripulante->estado != 'X'){
		sem_post(&HABILITA_GRADO_MULTITAREA);

		sem_wait(&CONTINUAR_PLANIFICACION);
		sem_post(&CONTINUAR_PLANIFICACION);
		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		list_remove(lista_tripulantes_trabajando, 0);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'X';
		list_add(lista_tripulantes_exit, tripulante);
	}
	printf("hilo %d P%d, EXIT\n", tripulante->tid, tripulante->puntero_pcb);
	close(conexion_miram);
}

void ready_exec() {
	//sem_wait(&INICIAR_TRIPULANTE);
	tcbTripulante* tripulante1 = malloc(tamanio_TCB);
	int lista_size;
	while(1){
		lista_size = list_size(lista_tripulantes_ready);
		if (lista_size >0){
			sem_wait(&HABILITA_GRADO_MULTITAREA);
			sem_wait(&HABILITA_EJECUTAR);
			sem_wait(&CONTINUAR_PLANIFICACION);
			sem_post(&CONTINUAR_PLANIFICACION);
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
	tcbTripulante* tripulante1 = malloc(tamanio_TCB);
	while(1){
		sem_wait(&AGREGAR_NUEVO_A_READY);
		sem_wait(&CONTINUAR_PLANIFICACION);
		sem_post(&CONTINUAR_PLANIFICACION);
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
	tcbTripulante* tripulante1 = malloc(tamanio_TCB);
	tarea* tarea_recibida;
	while(1){
		sem_wait(&PASA_A_BLOQUEADO);

		sem_wait(&MUTEX_LISTA_BLOQUEADO);
		tripulante1 = (tcbTripulante*)list_get(lista_tripulantes_bloqueado, 0);
		sem_post(&MUTEX_LISTA_BLOQUEADO);

		sem_wait(&CONTINUAR_PLANIFICACION);
		sem_post(&CONTINUAR_PLANIFICACION);
		printf("hilo %d P%d, haciendo I/O \n", tripulante1->tid, tripulante1->puntero_pcb);
		sleep(1);
		sem_wait(&CONTINUAR_PLANIFICACION);
		sem_post(&CONTINUAR_PLANIFICACION);
		printf("hilo %d P%d, termino I/O \n", tripulante1->tid, tripulante1->puntero_pcb);
		fflush(stdout);
		tarea_recibida = pedir_tarea_normal(tripulante1);

		sem_wait(&CONTINUAR_PLANIFICACION);
		sem_post(&CONTINUAR_PLANIFICACION);
		sem_wait(&MUTEX_LISTA_BLOQUEADO);
		tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_bloqueado, 0);
		sem_post(&MUTEX_LISTA_BLOQUEADO);

		if(tarea_recibida != NULL){ // TIENE MAS TAREAS
			tripulante1->estado = 'R';
			sem_wait(&MUTEX_LISTA_READY);
			list_add(lista_tripulantes_ready, tripulante1);
			sem_post(&MUTEX_LISTA_READY);
		} else{ // NO TIENE MAS TAREAS
			tripulante1->estado = 'X';
			list_add(lista_tripulantes_exit, tripulante1);
			sem_post(&(tripulante1->semaforo_tripulante));
		}

	}
	free(tripulante1);

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

				char* tareas = malloc(sizeof(char));
				leer_tareas(parametros[2], &tareas);
				printf("Las tareas recibidas por parametro son: %s\n", tareas);

/*
INICIAR_PATOTA 5 tareas.txt 300|4 10|20 4|500
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
					posx =0;
					posy =0;
					agregar_a_paquete(paquete, tripulante, tamanio_TCB);
					tid++;
				}

				char* largo_tarea = malloc(sizeof(char));
				sprintf(largo_tarea, "%d", strlen(tareas)+1);
				agregar_a_paquete(paquete, largo_tarea, strlen(largo_tarea)+1);
				agregar_a_paquete(paquete, tareas, strlen(tareas)+1);
				//agregar_a_paquete(paquete, "Hola Mundo", 11);
				free(tareas);

				enviar_paquete(paquete, conexionMiRam);


				//recibe confirmacion de miram de que esta todo bien
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
				//tipoMensaje = recibir_operacion(conexionMiRam);
				//recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);

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
