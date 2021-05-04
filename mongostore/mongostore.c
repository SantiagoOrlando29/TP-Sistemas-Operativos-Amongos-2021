

#include "mongostore.h"

int main(void)
{
	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	logger = log_create("mongostore.log", "mongostore", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor();
	log_info(logger, "Mongo listo para recibir ordenes desde Discordiador");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* lista;
	while(1)
	{
		int tipoMensaje = recibir_operacion(cliente_fd);
		switch(tipoMensaje)
		{
		case MensajeDos:
			recibir_mensaje(cliente_fd);
			break;
		/*case PAQUETE:
			lista = recibir_paquete(cliente_fd);
			printf("Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		*/
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;
}
