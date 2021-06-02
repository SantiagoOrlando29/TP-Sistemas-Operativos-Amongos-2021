#include "discordiador.h"
#include <semaphore.h>




config_discordiador configuracion;
//config_struct configuracion;
sem_t ESPERA_AGREGAR_NUEVO_A_READY;
sem_t AGREGAR_NUEVO_A_READY;
sem_t INICIAR_TRIPULANTE;
sem_t NUEVO_READY;
sem_t HABILITA_EJECUTAR;
sem_t HABILITA_DOS;
sem_t PASA_A_BLOQUEADO;
sem_t MUTEX_LISTA_READY;
sem_t MUTEX_LISTA_TRABAJANDO;
sem_t MUTEX_LISTA_BLOQUEADO;
int id_tripulante = 0;
t_list* lista_tripulantes_nuevo;
t_list* lista_tripulantes_ready;
t_list* lista_tripulantes_bloqueado;
t_list* lista_tripulantes_trabajando;
t_list* lista_tripulantes_exit;


tarea* pedir_tarea(tcbTripulante* tripulante){
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
		printf("hilo %d P%d, FIN DE Q \n", tripulante->tid, tripulante->puntero_pcb);

		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		list_remove(lista_tripulantes_trabajando, 0);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'R';

		sem_wait(&MUTEX_LISTA_READY);
		list_add(lista_tripulantes_ready, tripulante);
		sem_post(&MUTEX_LISTA_READY);

		sem_post(&HABILITA_DOS);
		sem_post(&HABILITA_EJECUTAR);
		sem_wait(&(tripulante->semaforo_tripulante));
		*quantums_ejecutados = 0;
	}
}

