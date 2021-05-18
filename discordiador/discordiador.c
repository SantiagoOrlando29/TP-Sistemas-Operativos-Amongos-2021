#include "discordiador.h"

config_struct configuracion;



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

void Trabajar (int *numeroId){
	int numero = 0;
	while(numero <= 10){
	printf("hola soy el hilo %d, estoy trabajando",*numeroId);
	numero ++;
	}
}



int menu_discordiador(int conexionMiRam, int conexionMongoStore,  t_log* logger) {

	 /*Hacer una funcion que cree las diferetnes listas*/
	t_list* lista_tripulantes_ready = list_create();
	t_list* lista_tripulantes_bloqueado = list_create();
	t_list* lista_tripulantes_trabajando = list_create();
	t_list* listaTripulantes;
	int tipoMensaje = -1;
	char* nombreThread = "";

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
				//funciona mal
			//	recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger,lista_tripulantes_ready);
				lista_tripulantes_ready = recibir_paquete(conexionMiRam);

				break;

			case LISTAR_TRIPULANTES:
				/*enviar_header(LISTAR_TRIPULANTES, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);*/
				break;

			case INICIAR_PLANIFICACION:
				tripulante=(nuevoTripulante*)list_get(lista_tripulantes_ready, 0);
				printf("%d",tripulante->id);
				pthread_t nombreHilo = *(char*)(tripulante->id);
				pthread_create(&nombreHilo,NULL,(void*)Trabajar,tripulante->id);
				free(tripulante);
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

