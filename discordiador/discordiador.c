#include "discordiador.h"
#include <semaphore.h>




config_discordiador configuracion;
//config_struct configuracion;

sem_t INICIAR_TRIPULANTE;
sem_t TRABAJAR_TRIPULANTE;
sem_t BLOQUEAR_TRIPULANTE;
sem_t READY_TRIPULANTE;
sem_t ESPERA_HILO;
sem_t HABILITA_EJECUTAR;
int id_tripulante = 0;
t_list* lista_tripulantes_ready;
t_list* lista_tripulantes_bloqueado;
t_list* lista_tripulantes_trabajando;



void esperar_cpu (tcbTripulante* tripulante){
	while(tripulante->estado != 'R'){

	}

	sem_wait(&(tripulante->semaforo_tripulante));

	int numero = 0;
	while(numero < 5){
		printf("hola soy el hilo %d, estoy esperando CPU \n", tripulante->tid);
		fflush(stdout);
		numero ++;
	}

	tripulante->estado = 'E';
	//list_remove(lista_tripulantes_ready, 0);
	//list_add(lista_tripulantes_trabajando, tripulante);
	sem_post(&(tripulante->semaforo_tripulante));
}


void trabajar (tcbTripulante* tripulante){
	//sem_post(&ESPERA_HILO);

	sem_wait(&(tripulante->semaforo_tripulante));

	int numero = 0;
	while(numero < 5){
		printf("hola soy el hilo %d, estoy trabajando \n", tripulante->tid);
		fflush(stdout);
		numero ++;
	}

	tripulante->estado = 'B';
	list_remove(lista_tripulantes_trabajando, 0);
	list_add(lista_tripulantes_bloqueado, tripulante);
	sem_post(&(tripulante->semaforo_tripulante));

	//int lista_size = list_size(lista_tripulantes_ready);
	//if (lista_size >0){
	/*tcbTripulante* tripulanteA = malloc(sizeof(tcbTripulante));
	tripulanteA = (tcbTripulante*)list_remove(lista_tripulantes_ready, 0);
	list_add(lista_tripulantes_trabajando, tripulanteA);
	sem_post(&(tripulanteA->semaforo_tripulante));*/
	//}
	//sem_post(&INICIAR_PLANI);
	sem_post(&HABILITA_EJECUTAR);
}

void trabajar_bloqueado (tcbTripulante* tripulante){
	while(tripulante->estado != 'B'){

	}

	sem_wait(&(tripulante->semaforo_tripulante));

	int numero = 0;
	while(numero < 5){
		printf("hola soy el hilo %d, estoy bloqueado \n", tripulante->tid);
		fflush(stdout);
		numero ++;
	}

	tripulante->estado = 'R';
	list_remove(lista_tripulantes_bloqueado, 0);
	//list_add(lista_tripulantes_ready, tripulante);
	sem_post(&(tripulante->semaforo_tripulante));
}


int main(int argc, char* argv[]) {
	//config_struct configuracion;
	t_log* logger;


	//Reinicio el anterior y arranco uno nuevo
	FILE* archivo = fopen("discordiador.log","w");
	fclose(archivo);
	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);



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

	//t_list* lista_tripulantes_ready = list_create();
	//t_list* lista_tripulantes_bloqueado = list_create();
	//t_list* lista_tripulantes_trabajando = list_create();
	lista_tripulantes_ready = list_create();
	lista_tripulantes_bloqueado = list_create();
	lista_tripulantes_trabajando = list_create();
	t_list* listaTripulantes;
	int tipoMensaje = -1;
	char* nombreThread = "";
	int cantidad_tripulantes;
	int id_prueba=0;
	tcbTripulante* tripulante1 = malloc(sizeof(tcbTripulante));
	tcbTripulante* tripulante2 = malloc(sizeof(tcbTripulante));
	while(1){

		tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);


		t_paquete* paquete;
		char* nombreHilo = "";
		char* leido = readline("");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:

				/*paquete = crear_paquete(INICIAR_PATOTA);
				char** parametros = string_split(leido, " ");
				log_info(logger, (char*)parametros[1]);
				agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));

				tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
				agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));
				enviar_paquete(paquete, conexionMiRam);
				eliminar_paquete(paquete);
				Solo para probar que funciona pero esto debe ser un paquete*/
				enviar_header(INICIAR_PATOTA, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				printf("%d", tipoMensaje);
				lista_tripulantes_ready=recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger, lista_tripulantes_ready);
				cantidad_tripulantes = 0;
				while(cantidad_tripulantes < 4){
					cantidad_tripulantes ++;
					tcbTripulante* tripulante=crear_tripulante(cantidad_tripulantes,'N',5,6,1,1);
					pthread_t nombreHilo = (char*)(cantidad_tripulantes);
					pthread_create(&nombreHilo,NULL,(void*)trabajar,tripulante);
					//sem_wait(&ESPERA_HILO);
					list_add(lista_tripulantes_ready, tripulante);
				}
				cantidad_tripulantes = 0;
				while(cantidad_tripulantes < 4){
					cantidad_tripulantes ++;
					pthread_detach(&nombreHilo);
				}
				break;

			case LISTAR_TRIPULANTES:
				/*enviar_header(LISTAR_TRIPULANTES, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);*/
				break;

			case INICIAR_PLANIFICACION:
				sem_post(&INICIAR_TRIPULANTE);

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
					tcbTripulante* tripulante3 = malloc(sizeof(tcbTripulante));
					while(list_size(lista_tripulantes_ready) >0){
						sem_wait(&HABILITA_EJECUTAR);
						tripulante3 = (tcbTripulante*)list_remove(lista_tripulantes_ready, 0);
						tripulante3->estado = 'E';
						list_add(lista_tripulantes_trabajando, tripulante3);
						sem_post(&(tripulante3->semaforo_tripulante));
					}
				//trabajar_bloqueado(tripulante1);
				//trabajar_bloqueado(tripulante2);

				}

					//sem_wait(&(tripulante1->semaforo_tripulante));
					//sem_wait(&(tripulante2->semaforo_tripulante));


					/*tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_trabajando, 0);
					tripulante2 = (tcbTripulante*)list_remove(lista_tripulantes_trabajando, 0);
					list_add(lista_tripulantes_bloqueado, tripulante1);
					list_add(lista_tripulantes_bloqueado, tripulante2);*/




				/*tripulante1 = (tcbTripulante*)list_remove(lista_tripulantes_bloqueado, 0);
				tripulante2 = (tcbTripulante*)list_remove(lista_tripulantes_bloqueado, 0);
				list_add(lista_tripulantes_ready, tripulante1);
				list_add(lista_tripulantes_ready, tripulante2);*/
				//esperar_cpu (tripulante1);
				//esperar_cpu (tripulante2);







				/*if(list_size(lista_tripulantes_ready)!=0){
					//funcion lista para usar con los hilos
					id_prueba=(int*)list_get(lista_tripulantes_ready, 0);
					printf("%d\n", id_prueba);
					id_prueba=(int*)list_get(lista_tripulantes_ready, 1);
					printf("%d", id_prueba);
					printf("Primer Orden", id_prueba);

				}else{
					printf("no hay datos capo");
				}

				free(tripulante);*/
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