void tripulante_hilo (tcbTripulante* tripulante){
	sem_wait(&(tripulante->semaforo_tripulante));
	//si funcion tomar tarea != null entonces
	tarea* tarea_recibida = crear_tarea(GENERAR_OXIGENO,5,2,2,4); //TAREA NORMAL

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
			printf("hilo %d P%d, me estoy moviendo \n", tripulante->tid, tripulante->puntero_pcb);
			sleep(1);
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

			sem_wait(&MUTEX_LISTA_TRABAJANDO);
			list_remove(lista_tripulantes_trabajando, 0);
			sem_post(&MUTEX_LISTA_TRABAJANDO);

			tripulante->estado = 'B';

			sem_wait(&MUTEX_LISTA_BLOQUEADO);
			list_add(lista_tripulantes_bloqueado, tripulante);
			sem_post(&MUTEX_LISTA_BLOQUEADO);

			printf("hilo %d P%d, me bloquie \n", tripulante->tid, tripulante->puntero_pcb);
			sem_post(&PASA_A_BLOQUEADO);
			sem_post(&HABILITA_DOS);
			sem_wait(&(tripulante->semaforo_tripulante));
			if(tripulante->estado != 'X'){ // O SEA Q TIENE MAS TAREAS
				tarea_recibida = pedir_tarea_normal(tripulante); //esto en realidad no iria, pq se la tiene q pasar el bloq o algo asi
				sem_post(&HABILITA_EJECUTAR);
			} else{ // NO TIENE MAS TAREAS
				tripulante->prox_instruccion = 100;
			}
		} else { //TAREA NORMAL
			while(contador_ciclos_trabajando < tarea_recibida->tiempo){
				printf("hilo %d P%d, estoy trabajando \n", tripulante->tid, tripulante->puntero_pcb);
				sleep(1);
				contador_ciclos_trabajando++;
				quantums_ejecutados++;
				if(contador_ciclos_trabajando == tarea_recibida->tiempo){//ESTE IF ESTA PARA QUE SI  YA TERMINO LA TAREA, SE FIJE SI TIENE OTRA MAS
					tarea_recibida = pedir_tarea(tripulante); // I/O     // Y SI NO LA TIENE QUE VAYA DIRECTO A EXIT SIN BLOQUEARSE POR FIN DE QUANTUM
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
		sem_post(&HABILITA_DOS);

		sem_wait(&MUTEX_LISTA_TRABAJANDO);
		list_remove(lista_tripulantes_trabajando, 0);
		sem_post(&MUTEX_LISTA_TRABAJANDO);

		tripulante->estado = 'X';
		list_add(lista_tripulantes_exit, tripulante);
	}
	printf("hilo %d P%d, EXIT\n", tripulante->tid, tripulante->puntero_pcb);

}

void ready_exec() {
	//sem_wait(&INICIAR_TRIPULANTE);
	tcbTripulante* tripulante1 = malloc(sizeof(tcbTripulante));
	int lista_size;
	while(1){
		lista_size = list_size(lista_tripulantes_ready);
		if (lista_size >0){
			sem_wait(&HABILITA_DOS);
			sem_wait(&HABILITA_EJECUTAR);
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
		tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_nuevo, 0);
		sem_post(&(tripulante1->semaforo_tripulante));
		sem_wait(&NUEVO_READY);
		tripulante1->estado = 'R';

		sem_wait(&MUTEX_LISTA_READY);
		list_add(lista_tripulantes_ready, tripulante1);
		sem_post(&MUTEX_LISTA_READY);

		sem_post(&ESPERA_AGREGAR_NUEVO_A_READY);
	}
	free(tripulante1);

}

void bloqueado_ready() {
	tcbTripulante* tripulante1 = malloc(sizeof(tcbTripulante));
	tarea* tarea_recibida;
	while(1){
		sem_wait(&PASA_A_BLOQUEADO);

		sem_wait(&MUTEX_LISTA_BLOQUEADO);
		tripulante1 = (tcbTripulante*)list_get(lista_tripulantes_bloqueado, 0);
		sem_post(&MUTEX_LISTA_BLOQUEADO);

		printf("hilo %d P%d, haciendo I/O \n", tripulante1->tid, tripulante1->puntero_pcb);
		sleep(1);
		printf("hilo %d P%d, termino I/O \n", tripulante1->tid, tripulante1->puntero_pcb);
		fflush(stdout);
		tarea_recibida = pedir_tarea_normal(tripulante1);

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
	t_log* logger;


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

	 /*Hacer una funcion que cree las diferetnes listas*/
	sem_init(&INICIAR_TRIPULANTE, 0,0);
	sem_init(&HABILITA_EJECUTAR, 0,1);
	sem_init(&NUEVO_READY, 0,0);
	sem_init(&AGREGAR_NUEVO_A_READY, 0,0);
	sem_init(&ESPERA_AGREGAR_NUEVO_A_READY, 0,1);
	sem_init(&HABILITA_DOS, 0,2);
	sem_init(&PASA_A_BLOQUEADO, 0,0);
	sem_init(&MUTEX_LISTA_READY, 0,1);
	sem_init(&MUTEX_LISTA_TRABAJANDO, 0,1);
	sem_init(&MUTEX_LISTA_BLOQUEADO, 0,1);


	lista_tripulantes_nuevo = list_create();
	lista_tripulantes_ready = list_create();
	lista_tripulantes_bloqueado = list_create();
	lista_tripulantes_trabajando = list_create();
	lista_tripulantes_exit = list_create();
	int tipoMensaje = -1;
	int cantidad_tripulantes;
	int patota = 0;

	pthread_t ready;
	pthread_t nuevo;
	pthread_t bloqueado;
	pthread_create(&ready,NULL,(void*)ready_exec,NULL);
	pthread_detach(&ready);
	pthread_create(&nuevo,NULL,(void*)nuevo_ready,NULL);
	pthread_detach(&nuevo);
	pthread_create(&bloqueado,NULL,(void*)bloqueado_ready,NULL);
	pthread_detach(&bloqueado);


	while(1){
		t_paquete* paquete;
		tcbTripulante* tripulante = malloc(sizeof(tcbTripulante));
		//char* nombreHilo = "";
		char* leido = readline("Iniciar consola: ");
		printf("\n");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:
				enviar_header(INICIAR_PATOTA, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				//lista_tripulantes_ready=recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);
				cantidad_tripulantes = 0;
				patota++;
				while(cantidad_tripulantes < 5){
					cantidad_tripulantes ++;
					tcbTripulante* tripulante =crear_tripulante(cantidad_tripulantes,'N',5,6,1,patota);
					pthread_t nombreHilo = (char*)(cantidad_tripulantes);
					pthread_create(&nombreHilo,NULL,(void*)tripulante_hilo,tripulante);
					pthread_detach(&nombreHilo);
					sem_wait(&ESPERA_AGREGAR_NUEVO_A_READY);
					list_add(lista_tripulantes_nuevo, tripulante);
					sem_post(&AGREGAR_NUEVO_A_READY);
				}
				break;

			case LISTAR_TRIPULANTES:
				/*enviar_header(LISTAR_TRIPULANTES, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);*/
				//int list_size(t_list *);
				//void *list_get(t_list *, int index);
				sem_wait(&MUTEX_LISTA_READY);
				for(int i=0; i < list_size(lista_tripulantes_ready); i++){
					tripulante = (tcbTripulante*)list_get(lista_tripulantes_ready, i);
					printf("Tripulante: %d   Patota: %d   Status: %c\n", tripulante->tid, tripulante->puntero_pcb, tripulante->estado);
				}
				sem_post(&MUTEX_LISTA_READY);

				sem_wait(&MUTEX_LISTA_TRABAJANDO);
				for(int i=0; i < list_size(lista_tripulantes_trabajando); i++){
					tripulante = (tcbTripulante*)list_get(lista_tripulantes_trabajando, i);
					printf("Tripulante: %d   Patota: %d   Status: %c\n", tripulante->tid, tripulante->puntero_pcb, tripulante->estado);
				}
				sem_post(&MUTEX_LISTA_TRABAJANDO);

				sem_wait(&MUTEX_LISTA_BLOQUEADO);
				for(int i=0; i < list_size(lista_tripulantes_bloqueado); i++){
					tripulante = (tcbTripulante*)list_get(lista_tripulantes_bloqueado, i);
					printf("Tripulante: %d   Patota: %d   Status: %c\n", tripulante->tid, tripulante->puntero_pcb, tripulante->estado);
				}
				sem_post(&MUTEX_LISTA_BLOQUEADO);

				break;

			case INICIAR_PLANIFICACION:
				sem_post(&INICIAR_TRIPULANTE);

				break;

			case PAUSAR_PLANIFICACION:
				sem_wait(&INICIAR_TRIPULANTE);
				break;
			case OBTENER_BITACORA:
				enviar_header(OBTENER_BITACORA, conexionMongoStore);
				tipoMensaje = recibir_operacion(conexionMongoStore);
				t_list* lista = recibir_paquete(conexionMongoStore);
				char* mensaje = (char*)list_get(lista, 0);
				log_info(logger, mensaje);
				list_destroy(lista);
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
