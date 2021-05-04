

#include "miram.h"

int main(void)
{
	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor();
	log_info(logger, "MiRam listo para recibir ordenes desde Discordiador");
	int cliente_fd = esperar_cliente(server_fd);
	nuevoTripulante* tripulante = malloc (sizeof(nuevoTripulante));
	tripulante = NULL;
	t_list* lista;
	while(1)
	{
		int tipoMensaje = recibir_operacion(cliente_fd);
		switch(tipoMensaje)
		{
		case INICIAR_PATOTA:
			lista = recibir_paquete(cliente_fd);
			/*casteo porque lo que recibo de get es un (void*) con eso lo guardo en la nueva estructura*/
			tripulante = (nuevoTripulante*)list_get(lista, 0);
			printf("\n ID: %d \n", tripulante->id );
			printf("Posicion X: %d \n", tripulante->posicionX );
			printf("Posicion Y: %d \n", tripulante->posicionY );
			printf("Pertenece a Patota: %d \n", tripulante->numeroPatota );
			free(list_get(lista,0));

			break;

		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	free(tripulante);
	return EXIT_SUCCESS;
}
