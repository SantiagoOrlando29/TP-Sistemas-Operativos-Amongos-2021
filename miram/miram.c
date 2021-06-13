#include "miram.h"
config_struct configuracion;
t_log* logger;



int main(void)
{

	leer_config();

	iniciar_miram(&configuracion);
	t_list * memoria_aux;
	memoria_aux=list_create();

	imprimir_ocupacion_marcos(configuracion);


	agregar_memoria_aux(memoria_aux,&configuracion);
	imprimir_memoria(memoria_aux);

	imprimir_ocupacion_marcos(configuracion);

	int destino;

	memcpy((configuracion.posicion_inicial), "mar1", 5);
	memcpy((configuracion.posicion_inicial+1*atoi(configuracion.tamanio_pag)), "mar2", 5);
	memcpy((configuracion.posicion_inicial+2*atoi(configuracion.tamanio_pag)), "mar3", 5);
	memcpy((configuracion.posicion_inicial+3*atoi(configuracion.tamanio_pag)), "mar4", 5);
	memcpy((configuracion.posicion_inicial+4*atoi(configuracion.tamanio_pag)), "mar5", 5);
	memcpy((configuracion.posicion_inicial+5*atoi(configuracion.tamanio_pag)), "mar6", 5);
	memcpy((configuracion.posicion_inicial+6*atoi(configuracion.tamanio_pag)), "mar7", 5);
	memcpy((configuracion.posicion_inicial+7*atoi(configuracion.tamanio_pag)), "mar8", 5);
	memcpy((configuracion.posicion_inicial+8*atoi(configuracion.tamanio_pag)), "mar9", 5);
	memcpy((configuracion.posicion_inicial+9*atoi(configuracion.tamanio_pag)), "mar10", 5);
	memcpy((configuracion.posicion_inicial+2000*atoi(configuracion.tamanio_pag)), "mar10", 5);



	printf("Lo leido es %s\n",(configuracion.posicion_inicial));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+1*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+2*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+3*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+4*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+5*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+6*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+7*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+8*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+9*atoi(configuracion.tamanio_pag)));
	printf("Lo leido es %s\n",(configuracion.posicion_inicial+2000*atoi(configuracion.tamanio_pag)));



/*
	t_list* lista_recibir = list_create();
	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);
	pthread_t servidor;
	//iniciar_servidor(&configuracion);
	int hilo_servidor = 1;
	pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion);
	pthread_join(servidor,NULL);
	//pthread_detach(servidor);
	if(hilo_servidor != 0 )
		log_info(logger, "Falla al crearse el hilo");
	else
		log_info(logger, "Hilo creado correctamente");



	list_destroy(lista_recibir);
*/
	return 0;
}



void leer_config(){

    t_config * archConfig = config_create("miram.config");

    configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
    configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");
    configuracion.tamanio_memoria = config_get_string_value(archConfig, "TAMANIO_MEMORIA");
    configuracion.squema_memoria = config_get_string_value(archConfig, "ESQUEMA_MEMORIA");
    configuracion.tamanio_pag =config_get_string_value(archConfig, "TAMANIO_PAG");
    configuracion.tamanio_swap =config_get_string_value(archConfig, "TAMANIO_SWAP");
    configuracion.path_swap = config_get_string_value(archConfig, "PATH_SWAP");
    configuracion.algoritmo_reemplazo = config_get_string_value(archConfig, "ALGORITMO_REEMPLAZO");
    //Parametros utiles (No obtenidos del archivo de configuracion)
    configuracion.cant_marcos=0;


}
