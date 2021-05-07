#include"discordiador.h"


int main(int argc, char* argv[]) {

	t_log* logger;

	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);

	t_config* configuracion;

	configuracion = leer_config();

	int conexionMiRam = crear_conexion(
			config_get_string_value(configuracion, "IP_MI_RAM_HQ"),
			config_get_string_value(configuracion, "PUERTO_MI_RAM_HQ")
	);
	int conexionMongoStore = crear_conexion(
			config_get_string_value(configuracion, "IP_I_MONGO_STORE"),
			config_get_string_value(configuracion, "PUERTO_I_MONGO_STORE")
	);


	//char* mensajeUno = "MensajeUno";
	//char* mensajeDos = "MensajeDos";
	t_paquete* paquete = armar_paquete();

	enviar_paquete(paquete, conexionMiRam);
	//enviar_mensaje(mensajeUno, conexionMiRam);
	//enviar_mensaje(mensajeDos, conexionMongoStore);



	terminar_discordiador(conexionMiRam, conexionMongoStore, logger, configuracion, paquete);


	return 0;


}

t_paquete* armar_paquete() {
	tipoMensaje tipo;
	t_paquete* paquete = NULL;

	char* leido = readline("");

	switch (codigoOperacion(leido)){
		case INICIAR_PATOTA:
			tipo = INICIAR_PATOTA;
			paquete = crear_paquete(tipo);
			nuevoTripulante* tripulante = crearNuevoTripulante(1,5,6,7);
			agregar_a_paquete(paquete, tripulante, tamanioTripulante(tripulante));
			break;
		default:
			mensajeError();
			getchar();
	}



	free(leido);
	return paquete;
}

t_config* leer_config(void){
	return config_create("discordiador.config");
}

void terminar_discordiador (int conexionMiRam, int conexionMongoStore, t_log* logger, t_config* configuracion, t_paquete* paquete){

	log_destroy(logger);
	liberar_conexion(conexionMiRam);
	liberar_conexion(conexionMongoStore);
	eliminar_paquete(paquete);
}

