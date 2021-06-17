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

	for(int i=0; i<(configuracion.cant_marcos);i++){
		printf("%d\n",configuracion.marcos[i]);
	}

	agregar_memoria_aux(memoria_aux);
	imprimir_memoria(memoria_aux);
    */

	tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
	// donde arranco 134529280 -> marco 0
	printf("Posicion Inicial %d \n",&(configuracion.posicion_inicial));
	int offset = 0 ;

	memcpy((configuracion.posicion_inicial)+offset,&tripulante->tid,sizeof(int));
	offset += sizeof(int);
	memcpy((configuracion.posicion_inicial)+offset,&tripulante->estado,sizeof(char));
	offset += sizeof(char);
	memcpy((configuracion.posicion_inicial)+offset,&tripulante->posicionX,sizeof(int));


	// deserealizamos

	int size;
	int desplazamiento = 0;
	int id_patota = -1;
	char estado = "";
	int posicion = 0;
    memcpy(&id_patota, configuracion.posicion_inicial+desplazamiento, sizeof(int));
    printf("el numero del tripulante es: %d \n",id_patota);
    desplazamiento += sizeof(int);
    memcpy(&estado, configuracion.posicion_inicial+desplazamiento, sizeof(char));
    printf("el numero del tripulante es: %c\n",estado);
    desplazamiento += sizeof(char);
    memcpy(&posicion, configuracion.posicion_inicial+desplazamiento, sizeof(int));
    printf("el numero de la posicion en x es: %d\n",posicion);









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
