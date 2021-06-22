#include "miram.h"
//config_struct configuracion;
//t_log* logger;


int main(void)
{

	leer_config();
	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);
	iniciar_miram(&configuracion);

	sem_init(&MUTEX_PEDIR_TAREA, 0,1);

	t_list* memoria_aux=list_create();
	agregar_memoria_aux(memoria_aux,&configuracion);

 /*	crear_marcos(cuantostripulantes,+strlen(tarea)+1);


	crear_marcos(size(patota)+size(tripulante)*N+strlen(tarea)+1){
		int tamaño_marco = leerarhivoconfiguracion;


		if(tamaño_marco - = pid <0);
		if(tamaño_marco - = puntero_tareas<0);

		for(int i i< N i++)
		if(tamaño_marco - = tid <0)
			cuantos_marcos ++;
		tamaño_marco = leerarchivoconfiguracion;
		if(tamaño_marco - = estado<0);
		if(tamaño_marco - = posicionx <0);
		if(tamaño_marco - = posiciony<0);
		if(tamaño_marco - = proximainstruc<0);
		if(tamaño_marco - = punteropcb<0);


		while(hasta que me de menor a 0)

		tamaño_marco -= strlen(tarea)+1 <0
				cuantos_marcos++;
				tamaño_marco = leerarchivoconfiguracion;


	return cuantos_marcos;

	patota=listget(mensajeRecibido);
	cuantostripulantes=listget(mensajerecibido)
	tarea=listget(mensajerecibido)

	cuantosMarcos=cuantos_marcos(cuantostripulante,tarea)

	tabla=crear_tabla(cuantosMarcos,tabla)
	add(lista_tablas,tabla);

	}
 */

/*
	tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);

	escribir_tripulante(tripulante,configuracion.posicion_inicial);

	tcbTripulante* tripulante2 = obtener_tripulante(configuracion.posicion_inicial);

	printf("Tripulante Exitoso %d, %c, %d, %d, %d", tripulante2->tid, tripulante2->estado, tripulante2->posicionX, tripulante2->posicionY, tripulante2->tid);

	agregar_tripulante_marco(tripulante2, 1,memoria_aux, &configuracion);

	tcbTripulante* tripulante3 = obtener_tripulante(configuracion.posicion_inicial + 0* atoi(configuracion.tamanio_pag));
	//tcbTripulante* tripulante4 = obtener_tripulante(configuracion.posicion_inicial + 1*configuracion.tamanio_pag);

	printf("Tripulante Exitoso Leido marco%d, %c, %d, %d, %d", tripulante3->tid, tripulante3->estado, tripulante3->posicionX, tripulante3->posicionY, tripulante3->tid);
	//tcbTripulante* tripulante5 = obtener_tripulante(configuracion.posicion_inicial + 2*configuracion.tamanio_pag);



	int cuantos_marcos_necesito = cuantos_marcos(5,15,&configuracion);

	printf("cuantos marcos necesito %d", cuantos_marcos_necesito);
*/

/*
	tarea* prueba=malloc(sizeof(tarea));
	prueba=crear_tarea(1,2,3,4,5);
	//printf("%s",tarea_a_string(prueba));


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


	int numero=1234;

	int offset = 0 ;

	printf("Copy %d\n",&(configuracion.posicion_inicial) );
	memcpy((configuracion.posicion_inicial)+offset,&numero, sizeof(int));

	offset+= sizeof(int);

	char* pruebaString = "mar1 ";
	memcpy((configuracion.posicion_inicial)+offset,pruebaString, sizeof(char*));
	offset+= sizeof(char*);
	//tcbTripulante* tripulante =crear_tripulante(1,'N',5,6,1,1);
	//memcpy((configuracion.posicion_inicial)+offset,tripulante, sizeof(tamanio_tcb));




	int size;
	int desplazamiento = 0;
	void * buffer=configuracion.posicion_inicial ;
	int tamanio=2379;

	memcpy(buffer + desplazamiento,"HOla",5);
	printf("Copy1 %s\n",(buffer + desplazamiento));

	desplazamiento+=5;

	//char* valor = malloc(sizeof(char*));
	memcpy(buffer+desplazamiento, &tamanio, sizeof(tamanio));

	printf("Copy 2%d\n",*(buffer+desplazamiento));
	/*
	desplazamiento+=sizeof(char*);
	tcbTripulante* tripulantePrueba =crear_tripulante(1,'N',5,6,1,1);
	desplazamiento+=sizeof(tcbTripulante);
	memcpy(buffer+desplazamiento,tripulantePrueba ,sizeof(tamanio_tcb));
	printf("%d", tripulantePrueba->tid);


	int destino;

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

*/

	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);
	pthread_t servidor;
	//iniciar_servidor(&configuracion);
	int hilo_servidor = 1;
	if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion))!=0){
		log_info(logger, "Falla al crearse el hilo");
	}
	pthread_join(servidor,NULL);




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
    configuracion.criterio_seleccion = config_get_string_value(archConfig, "CRITERIO_SELECCION");
    //Parametros utiles (No obtenidos del archivo de configuracion)
    configuracion.cant_marcos=0;


}
