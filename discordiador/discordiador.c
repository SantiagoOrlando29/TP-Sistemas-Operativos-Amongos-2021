#include"discordiador.h"

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

int menu_discordiador(int conexionMiRam, int conexionMongoStore,  t_log* logger) {
	t_paquete* paquete = NULL;
	int tipoMensaje = -1;
	nuevoTripulante* tripulante = crearNuevoTripulante(1,5,6,7);

	char* leido = "";


	while(1){
		leido = readline("");
		switch (codigoOperacion(leido)){
			case INICIAR_PATOTA:
				paquete = crear_paquete(INICIAR_PATOTA);
				char** parametros = string_split(leido, " ");
				log_info(logger,parametros[1]);
				agregar_a_paquete(paquete, tripulante, tamanioTripulante(tripulante));
				enviar_paquete(paquete, conexionMiRam);
				break;
			case LISTAR_TRIPULANTES:
				paquete = crear_paquete(LISTAR_TRIPULANTES);
				enviar_paquete(paquete, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);
				break;
			case FIN:
				return EXIT_FAILURE;
			default:
				mensajeError(logger);
		}

	}
	free(tripulante);
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

