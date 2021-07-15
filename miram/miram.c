#include "miram.h"
config_struct configuracion;
t_log* logger;


int main(void)
{

	leer_config();
	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);
	iniciar_miram(&configuracion);


	//t_list* memoria_aux=list_create();
	//agregar_memoria_aux(memoria_aux,&configuracion);

	pthread_t servidor;
	iniciar_servidor(&configuracion);
	int hilo_servidor = 1;
	if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion))!=0){
		log_info(logger, "Falla al crearse el hilo");
	}
	pthread_join(servidor,NULL);


	/*

	imprimir_ocupacion_marcos(&configuracion);

	int marcos = cuantos_marcos(1,10,&configuracion);



	printf("Cantidad utilizados: %d\n", marcos);

	reservar_marco(5,&configuracion, memoria_aux, 1);

	reservar_marco(8,&configuracion,memoria_aux, 2);

	reservar_marco(2,&configuracion, memoria_aux, 3);

	reservar_marco(6,&configuracion, memoria_aux, 4);

	imprimir_memoria(memoria_aux);



	imprimir_ocupacion_marcos(&configuracion);
*/
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


	t_list* lista_recibir = list_create();
	logger = log_create("MiRam.log", "MiRam", 1, LOG_LEVEL_DEBUG);


	list_destroy(lista_recibir);


/*
	tabla_espacios_de_memoria = list_create();
	espacio_de_memoria* memoria_principal = crear_espacio_de_memoria(0, atoi(configuracion.tamanio_memoria), true);
	list_add(tabla_espacios_de_memoria, memoria_principal);
	imprimir_tabla_espacios_de_memoria();


	uint32_t numero_patota;

	numero_patota = 1;

	pcbPatota* pcb_patota1 = crear_pcb(numero_patota);


	tcbTripulante* tripulante_1 = crear_tripulante(1,'N',5,6,1,1);
	tcbTripulante* tripulante_2 = crear_tripulante(2,'N',7,8,1,1);

	char* tarea_prueba = malloc(24);
	tarea_prueba = "GENERAR_OXIGENO 1;4;4;1";

	espacio_de_memoria* espacio_de_memoria_pcb_patota1 = asignar_espacio_de_memoria(tamanio_pcb(pcb_patota1));
	imprimir_tabla_espacios_de_memoria();

	/*tabla_segmentacion* tabla_segmentos_patota1;
	tabla_segmentos_patota1->id_patota = 1;
	tabla_segmentos_patota1->segmento_inicial = list_create();
	list_add(tabla_segmentos_patota1->segmento_inicial, pcb_patota1);
	list_add(tabla_segmentos_patota1->segmento_inicial, tripulante_1);
	list_add(tabla_segmentos_patota1->segmento_inicial, tripulante_2);
	list_add(tabla_segmentos_patota1->segmento_inicial, tarea_prueba);
*/


	//imprimir_memoria(memoria_aux);
	return 0;
}

espacio_de_memoria* crear_espacio_de_memoria(int base, int tam, bool libre){
	espacio_de_memoria* nuevo_espacio_de_memoria = malloc(sizeof(espacio_de_memoria));

	nuevo_espacio_de_memoria->base = base;
	nuevo_espacio_de_memoria->tam = tam;
	nuevo_espacio_de_memoria->libre = libre;

    return nuevo_espacio_de_memoria;
}

void imprimir_tabla_espacios_de_memoria(){
    int size = list_size(tabla_espacios_de_memoria);
    printf("<--------------------------------------------\n");

    for(int i=0; i < size; i++) {
    	espacio_de_memoria *espacio = list_get(tabla_espacios_de_memoria, i);
        printf("base: %d, tam: %d, is_free: %s \n", espacio->base, espacio->tam, espacio->libre ? "true" : "false");
    }
    printf("-------------------------------------------->\n");
}

