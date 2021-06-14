#include "miram.h"
config_struct configuracion;
t_log* logger;



int main(void)
{

	leer_config();

	iniciar_miram(&configuracion);

	/*
	t_list * memoria_aux;
	memoria_aux=list_create();

	imprimir_ocupacion_marcos(configuracion);

	printf("%s\n",temporal_get_string_time("%H:%M:%S:%MS"));

	//posicion_patota(2,memoria_aux);

	agregar_memoria_aux(memoria_aux,&configuracion);
	//printf("Posicion de patota buscada es %d\n",posicion_patota(2,memoria_aux));

	imprimir_memoria(memoria_aux);
	printf("Posicion de patota buscada es %d\n",posicion_patota(1,memoria_aux));
	printf("Posicion de patota buscada es %d\n",posicion_patota(2,memoria_aux));
	printf("Posicion de patota buscada es %d\n",posicion_patota(1,memoria_aux));
	printf("Posicion de patota buscada es %d\n",posicion_patota(2,memoria_aux));
	printf("Posicion de patota buscada es %d\n",posicion_patota(1,memoria_aux));

	imprimir_ocupacion_marcos(configuracion);



	int destino;
*/
	int numero=1234;

	printf("COpy");
	memcpy(&(configuracion.posicion_inicial), &numero, sizeof(int));
	printf("Lo leido es %d\n",(int)(configuracion.posicion_inicial));


	printf("COpy");
	memcpy((&(configuracion.posicion_inicial)+20), "mar1", 5);
	printf("Lo leido es %s\n",(configuracion.posicion_inicial)+20);
	int a, b;
	memcpy(&a, &numero, sizeof(a));
	memcpy(&b, (int*)&numero + 1, sizeof(b));

	//printf("Lo leido es %d\n",a);
	//printf("Lo leido es %d\n",b);

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
