#include "miram.h"
config_struct configuracion;
t_log* logger;



int main(void)
{
	leer_config();

	void iterator(char* value)
	{
		printf("%s\n", value);
	}


	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);
	int tipoMensaje;
	int server_fd = iniciar_servidor(configuracion.ip_miram,configuracion.puerto_miram);
	log_info(logger, "MiRam listo para recibir ordenes desde Discordiador");
	int discordiador = esperar_cliente(server_fd);
	while(1)
	{
		t_list* lista = list_create();
		nuevoTripulante* tripulante;
		t_paquete* paquete;
		tipoMensaje = recibir_operacion(discordiador);

		if(tipoMensaje > 0){
			switch(tipoMensaje)
			{
			case INICIAR_PATOTA:
				lista = recibir_paquete(discordiador);
				/*casteo porque lo que recibo de get es un (void*) con eso lo guardo en la nueva estructura*/
				tripulante = (nuevoTripulante*)list_get(lista, 0);
				printf("\n ID: %d \n", tripulante->id );
				printf("Posicion X: %d \n", tripulante->posicionX );
				printf("Posicion Y: %d \n", tripulante->posicionY );
				printf("Pertenece a Patota: %d \n", tripulante->numeroPatota );
				break;

			case LISTAR_TRIPULANTES:
				paquete = crear_paquete(LISTAR_TRIPULANTES);
				tripulante = crearNuevoTripulante(10,5,6,7);
				agregar_a_paquete(paquete, tripulante, tamanioTripulante(tripulante));
				enviar_paquete(paquete, discordiador);
				eliminar_paquete(paquete);
				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				return EXIT_FAILURE;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
			}
			free(tripulante);

			list_destroy(lista);
		}


	}
	return EXIT_SUCCESS;
}



void leer_config(){

    t_config * archConfig = config_create("miram.config");

    configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
    configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");
    configuracion.ip_mongostore = config_get_string_value(archConfig, "IP_I_MONGO_STORE");
    configuracion.puerto_mongostore = config_get_string_value(archConfig, "PUERTO_I_MONGO_STORE");

}
