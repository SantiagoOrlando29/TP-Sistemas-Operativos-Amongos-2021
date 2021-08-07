#include "miram.h"
config_struct configuracion;
int main(void)
{
	FILE* archivo = fopen("MiRam.log","w");
	fclose(archivo);
	logger = log_create("MiRam.log","MiRam",0,LOG_LEVEL_INFO);

	int piddd = getpid();
	printf("pid %d  \n", piddd);
	signal(SIGUSR2, sig_handler);
	signal(SIGUSR1, sig_handler);

	sem_init(&MUTEX_PEDIR_TAREA, 0,1);
	sem_init(&MUTEX_CAMBIAR_ESTADO, 0,1);
	sem_init(&MUTEX_CAMBIAR_POSICION, 0,1);
	sem_init(&MUTEX_TABLA_MEMORIA, 0,1);
	sem_init(&MUTEX_LISTA_TABLAS_SEGMENTOS, 0,1);
	sem_init(&MUTEX_LISTA_TABLAS_PAGINAS, 0,1);
	sem_init(&MUTEX_PETICION_MARCOS, 0,1);
	sem_init(&MUTEX_MEM_PRINCIPAL, 0,1);
	sem_init(&MUTEX_MEM_SEC, 0,1);
	sem_init(&MUTEX_MEMORIA,0,1);
	sem_init(&MUTEX_SWAP,0,1);
	sem_init(&MUTEX_CASE,0,1);
	sem_init(&MUTEX_FILE,0,1);
	sem_init(&MUTEX_CLOCK,0,1);
	sem_init(&MUTEX_LRU,0,1);
	sem_init (&SEM_MAPA,0,1);


	leer_config();

	iniciar_miram(&configuracion);

	pthread_t hilo_mapa;
	pthread_create(&hilo_mapa,NULL,crear_mapa,NULL);
	pthread_detach(hilo_mapa);

	pthread_t servidor;
	if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion))!=0){
		log_info(logger, "Falla al crearse el hilo");
	}

	pthread_join(servidor,NULL);

	return 0;
}


void leer_config(){

    //t_config * archConfig = config_create("miram.config");
	archConfig = config_create("miram.config");

    configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
    configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");
    configuracion.tamanio_memoria = config_get_int_value(archConfig, "TAMANIO_MEMORIA");
    configuracion.squema_memoria = config_get_string_value(archConfig, "ESQUEMA_MEMORIA");
    configuracion.tamanio_pag =config_get_int_value(archConfig, "TAMANIO_PAG");
    configuracion.tamanio_swap =config_get_int_value(archConfig, "TAMANIO_SWAP");
    configuracion.path_swap = config_get_string_value(archConfig, "PATH_SWAP");
    configuracion.algoritmo_reemplazo = config_get_string_value(archConfig, "ALGORITMO_REEMPLAZO");
    configuracion.criterio_seleccion = config_get_string_value(archConfig, "CRITERIO_SELECCION");
    //Parametros utiles (No obtenidos del archivo de configuracion)
    configuracion.cant_marcos=0;
}


