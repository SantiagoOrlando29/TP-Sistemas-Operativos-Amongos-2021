#include "discordiador.h"
#include <semaphore.h>




config_discordiador configuracion;
//config_struct configuracion;
sem_t NUEVO_AGREGAR;
sem_t AGREGAR_NUEVO;
sem_t INICIAR_TRIPULANTE;
sem_t NUEVO_READY;
sem_t TRABAJAR_TRIPULANTE;
sem_t BLOQUEAR_TRIPULANTE;
sem_t READY_TRIPULANTE;
sem_t ESPERA_HILO;
sem_t HABILITA_EJECUTAR;
sem_t HABILITA_EXEC_SIG;
int id_tripulante = 0;
t_list* lista_tripulantes_nuevo;
t_list* lista_tripulantes_ready;
t_list* lista_tripulantes_bloqueado;
t_list* lista_tripulantes_trabajando;



void tripulante_hilo (tcbTripulante* tripulante){
	sem_wait(&(tripulante->semaforo_tripulante));
	//si funcion tomar tarea != null entonces
	printf("hola soy el hilo %d, estoy listo para ejecutar \n", tripulante->tid);
	sem_post(&NUEVO_READY);
	fflush(stdout);
	sem_wait(&(tripulante->semaforo_tripulante));
	int numero = 0;
	while(numero < 3){
		printf("hola soy el hilo %d, estoy trabajando \n", tripulante->tid);
		sleep(2);
		numero ++;
		fflush(stdout);
	}

	tripulante->estado = 'B';
	list_remove(lista_tripulantes_trabajando, 0);
	//list_add(lista_tripulantes_bloqueado, tripulante);
	sem_wait(&HABILITA_EXEC_SIG);
	sem_post(&HABILITA_EJECUTAR);
}

void iniciar_planificacion() {
	sem_wait(&INICIAR_TRIPULANTE);
	tcbTripulante* tripulante1 = malloc(sizeof(tcbTripulante));
	tcbTripulante* tripulante2 = malloc(sizeof(tcbTripulante));
	tcbTripulante* tripulante3 = malloc(sizeof(tcbTripulante));
	while(1){
		int lista_size = list_size(lista_tripulantes_ready);
		if (lista_size >0){
			tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_ready, 0);
			tripulante2 = (tcbTripulante*)list_remove(lista_tripulantes_ready, 0);
			tripulante1->estado = 'E';
			tripulante2->estado = 'E';
			list_add(lista_tripulantes_trabajando, tripulante1);
			list_add(lista_tripulantes_trabajando, tripulante2);
			sem_post(&(tripulante1->semaforo_tripulante));
			sem_post(&(tripulante2->semaforo_tripulante));
			while(list_size(lista_tripulantes_ready) >0){
				sem_wait(&HABILITA_EJECUTAR);
				tripulante3 = (tcbTripulante*)list_remove(lista_tripulantes_ready, 0);
				tripulante3->estado = 'E';
				list_add(lista_tripulantes_trabajando, tripulante3);
				sem_post(&(tripulante3->semaforo_tripulante));
				sem_post(&HABILITA_EXEC_SIG);
			}
			sem_post(&HABILITA_EXEC_SIG);
			sem_post(&HABILITA_EXEC_SIG);
			sem_wait(&HABILITA_EJECUTAR);
			sem_wait(&HABILITA_EJECUTAR);
		}
	}
	free(tripulante1);
	free(tripulante2);
	free(tripulante3);
}

void nuevo_ready() {
	tcbTripulante* tripulante1 = malloc(sizeof(tcbTripulante));
	while(1){
		sem_wait(&AGREGAR_NUEVO);
		tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_nuevo, 0);
		sem_post(&(tripulante1->semaforo_tripulante));
		sem_wait(&NUEVO_READY);
		list_add(lista_tripulantes_ready, tripulante1);
		sem_post(&NUEVO_AGREGAR);

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

	tcbTripulante* tripulante=crear_tripulante(1,'N',5,6,1,1);
	tarea* tarea_recibida1 = crear_tarea(GENERAR_OXIGENO,5,2,2,5);
	tarea* tarea_recibida2 = crear_tarea(GENERAR_COMIDA,7,2,2,5);
	tarea* tarea_recibida3 = crear_tarea(GENERAR_BASURA,9,2,2,5);
	hacer_tarea(tripulante,tarea_recibida1);
	hacer_tarea(tripulante,tarea_recibida2);
	hacer_tarea(tripulante,tarea_recibida3);


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
	sem_init(&TRABAJAR_TRIPULANTE, 0,0);
	sem_init(&BLOQUEAR_TRIPULANTE, 0,0);
	sem_init(&READY_TRIPULANTE, 0,0);
	sem_init(&ESPERA_HILO, 0,0);
	sem_init(&HABILITA_EJECUTAR, 0,0);
	sem_init (&NUEVO_READY, 0,0);
	sem_init (&AGREGAR_NUEVO, 0,0);
	sem_init (&NUEVO_AGREGAR, 0,1);
	sem_init (&HABILITA_EXEC_SIG, 0,1);


	lista_tripulantes_nuevo = list_create();
	lista_tripulantes_ready = list_create();
	lista_tripulantes_bloqueado = list_create();
	lista_tripulantes_trabajando = list_create();
	int tipoMensaje = -1;
	int cantidad_tripulantes;
	pthread_t lista_ready;
	pthread_t lista_nuevo;

	pthread_create(&lista_ready,NULL,(void*)iniciar_planificacion,NULL);
	pthread_detach(&lista_ready);
	pthread_create(&lista_nuevo,NULL,(void*)nuevo_ready,NULL);
	pthread_detach(&lista_nuevo);

	while(1){
		//tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
		t_paquete* paquete;
		char* nombreHilo = "";
		char* leido = readline("Iniciar consola: ");
		printf("\n");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:
				enviar_header(INICIAR_PATOTA, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				//lista_tripulantes_ready=recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);
				cantidad_tripulantes = 0;
				while(cantidad_tripulantes < 5){
					cantidad_tripulantes ++;
					tcbTripulante* tripulante =crear_tripulante(cantidad_tripulantes,'N',5,6,1,1);
					pthread_t nombreHilo = (char*)(cantidad_tripulantes);
					pthread_create(&nombreHilo,NULL,(void*)tripulante_hilo,tripulante);
					pthread_detach(&nombreHilo);
					sem_wait(&NUEVO_AGREGAR);
					list_add(lista_tripulantes_nuevo, tripulante);
					sem_post(&AGREGAR_NUEVO);

				}
				break;

			case LISTAR_TRIPULANTES:
				/*enviar_header(LISTAR_TRIPULANTES, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);*/
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
		pthread_mutex_destroy(&INICIAR_TRIPULANTE);

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
