#include"discordiador.h"

#define END 1

int estado = 0;

int main(int argc, char* argv[]) {

	t_log* logger;

	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);

	t_config* configuracion;

	configuracion = leer_config();

	char* ip_mongo  = config_get_string_value(configuracion, "IP_I_MONGO_STORE");
	char* puerto_mongo = config_get_string_value(configuracion, "PUERTO_I_MONGO_STORE");

	char* ip_miram = config_get_string_value(configuracion, "IP_MI_RAM_HQ");
	char* puerto_miram = config_get_string_value(configuracion, "PUERTO_MI_RAM_HQ");

	int conexionMiRam = crear_conexion(ip_miram,puerto_miram);
	int conexionMongoStore = crear_conexion(ip_mongo, puerto_mongo);




	armar_paquete(conexionMiRam, conexionMongoStore);

	terminar_discordiador(conexionMiRam, conexionMongoStore, logger, configuracion);



	return 0;


}


void armar_paquete(int conexMiRam, int conexMongoStore) {
	tipoMensaje tipo;
	t_paquete* paquete = NULL;

	t_log* logger;

	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);


	char* leido = "";

	//Hay que solucionar que lea codigoOperacion solo la primer parte

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

t_config* leer_config(void){
	return config_create("discordiador.config");
}

void terminar_discordiador (int conexionMiRam, int conexionMongoStore, t_log* logger, t_config* configuracion){

	log_destroy(logger);
	liberar_conexion(conexionMiRam);
	liberar_conexion(conexionMongoStore);
}

