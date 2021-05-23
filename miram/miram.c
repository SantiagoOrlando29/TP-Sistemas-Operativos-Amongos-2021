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
	//por el momento hasta que implementemos la memoria
	//t_list* tripulantes=list_create();
	//t_list* patotas=list_create();
	uint32_t pid=1; //ID_PATOTA
	while(1)
	{
		t_list* lista = list_create();
		pcbPatota* patota;
		tcbTripulante* tripulante;
		t_paquete* paquete;
		tipoMensaje = recibir_operacion(discordiador);
			switch(tipoMensaje)
			{
			case INICIAR_PATOTA:
				lista=recibir_paquete(discordiador);
				patota=crear_patota(pid,0);
				pid++;
				int i;
				for(i=0;i<list_size(lista);i++){
					tripulante=(tcbTripulante*)list_get(lista,i);
					mostrar_tripulante(tripulante,patota);
					printf("\n");

				}
				break;

			case LISTAR_TRIPULANTES:
				paquete = crear_paquete(LISTAR_TRIPULANTES);
				tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
				agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));
				enviar_paquete(paquete, discordiador);
				eliminar_paquete(paquete);
				break;

			case FIN:
				log_error(logger, "el discordiador finalizo el programa. Terminando servidor");
				return EXIT_FAILURE;

			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				return EXIT_FAILURE;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
			}
			free(patota);
			free(tripulante);
			list_destroy(lista);

	}
	return EXIT_SUCCESS;
}



void leer_config(){

    t_config * archConfig = config_create("miram.config");

    configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
    configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");

    //configuracion.tamanio_memoria=config_get_string_value(archConfig,"TAMANIO_MEMORIA");

    //configuracion.ip_mongostore = config_get_string_value(archConfig, "IP_I_MONGO_STORE");
    //configuracion.puerto_mongostore = config_get_string_value(archConfig, "PUERTO_I_MONGO_STORE");

}
