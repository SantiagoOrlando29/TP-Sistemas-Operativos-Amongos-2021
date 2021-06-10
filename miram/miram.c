#include "miram.h"
config_struct configuracion;
t_log* logger;



int main(void)
{

	leer_config();
	/*
	iniciar_miram(&configuracion);
	t_list * memoria_aux;
	memoria_aux=list_create();

	for(int i=0; i<(configuracion.cant_marcos);i++){
		printf("%d\n",configuracion.marcos[i]);
	}

	agregar_memoria_aux(memoria_aux);
	imprimir_memoria(memoria_aux);
    */
	t_list* lista_recibir = list_create();
	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);
	pthread_t servidor;
	//iniciar_servidor(&configuracion);
	int hilo_servidor = 1;
	if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion))!=0){
		log_info(logger, "Falla al crearse el hilo");
	}
	pthread_join(servidor,NULL);

	list_destroy(lista_recibir);

	return 0;
}



void leer_config(){

    t_config * archConfig = config_create("miram.config");

    configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
    configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");
    configuracion.tamanio_memoria = (int)config_get_string_value(archConfig, "TAMANIO_MEMORIA");
    configuracion.squema_memoria = config_get_string_value(archConfig, "ESQUEMA_MEMORIA");
    configuracion.tamanio_pag =(int)config_get_string_value(archConfig, "TAMANIO_PAG");
    configuracion.tamanio_swap =(int)config_get_string_value(archConfig, "TAMANIO_SWAP");
    configuracion.path_swap = config_get_string_value(archConfig, "PATH_SWAP");
    configuracion.algoritmo_reemplazo = config_get_string_value(archConfig, "ALGORITMO_REEMPLAZO");
    //Parametros utiles (No obtenidos del archivo de configuracion)
    configuracion.cant_marcos=0;


}
