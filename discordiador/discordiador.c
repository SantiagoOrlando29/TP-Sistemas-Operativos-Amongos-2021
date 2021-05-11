#include"discordiador.h"

#define END 1

int estado = 0;

config_struct configuracion;

int main(int argc, char* argv[]) {

	t_log* logger;

	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);

	leer_config();

	int conexionMiRam = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	int conexionMongoStore = crear_conexion(configuracion.ip_mongostore, configuracion.puerto_mongostore);

	armar_paquete(conexionMiRam, conexionMongoStore);

	terminar_discordiador(conexionMiRam, conexionMongoStore, logger);


	return 0;


}


void armar_paquete(int conexMiRam, int conexMongoStore) {
	tipoMensaje tipo;
	t_paquete* paquete = NULL;
	int tipoMensaje = -1;
	t_log* logger;
	t_list* lista;


	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);


	char* leido = "";


	while(estado != END){
		leido = readline("");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:
				tipo = INICIAR_PATOTA;
				paquete = crear_paquete(tipo);
				char** parametros = string_split(leido, " ");
				log_info(logger,parametros[1]);
				nuevoTripulante* tripulante = crearNuevoTripulante(1,5,6,7);
				agregar_a_paquete(paquete, tripulante, tamanioTripulante(tripulante));
				enviar_paquete(paquete, conexMiRam);
				break;
			case LISTAR_TRIPULANTES:
				tipo = LISTAR_TRIPULANTES;
				paquete = crear_paquete(tipo);
				enviar_paquete(paquete, conexMiRam);
				tipoMensaje = recibir_operacion(conexMiRam);
				if (tipoMensaje == 1){
					printf("recibi el paquete indicado");
					lista = recibir_paquete(conexMiRam);
					/*casteo porque lo que recibo de get es un (void*) con eso lo guardo en la nueva estructura*/
					tripulante = (nuevoTripulante*)list_get(lista, 0);
					printf("\n ID: %d \n", tripulante->id );
					printf("Posicion X: %d \n", tripulante->posicionX );
					printf("Posicion Y: %d \n", tripulante->posicionY );
					printf("Pertenece a Patota: %d \n", tripulante->numeroPatota );
					free(list_get(lista,0));
				}
				break;
			case FIN:
				estado = END;
				break;
			default:
				mensajeError();
		}

	}



	free(leido);
	eliminar_paquete(paquete);
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

