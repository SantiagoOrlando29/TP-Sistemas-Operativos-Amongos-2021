#include "mongostore.h"
config_struct configuracion;
t_log* logger;

int main(void)
{

	leer_config();

	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	logger = log_create("mongostore.log", "mongostore", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor(configuracion.ip_mongostore, configuracion.puerto_mongostore);
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

void leer_config(){

    t_config * archConfig = config_create("mongostore.config");

    configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
    configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");
    configuracion.ip_mongostore = config_get_string_value(archConfig, "IP_I_MONGO_STORE");
    configuracion.puerto_mongostore = config_get_string_value(archConfig, "PUERTO_I_MONGO_STORE");
    //configuracion.grado_multitarea= config_get_int_value(archConfig, "GRADO_MULTITAREA");
    //configuracion.algoritmo= config_get_string_value(archConfig, "ALGORITMO");
    //configuracion.quantum = config_get_int_value(archConfig, "QUANTUM");
    //configuracion.duracion_sabotaje = config_get_int_value(archConfig, "DURACION_SABOTAJE");
    //configuracion.retardo_cpu = config_get_int_value(archConfig, "RETARDO_CICLO_CPU");


    //destruir config, donde?
}