void eliminar_espacio_de_memoria(int base){
    for(int i = 0; i < list_size(tabla_espacios_de_memoria); i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

    	if(espacio->base == base){
            espacio->libre = true;
            log_info(logger, "Se libera el espacio de memoria con base %d", espacio->base);
        }
    }
}

espacio_de_memoria* buscar_espacio_de_memoria_libre(int tam){

	if (strcmp(configuracion.criterio_seleccion, "FF") == 0) {
        log_debug(logger, "First fit");
        return busqueda_first_fit(tam);

    } else if (strcmp(configuracion.criterio_seleccion, "BF") == 0) {
        log_debug(logger, "Best fit");
        return busqueda_best_fit(tam);

    } else {
        log_error(logger, "No se encontro el algoritmo pedido");
        exit(EXIT_FAILURE);
    }
}

espacio_de_memoria* busqueda_first_fit(int tam){
    int size = list_size(tabla_espacios_de_memoria);

    for(int i=0; i < size; i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);
        if(espacio->libre == true && tam <= espacio->tam){
            log_info(logger, "Se encontro espacio de memoria libre con suficiente espacio (base: %d)", espacio->base);
            return espacio;
        }
    }
    log_warning(logger, "No hay espacios de memoria libres");
    return NULL;
}

espacio_de_memoria* busqueda_best_fit(int tam){
    int size = list_size(tabla_espacios_de_memoria);
    t_list* espacios_libres = list_create();

    for(int i=0; i < size; i++){
    	espacio_de_memoria* espacio = list_get(tabla_espacios_de_memoria, i);

    	if(espacio->libre == true && tam <= espacio->tam){
            log_info(logger, "Se encontro espacio de memoria libre con suficiente espacio (base: %d)", espacio->base);

            if(tam == espacio->tam){
            	log_info(logger, "Se encontro el espacio de memoria con Best Fit (base:%d)", espacio->base);
            	return espacio;
            }
            list_add(espacios_libres, espacio);
        }
    }

    log_debug(logger, "Buscando el espacio de memoria con Best Fit");
    int espacios_libres_size = list_size(espacios_libres);

    if(espacios_libres_size != 0){
    	espacio_de_memoria* espacio_best_fit;
        int best_fit_diff = 999999; //revisar esto

        for(int i=0; i < espacios_libres_size; i++){
        	espacio_de_memoria* y = list_get(espacios_libres, i);
            int diff = y->tam - tam;

            if(best_fit_diff > diff){
                best_fit_diff = diff;
                espacio_best_fit = y;
            }
        }
        log_info(logger, "El espacio de memoria encontrado con Best Fit (base:%d)", espacio_best_fit->base);
        return espacio_best_fit;
    }else{
        log_warning(logger, "No hay espacios de memoria libres");
        return NULL;
    }
}

espacio_de_memoria* asignar_espacio_de_memoria(size_t tam) { //falta
	espacio_de_memoria *espacio_libre = buscar_espacio_de_memoria_libre(tam);
	if (espacio_libre != NULL) {
        //Si la particion libre encontrada es de igual tamanio a la particion a alojar no es necesario ordenar
        if (espacio_libre->tam == tam) {
        	espacio_libre->libre = false;
            //log_info(logger, "Espacio de memoria asignado(base: %d)", espacio_libre->base);
            return espacio_libre;
        } else {
        //Si no es de igual tamano, debo crear una nueva particion con base en la libre y reacomodar la base y tamanio de la libre.
            espacio_de_memoria* nuevo_espacio_de_memoria = crear_espacio_de_memoria(espacio_libre->base, tam, false);
            list_add(tabla_espacios_de_memoria, nuevo_espacio_de_memoria);
            //actualizo base y tamanio de particion libre.
           // espacio_libre->base += tam;
           // espacio_libre->tam -= tam;
            return nuevo_espacio_de_memoria;
        }

    } else {
        log_warning(logger, "It was not possible to assign partition!");
        return NULL;
    }
}

void leer_config(){

    t_config * archConfig = config_create("miram.config");

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
