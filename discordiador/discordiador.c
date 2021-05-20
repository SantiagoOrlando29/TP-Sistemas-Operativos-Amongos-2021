#include "discordiador.h"
#include <semaphore.h>


config_struct configuracion;

sem_t INICIAR_TRIPULANTE;
sem_t TRABAJAR_TRIPULANTE;
int id_tripulante = 0;



int main(int argc, char* argv[]) {

	t_log* logger;

	//Reinicio el anterior y arranco uno nuevo
	FILE* archivo = fopen("discordiador.log","w");
	fclose(archivo);
	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);



	leer_config();
	int conexionMiRam = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	int conexionMongoStore = crear_conexion(configuracion.ip_mongostore, configuracion.puerto_mongostore);

	menu_discordiador(conexionMiRam, conexionMongoStore,logger);

	terminar_discordiador(conexionMiRam, conexionMongoStore, logger);


	return 0;


}

void Trabajar (int* numeroId){
	int numero = 0;
	sem_wait(&INICIAR_TRIPULANTE);

	while(numero < 10){
		printf("hola soy el hilo %d, estoy trabajando \n", numeroId);
		fflush(stdout);
		numero ++;

	}
	sem_post(&TRABAJAR_TRIPULANTE);

}



int menu_discordiador(int conexionMiRam, int conexionMongoStore,  t_log* logger) {

	 /*Hacer una funcion que cree las diferetnes listas*/
	sem_init(&INICIAR_TRIPULANTE, 0,0);
	sem_init(&TRABAJAR_TRIPULANTE, 0,1);

	t_list* lista_tripulantes_ready = list_create();
	t_list* lista_tripulantes_bloqueado = list_create();
	t_list* lista_tripulantes_trabajando = list_create();
	t_list* listaTripulantes;
	int tipoMensaje = -1;
	char* nombreThread = "";
	int cantidad_tripulantes = 0;
	int id_prueba=0;
	while(1){

		nuevoTripulante* tripulante = malloc(sizeof(nuevoTripulante));
		t_paquete* paquete;
		char* nombreHilo = "";
		char* leido = readline("");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:
				/*Solo para probar que funciona pero esto debe ser un paquete*/
				enviar_header(INICIAR_PATOTA, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				lista_tripulantes_ready=recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger, lista_tripulantes_ready);
				while(cantidad_tripulantes < 4){
					cantidad_tripulantes ++;
					pthread_t nombreHilo = (char*)(cantidad_tripulantes);
					pthread_create(&nombreHilo,NULL,(void*)Trabajar,cantidad_tripulantes);
				}
				cantidad_tripulantes = 0 ;
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
				sem_wait(&TRABAJAR_TRIPULANTE);
				sem_post(&INICIAR_TRIPULANTE);
				sem_post(&INICIAR_TRIPULANTE);
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

