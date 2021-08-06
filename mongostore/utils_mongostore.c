//los char* de las list salen con o sin el fin de linea

//EXIT_FAILURES???

//TODO: Ver de donde sale IP
#include "utils_mongostore.h"

t_recurso_data lista_recursos[] = {
	{"OXIGENO", 'O', "Oxigeno.ims", NULL, NULL},
	{"COMIDA", 'C', "Comida.ims", NULL, NULL},
	{"BASURA", 'B', "Basura.ims", NULL, NULL}
};

t_tarea lista_tareas_disponibles[] = {
	{"GENERAR_OXIGENO", GENERAR, OXIGENO},
	{"GENERAR_COMIDA", GENERAR, COMIDA},
	{"GENERAR_BASURA", GENERAR, BASURA},
	{"CONSUMIR_OXIGENO", CONSUMIR, OXIGENO},
	{"CONSUMIR_COMIDA", CONSUMIR, COMIDA},
	{"DESCARTAR_BASURA", DESCARTAR, BASURA}
};

int variable_servidor = -1;
int socket_servidor;
int cant_sabotaje = 0;
int sabotajes_indice = 0;
int socket_cliente_sabotaje = 0;

///////////////////////FUNCIONES AGRUPADAS Y ORDENADAS "CRONOLOGICAMENTE"//////////////////////////////////////////////////////////////

//Lee la configuracion contenida en el archivo de configuracion definido y la carga en el sistema
void leer_config()
{
    t_config* config = config_create(ARCHIVO_CONFIGURACION);

	if(config == NULL)
	{
		log_error(logger, "No se puede cargar la configuracion. Se finaliza el programa");
		exit(EXIT_FAILURE);
	}

    configuracion.ip = string_duplicate(config_get_string_value(config, "IP"));//no la da el archivo de configuracion
    configuracion.puerto = string_duplicate(config_get_string_value(config, "PUERTO"));
    configuracion.punto_montaje = string_duplicate(config_get_string_value(config,"PUNTO_MONTAJE"));
    configuracion.tiempo_sincronizacion = config_get_int_value(config,"TIEMPO_SINCRONIZACION");
    configuracion.posiciones_sabotaje = config_get_array_value(config,"POSICIONES_SABOTAJE");

    log_info(logger, "Configuracion leida y almacenada en \"configuracion\"");

    config_destroy(config);
}

void blocks_abrir_hilo_sincronizacion()
{
	log_debug(logger, "Se entra a blocks_abrir_hilo");

    if((pthread_create(&thread_sincronizacion, NULL, (void*)blocks_sincronizar_segun_tiempo_sincronizacion,NULL))!=0){
        log_info(logger, "Falla al crearse el hilo");
    }

    pthread_detach(thread_sincronizacion);
}

int blocks_sincronizar_segun_tiempo_sincronizacion()
{
	while(1)
	{
		sleep(configuracion.tiempo_sincronizacion);
		sem_wait(&MUTEX_BLOCKS);
		msync(blocks_address, blocks_size, MS_SYNC);
		sem_post(&MUTEX_BLOCKS);
		//log_debug(logger, "Blocks.ims actualizado");
	}
	return OK;
}

void semaforos_inicializar()
{
	sem_init(&MUTEX_SUPERBLOQUE_BITMAP, 0, 1);
	sem_init(&MUTEX_SUPERBLOQUE_PATH, 0, 1);
	sem_init(&MUTEX_BLOCKS, 0, 1);
	sem_init(&MUTEX_OXIGENO_MD, 0, 1);
	sem_init(&MUTEX_COMIDA_MD, 0, 1);
	sem_init(&MUTEX_BASURA_MD, 0, 1);
	sem_init(&MUTEX_OXIGENO_IMS, 0, 1);
	sem_init(&MUTEX_COMIDA_IMS, 0, 1);
	sem_init(&MUTEX_BASURA_IMS, 0, 1);
	sem_init(&MUTEX_BITACORA, 0, 1);
}

///////////////////////COMIENZO DE FUNCIONES NECESARIAS PARA INICIAR EL FILE SYSTEM///////////////////////////////

//Inicia el File System restaurando a memoria los archivos existentes si los hubiera
void file_system_iniciar()
{
	//log_debug(logger, "Info-Comienza iniciar_file_system");
	utils_crear_directorio_si_no_existe(configuracion.punto_montaje);
	file_system_generar_rutas_completas();

	file_system_verificar_existencia_previa();
	//utils_eliminar_carpeta_completa_si_existe(bitacoras_path);
	//bitacoras_eliminar_si_existen();
	blocks_validar_existencia();
	recursos_validar_existencia_metadatas(); //BORRAR -- en principio no seria necesaria


	files_crear_directorios_inexistentes(); //Esto se asegura de que ya esten o los crea
	bitacoras_eliminar_si_existen();
	log_info(logger, "File System iniciado");
}

//Elimina las bitacoras si existen
void bitacoras_eliminar_si_existen()
{
	char* bloques_ocupados = bitacoras_obtener_cadena_con_el_total_de_bloques_ocupados();
	metadata_liberar_bloques_en_bitmap_y_en_blocks(bloques_ocupados);
	utils_eliminar_carpeta_completa_si_existe(bitacoras_path);
    utils_crear_directorio_si_no_existe(bitacoras_path);
    free(bloques_ocupados);
}

//Genera el total de rutas que se usaran para el File System almacenandolas en variables globales
void file_system_generar_rutas_completas()
{
	superbloque_path = string_from_format("%s/%s", configuracion.punto_montaje, "SuperBloque.ims");
	blocks_path = string_from_format("%s/%s", configuracion.punto_montaje, "Blocks.ims");
	files_path = string_from_format("%s/%s", configuracion.punto_montaje, "Files");
	recursos_cargar_rutas();
	bitacoras_path = string_from_format("%s/%s", files_path, "Bitacoras");
}

//Carga en el sistema las rutas usadas por los recursos existentes en la lista de recursos
void recursos_cargar_rutas()
{
	int cantidad_recursos = recursos_cantidad();

	for(int i = 0; i < cantidad_recursos; i++)
	{
		lista_recursos[i].ruta_completa = string_from_format("%s/%s", files_path, lista_recursos[i].nombre_archivo);
	}
}

//Devuelve la cantidad de recursos que tiene la carpeta files
int recursos_cantidad()
{
	return (sizeof(lista_recursos) / sizeof(t_recurso_data));
}

//Verifica la previa existencia del archivo SuperBloque.ims en el sistema, y si lo encuentra ofrece si desea restaurar el File System o iniciarlo limpio
void file_system_verificar_existencia_previa()
{
	if(utils_existe_en_disco(superbloque_path))
	{
		superbloque_levantar_estructura_desde_archivo();
		file_system_consultar_para_formatear();
	}
	else
	{
		file_system_iniciar_limpio();
	}
}

//Levanta la estructura en memoria del superbloque desde el archivo
void superbloque_levantar_estructura_desde_archivo()
{
	//log_debug(logger, "I-Se ingresa a superbloque_levantar_estructura_desde_archivo");
	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);

	//Los siguientes datos UNICAMENTE podrian ser levantados correctamente del archivo
	//si previamente se almacenaron en el orden definido por el enunciado
	int desplazamiento = 0;
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.block_size, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	superbloque_asignar_memoria_a_bitmap();
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, superbloque.bitmap, bitmap_size);

	close(superbloque_fd);
	log_debug(logger, "O-Superbloque levantado desde archivo");
}

//Setea el tamanio del bitmap en bytes en base a la cantidad de bloques de superbloque.blocks y le asigna dicho espacio en memoria
void superbloque_asignar_memoria_a_bitmap()
{
	//Tamanio del bitmap teniendo en cuenta que pueda haber un byte incompleto
	bitmap_size = 1 + (superbloque.blocks - 1) / CHAR_BIT;
	if(superbloque.bitmap != NULL)
	{
		free(superbloque.bitmap);
	}
	superbloque.bitmap = malloc(bitmap_size);
	log_debug(logger, "IO-Al salir de superbloque_asignar_memoria_a_bitmap se tiene bitmap_size %d", bitmap_size);
}

//Consulta por consola al usuario si desea levantar un File System existente o crear uno nuevo
void file_system_consultar_para_formatear()
{
	char formatear_char;
	printf("Se encontro informacion de un File System previo con tamanio de bloques = %d y bloques = %d. Quisieras formaterlo? (S/N) ",
			superbloque.block_size, superbloque.blocks);
	formatear_char = getchar();
	getchar();

	while(strchr("sSnN", formatear_char) == NULL)
	{
		printf("La respuesta ingresada no es valida, prueba otra vez... quisieras formatear el File System? (S/N) ");
		formatear_char = getchar();
		printf("\n");
		getchar();
	}

	if(formatear_char == 's' || formatear_char == 'S')
	{
		file_system_iniciar_limpio();
	}
}

//Inicia un File System limpio eliminando cualquier archivo existente previo
void file_system_iniciar_limpio()
{
	file_system_eliminar_archivos_previos();
	superbloque_generar_estructura_con_valores_tomados_por_consola();
	utils_crear_archivo(superbloque_path);
	superbloque_cargar_archivo();
}

//Elimina los archivos que pudieran estar desde antes en el File System (para el caso en que deba iniciarse limpio)
void file_system_eliminar_archivos_previos()
{
	remove(blocks_path); //Elimina el archivo Blocks.ims//ex blocks_eliminar_archivo();
	utils_eliminar_carpeta_completa_si_existe(files_path);
	log_info(logger, "Se eliminan archivos previos");
}

//si existe, limina la carpeta Files y t.odo su contenido (incluyendo la carpeta Bitacoras y su contenido)
//void files_eliminar_carpeta_completa()
//{
//	if(utils_existe_en_disco(files_path))
//	{
//		char* buffer = string_from_format("rm -rf %s", files_path);
//		system(buffer);
//		free(buffer);
//	}
//}

//Si existe, elimina la carpeta ubicada en path y t.odo su contenido directo e indirecto
void utils_eliminar_carpeta_completa_si_existe(char* path)
{
	if(utils_existe_en_disco(path))
	{
		char* buffer = string_from_format("rm -rf %s", path);
		system(buffer);
		free(buffer);
	}
}

//Toma por consola los valores del tamanio de bloques y de cantidad de bloques que setearan a SuperBloque.ims
void superbloque_generar_estructura_con_valores_tomados_por_consola()
{
	//log_debug(logger, "I-Se ingresa a superbloque_generar_estructura_con_valores_tomados_por_consola para tomar por consola el tamanio y la cantidad de bloques");
	printf("Se procedera a crear el File System del modulo i-Mongo-Store para AmongOs...\n"
			"Indique por favor cual sera el tamanio que tendran los bloques en el sistema: ");
	scanf("%d", &superbloque.block_size);

	printf("Cual sera la cantidad de bloques: ");
	scanf("%d", &superbloque.blocks);

	printf("Muchas gracias!\n");

	superbloque_asignar_memoria_a_bitmap();
	superbloque_setear_bitmap_a_cero();

	log_debug(logger, "Se ingreso por block_size %d y blocks %d. Ademas se genero el bitmap vacio",
			superbloque.block_size, superbloque.blocks);
}

//Setea el total de bytes del bitmap en el SuperBloque en memoria a 0
void superbloque_setear_bitmap_a_cero()
{
	memset(superbloque.bitmap, 0, bitmap_size);
}

//Se carga el archivo SuperBloque.ims con los valores de la estructura superbloque
void superbloque_cargar_archivo()
{
	//log_debug(logger, "I-Se ingresa a superbloque_cargar_archivo");
	int superbloque_fd;
	superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);

	int desplazamiento = 0;
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	write(superbloque_fd, &superbloque.block_size, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	write(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	write(superbloque_fd, superbloque.bitmap, bitmap_size);
	close(superbloque_fd);
	log_debug(logger, "O-Archivo SuperBloque.ims cargado");
}

//Verifica la existencia del archivo Blocks.ims y su copia en la memoria. Si no existieran se generan
void blocks_validar_existencia()
{
	blocks_validar_existencia_del_archivo();
	blocks_mapear_archivo_a_memoria();

	log_debug(logger, "O-Existencia de Blocks validada");
}

//Verifica si existe el archivo del superbloque y si no existe se crea inicializandolo
void blocks_validar_existencia_del_archivo()
{
	//Calculo del tamanio del archivo de Blocks.ims
	blocks_size = superbloque.blocks * superbloque.block_size;

	if(utils_existe_en_disco(blocks_path) == 0)
	{
		utils_crear_archivo(blocks_path);
		utils_dar_tamanio_a_archivo(blocks_path, blocks_size);
	}
	//log_debug(logger, "O-Existencia del archivo Blocks.ims validada");
}

//Mapea a memoria el contenido del archivo Blocks.ims
void blocks_mapear_archivo_a_memoria()
{
	//log_debug(logger, "I-Se ingresa a blocks_mapear_archivo_a_memoria");
	int blocks_fd = utils_abrir_archivo_para_lectura_escritura(blocks_path);

	blocks_address = mmap(NULL, blocks_size, PROT_WRITE | PROT_READ, MAP_SHARED, blocks_fd, 0);

	close(blocks_fd);

	log_debug(logger, "O-Blocks mapeado del archivo a la memoria");
}

//Valida la existencia de las metadatas de los recursos existentes en la lista de recursos
void recursos_validar_existencia_metadatas()
{
	int cantidad_recursos = recursos_cantidad();

	for(int i = 0; i < cantidad_recursos; i++)
	{
		recurso_validar_existencia_metadata_en_memoria(&lista_recursos[i]);
	}
	log_debug(logger, "Salgo de recursos_validar_existencia_metadatas()");
}

//Si no existe la metadata en memoria, la genera con los valores del archivo o con sus valores default si este ultimo tampoco estuviera
void recurso_validar_existencia_metadata_en_memoria(t_recurso_data* recurso_data)
{
	//log_debug(logger, "I-Entro a recurso_validar_existencia_metadata_en_memoria para el recurso %s", recurso_data->nombre);
	if(recurso_data->metadata == NULL)
	{
		recurso_data->metadata = malloc(sizeof(t_recurso_md));
		recurso_data->metadata->caracter_llenado = string_from_format("%c", recurso_data->caracter_llenado);
	}

	if(utils_existe_en_disco(recurso_data->ruta_completa))
	{
		recurso_levantar_de_archivo_a_memoria_valores_variables(recurso_data);
	}
	else
	{
		//free(recurso_data->metadata->blocks);//OK? SANTI
		metadata_setear_con_valores_default_en_memoria(recurso_data->metadata);
	}
	log_debug(logger, "Existencia de metadata para el recurso %s validada. Valor fijo CARACTER_LLENADO %s",
			recurso_data->nombre, recurso_data->metadata->caracter_llenado);
}

//Levanta a la estructura metadata en memoria los valores variables contenidos en el archivo de metadata correspondiente
void recurso_levantar_de_archivo_a_memoria_valores_variables(t_recurso_data* recurso_data)
{
	t_config* recurso_config = config_create(recurso_data->ruta_completa);
	t_recurso_md* recurso_md = recurso_data->metadata;

	//free(recurso_md->blocks);//CHEQUEAR

	recurso_md->size = config_get_int_value(recurso_config, "SIZE");
	recurso_md->block_count = config_get_int_value(recurso_config, "BLOCK_COUNT");
	recurso_md->blocks = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));
	strcpy(recurso_md->md5_archivo, config_get_string_value(recurso_config, "MD5_ARCHIVO"));

	config_destroy(recurso_config);

	log_debug(logger, "Valores variables levantados SIZE %d BLOCK_COUNT %d BLOCKS %s y MD5_ARCHIVO %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo);
}

//Setea en memoria los valores variables de la metadata del recurso dado con los valores default
void metadata_setear_con_valores_default_en_memoria(t_recurso_md* recurso_md)
{
	//log_debug(logger, "I-Se ingresa a recurso_inicializar_metadata_en_memoria para el recurso con caracter de llenado %s", recurso_md->caracter_llenado);
	recurso_md->size = 0;
	recurso_md->block_count = 0;
	recurso_md->blocks = string_duplicate("[]");
	strcpy(recurso_md->md5_archivo, "d41d8cd98f00b204e9800998ecf8427e");

	//log_debug(logger, "Valores default seteados en memoria: SIZE %d BLOCK_COUNT %d BLOCKS %s y MD5_ARCHIVO %s",
			//recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo);
}

//Crea las carpetas necesarias para persistir la informacion delasignar File System
void files_crear_directorios_inexistentes()
{
	utils_crear_directorio_si_no_existe(files_path);
	utils_crear_directorio_si_no_existe(bitacoras_path);

	log_debug(logger, "Directorios del File System confirmados");
}

///////////////FIN DE FUNCIONES NECESARIAS PARA INICIAR EL FILE SYSTEM//////////////////////////////////////////////////////

////////////////////////////FUNCIONES NECESARIAS PARA ATENDER LA SEÑAL SIGUSR1////////////////////

//Funcion manejadora de la senial para notificar al discordiador de un sabotaje
void sig_handler(int signum)
{
    if(signum == SIGUSR1){
        log_info(logger, "SIGUSR1");
        calcular_md5_bitmaps_actuales();
        notificar_sabotaje();
    }
}

//[1,1,1,1] antes sabotaje. md5 viejo
//[1,1,1,1,1,1] sabotaje. md5 sabotaje.
//[1,1,1,1,1] con bitacoras nuevas
/*
1. [1,1,1,1] antes sabotaje. bitmap_memoria. Oxigeno:Blocks=[0,1]  Bitacora_tripu1:blocks=[2,3]. [1,1,1,1] bitmap_disco
2. despues de saboteo: [1,1,1,1] antes sabotaje. bitmap_memoria (superbloque.bitmap). Oxigeno:Blocks=[0,1]  Bitacora_tripu1:blocks=[2,3].
    [1,1,1,1,1,1] bitmap_disco
3. tiro signal->dentro de signal calculo: md5_global_viejo_real (con superbloque.bitmap). calculo: md5_global_saboteado_actual_de disco: con [1,1,1,1,1,1]
4. carga bitacoras: superbloque.bitmap queda [1,1,1,1,1,1].  Oxigeno:Blocks=[0,1]  Bitacora_tripu1:blocks=[2,3,4,5]
5. cuando llega a la funcion delm sabotaje de bitmap: compara los md5.
6.A. Si dan distinto->hubo sabotaje: log_info
6.B. Si dan igual-> no hubo sabotaje: log_info.
 */

void calcular_md5_bitmaps_actuales()
{
	cadena_calcular_md5(superbloque.bitmap, bitmap_size, md5_bitmap_real_inicial);
	char* bitmap_actual = superbloque_obtener_bitmap_de_archivo();
	cadena_calcular_md5(bitmap_actual, bitmap_size, md5_bitmap_actual);
}
//0x9b35c98

char* superbloque_obtener_bitmap_de_archivo()
{
	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
	int desplazamiento = 2 * sizeof(uint32_t);
	char* bitmap_obtenido = malloc(bitmap_size);

	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, bitmap_obtenido, bitmap_size);

	close(superbloque_fd);
	return bitmap_obtenido;
}

//Envia al Discordiador a traves del socket abierto socket_cliente_sabotaje la posicion del sabotaje que debe ser atendida
void notificar_sabotaje()
{
	char** posiciones_sabotaje_array = configuracion.posiciones_sabotaje;
	char* posicion_sabotaje = posiciones_sabotaje_array[sabotajes_indice++];

	enviar_mensaje(posicion_sabotaje, socket_cliente_sabotaje);
}

////////////////////////////FIN DE FUNCIONES NECESARIAS PARA ATENDER LA SEÑAL SIGUSR1////////////////////

////////////////////////////FUNCIONES NECESARIAS PARA LA COMUNICACION CON DISCORDIADOR///////////////////

void iniciar_servidor()
{
	log_info(logger, "Servidor iniciando");

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(configuracion.ip, configuracion.puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        int activado = 1;
        setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)); //para que si se cierra sin el close no haya que esperar

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	log_info(logger, "Servidor Mongo encendido");


	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = 0;


	int hilo;
	while(variable_servidor != 0)
	{
		socket_cliente = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);
log_info(logger, "post socket_cliente");
		if(socket_cliente>0)
		{
			hilo ++ ;
			log_info(logger, "Estableciendo conexión desde %d", dir_cliente.sin_port);
			log_info(logger, "Creando hilo");

			pthread_t hilo_cliente = (char)hilo;
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente,(void*)socket_cliente);
			pthread_detach(hilo_cliente);
		}
	}
	//printf("me fui");//log_destroy(logger);
}

void iniciar_servidor2()
{
    log_info(logger, "Servidor iniciando 2");

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(configuracion.ip, "5003", &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        int activado = 1;
        setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)); //para que si se cierra sin el close no haya que esperar

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

    listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    log_info(logger, "Servidor Mongo2 encendido");


    struct sockaddr_in dir_cliente;
    int tam_direccion = sizeof(struct sockaddr_in);

    socket_cliente_sabotaje = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);

    //printf("me fui");//log_destroy(logger);
}

int funcion_cliente(int socket_cliente)
{
	log_debug(logger, "Se ingresa a funcion_cliente");
	log_info(logger, "Se conecto el socket %d", socket_cliente);

	int codigo_operacion;
	t_list* lista;

	while(1)
	{
		codigo_operacion = recibir_operacion(socket_cliente);

		switch(codigo_operacion)
		{
			case REALIZAR_TAREA:
				//log_info(logger, "case realizar tarea");
				lista = recibir_paquete(socket_cliente);
				//log_info(logger, "paquete recibido");

				char* nombre_tarea = list_get(lista, 0);
				char* cantidad_str = list_get(lista, 1);

				if(recurso_realizar_tarea(nombre_tarea, cantidad_str) == OK)
				{
					//log_debug(logger, "Ya realice \"%s %s\" y estoy por enviar el ok", nombre_tarea, cantidad_str);
					enviar_ok(socket_cliente);
				}
				else
				{
					log_error(logger, "SE SUPONE NO TENDRIA QUE LLEGAR HASTA ACA! En funcion_cliente case REALIZAR_TAREA."
							" La tarea recibida \"%s %s\" no genera cambios en el File System", nombre_tarea, cantidad_str);
				}

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);
				break;

			case CARGAR_BITACORA:
				lista = recibir_paquete(socket_cliente);

				char* tid_str = list_get(lista, 0);
				char* mensaje = list_get(lista, 1);

				sem_wait(&MUTEX_BITACORA);

				if(cargar_bitacora(tid_str, mensaje) == OK)
				{
					sem_post(&MUTEX_BITACORA);
					//log_debug(logger, "Tripu %s ya cargue bitacora: \"%s\" ", tid_str, mensaje);
					enviar_ok(socket_cliente);
				}
				else
				{
					sem_post(&MUTEX_BITACORA);
					log_error(logger, "SE SUPONE NO TENDRIA QUE LLEGAR HASTA ACA! En funcion_cliente case CARGAR_BITACORA. La operacion "
							"para el tripulante %s con el mensaje \"%s\" no genera cambios en el File System", tid_str, mensaje);
				}

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);
				break;

			case OBTENER_BITACORA:
				lista = recibir_paquete(socket_cliente);
				int int_tid = atoi(list_get(lista, 0));

				sem_wait(&MUTEX_BITACORA);
				char* bitacora_a_enviar = bitacora_obtener_mensajes_de_tripulante(int_tid);
				sem_post(&MUTEX_BITACORA);

				enviar_mensaje(bitacora_a_enviar, socket_cliente);

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);
				free(bitacora_a_enviar);
				break;

			case INICIAR_FSCK:
				if(fsck_iniciar() == OK)
				{
					log_debug(logger, "Ya ejecute las instrucciones del fsck");
					enviar_ok(socket_cliente);
				}
				else
				{
					log_error(logger, "No pude realizar correctamente las instrucciones del FSCK --NO DEBERIA LLEGAR ACA --en funcion_cliente()");
				}
				break;

			case FIN_HILO_TRIPULANTE:
				log_info(logger, "fin de hilo tripulante");
				close(socket_cliente);
				return 0;
				break;

			case FIN:
				log_error(logger, "CASE FIN --- El discordiador finalizo el programa. Terminando servidor");
				variable_servidor = 0;
				//shutdown(socket_servidor, SHUT_RD);//A VECES ANDA Y A VECES NO
				close(socket_servidor);
				close(socket_cliente);

				log_debug(logger, "En el case FIN despues de cerrar los sockets");//BORRAR--VERIFICAR SI ES FUNCIONAL
				log_destroy(logger);
				return EXIT_FAILURE;

			case -1:
				log_info(logger, "El cliente se desconecto.");
				close(socket_cliente);
				return EXIT_FAILURE;
			default:
				log_error(logger, "A esa operacion %d no la tengo ---NO DEBERIA HABER LLEGADO ACA!!!", codigo_operacion);
		}
	}
}

//Devuelve el codigo de operacion recibido traves del socket abierto socket_clietne
int recibir_operacion(int socket_cliente)
{
    int codigo_operacion;
    int respuesta_recv = recv(socket_cliente, &codigo_operacion, sizeof(uint32_t), MSG_WAITALL);

    if(respuesta_recv == 0)
    {
        log_error(logger, "recv devuelve 0 en recibir_operacion");
        codigo_operacion = -1;
    }
    else if(respuesta_recv == -1)
    {
    	log_error(logger, "recv devuelve -1 en recibir_operacion");
    }

    //log_debug(logger, "Codigo de operacion recibido: cod_op = %d", codigo_operacion);
    return codigo_operacion;
}

//Recibe un paquete a traves del socket abierto socket_cliente, guarda sus valores en una lista y devuelve el puntero a esta
t_list* recibir_paquete(int socket_cliente)
{
	uint32_t size;
	uint32_t desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	uint32_t tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

//Recibe el tamanio y el contenido de un buffer a traves del socket abierto socket_cliente,
//retornando el tamanio en el parametro size y devolviendo el puntero al buffer
void* recibir_buffer(uint32_t* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

//Envia el mensaje "ok" a traves del socket abierto _socket
void enviar_ok(int _socket)
{
	char* mensaje = "ok";
	enviar_mensaje(mensaje, _socket);
}

//Envia al socket abierto indicado por socket_cliente el mensaje indicado por mensaje serializado
void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + sizeof(uint32_t);

	void* a_enviar = malloc(bytes);
	int desplazamiento = 0;

	memcpy(a_enviar + desplazamiento, &(paquete->buffer->size), sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
	memcpy(a_enviar + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

//Elimina el paquete indicado por paquete incluyendo toda su estructura interna
void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

//Libera el espacio indicado por el puntero contenido
void destruir_lista_paquete(char* contenido)
{
	free(contenido);
}

////////////////////////////FIN DE FUNCIONES NECESARIAS PARA LA COMUNICACION CON DISCORDIADOR///////////////////


////////////////////////FUNCIONES NECESARIAS PARA REALIZAR TAREAS///////////////////////

//Realiza la tarea recibida con la cantidad recibida actualizando los archivos bajo demanda
int recurso_realizar_tarea(char* nombre_tarea, char* cantidad_str)
{
	//log_debug(logger, "Se ingresa a recurso_realizar_tarea con \"%s %s\"", nombre_tarea, cantidad_str);

	int cantidad = atoi(cantidad_str);
	t_tarea* tarea = tarea_buscar_accion_y_recurso(nombre_tarea);

	int tarea_realizada;

	if(tarea != NULL && cantidad >= 0)
	{
		t_recurso_data* recurso_data = &lista_recursos[tarea->codigo_recurso];
		int actualizacion_de_archivos_requerida = 0;

		switch(tarea->codigo_accion)
		{
			case GENERAR:
				actualizacion_de_archivos_requerida = recurso_generar_cantidad(recurso_data, cantidad);
				break;
			case CONSUMIR:
				actualizacion_de_archivos_requerida = recurso_consumir_cantidad(recurso_data, cantidad);
				break;
			case DESCARTAR:
				actualizacion_de_archivos_requerida = recurso_descartar(recurso_data, cantidad);
				break;
			default:
				log_error(logger, "Algo fallo, no deberia haber llegado hasta aca en recurso_realizar_tarea - presumible error en array de tareas");
				free(tarea);
				return ERROR;
		}

		if(actualizacion_de_archivos_requerida)
		{
			superbloque_actualizar_bitmap_en_archivo();
			//blocks_actualizar_archivo();
			log_info(logger, "Se actualizaron los archivos de superbloque y blocks debido a la realizacion de \"%s %d\"", tarea->nombre, cantidad);
		}
		else
		{
			log_debug(logger, "La tarea \"%s %s\" no precisa actualizar ni Blocks.ims ni SuperBloque.ims", nombre_tarea, cantidad_str);
		}
		free(tarea);
		return OK;
	}
	else
	{
		log_warning(logger, "SE SUPONE NO TENDRIA QUE LLEGAR ACA EN LAS PRUEBAS REALES YA QUE NO NOS LLEGARIA DESDE EL DISCORDIADOR UNA TAREA"
				" NO VALIDA NI UNA CANTIDAD NEGATIVA (AGUS NO TE ASUSTES QUE SOLO ESTOY PROBANDO EL WARNING) -- No se actualizaron los archivos"
				" de superbloque y blocks ya que la tarea \"%s %s\" no realizo modificaciones", nombre_tarea, cantidad_str);
		return ERROR;
	}
}

//Devuelve un puntero a la estructura de tarea correspondiente a tarea_str. En caso de no encontrarla devuelve NULL
t_tarea* tarea_buscar_accion_y_recurso(char* tarea_str)
{
	t_tarea* tarea = NULL;

	int cantidad_tareas = tarea_cantidad_disponibles();
	for(int i = 0; i < cantidad_tareas; i++)
	{
		if(strcmp(tarea_str, lista_tareas_disponibles[i].nombre) == 0)
		{
			tarea = malloc(sizeof(t_tarea));
			memcpy(tarea, &lista_tareas_disponibles[i], sizeof(t_tarea));
			//log_debug(logger, "Se encontro %s con accion_code %d y recurso_code %d", tarea_str, tarea->codigo_accion, tarea->codigo_recurso);
			break;
		}
	}

	if(tarea == NULL)
	{
		log_warning(logger, "No se encontro la tarea \"%s\" en el array de tareas disponibles --NO DEBERIA HABER ENTRADO ACA", tarea_str);
	}

	//log_debug(logger, "Saliendo de tarea_buscar_accion_y_recurso");
	return tarea;
}

//Devuelve la cantidad de tareas disponibles para realizarse
int tarea_cantidad_disponibles()
{
	return (sizeof(lista_tareas_disponibles) / sizeof(t_tarea));
}

//Actualiza la metadata del recurso en memoria al archivo. Si este no existe lo crea con los valores actualizados
void recurso_actualizar_archivo(t_recurso_data* recurso_data)
{
	//log_debug(logger, "Se ingresa a recurso_actualizar_archivo de recurso %s", recurso_data->nombre);

	/*switch(recurso_data->caracter_llenado)
	{
		case 'O':
			sem_wait(&MUTEX_OXIGENO_IMS);
			break;
		case 'C':
			sem_wait(&MUTEX_COMIDA_IMS);
			break;
		case 'B':
			sem_wait(&MUTEX_BASURA_IMS);
			break;
		default:
			log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
	}*/

	if(utils_existe_en_disco(recurso_data->ruta_completa) == 0)
	{
		utils_crear_archivo(recurso_data->ruta_completa);
	}

	t_config* recurso_md = config_create(recurso_data->ruta_completa);

	char* size_str = string_itoa(recurso_data->metadata->size);
	char* block_count_str = string_itoa(recurso_data->metadata->block_count);

	config_set_value(recurso_md, "SIZE", size_str);
	config_set_value(recurso_md, "BLOCK_COUNT", block_count_str);
	config_set_value(recurso_md, "BLOCKS", recurso_data->metadata->blocks);
	config_set_value(recurso_md, "CARACTER_LLENADO", recurso_data->metadata->caracter_llenado);
	config_set_value(recurso_md, "MD5_ARCHIVO", recurso_data->metadata->md5_archivo);

	free(size_str);
	free(block_count_str);

	config_save(recurso_md);

	log_debug(logger, "Valores persistidos: SIZE %d  B_C %d  BS %s  C_LL %s MD5 %s", config_get_int_value(recurso_md, "SIZE"),
			config_get_int_value(recurso_md, "BLOCK_COUNT"), config_get_string_value(recurso_md, "BLOCKS"),
			config_get_string_value(recurso_md, "CARACTER_LLENADO"), config_get_string_value(recurso_md, "MD5_ARCHIVO"));
	config_destroy(recurso_md);

/*	switch(recurso_data->caracter_llenado)
	{
		case 'O':
			sem_post(&MUTEX_OXIGENO_IMS);
			break;
		case 'C':
			sem_post(&MUTEX_COMIDA_IMS);
			break;
		case 'B':
			sem_post(&MUTEX_BASURA_IMS);
			break;
		default:
			log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
	}*/

	log_info(logger, "Se actualizo metadata de %s de memoria a archivo %s", recurso_data->nombre, recurso_data->ruta_completa);
}

//Actualiza en el archivo SuperBloque.ims el campo bitmp utilizando write
void superbloque_actualizar_bitmap_en_archivo()
{
	sem_wait(&MUTEX_SUPERBLOQUE_PATH);
	sem_wait(&MUTEX_SUPERBLOQUE_BITMAP);

	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
	lseek(superbloque_fd, 2 * sizeof(uint32_t), SEEK_SET);
	write(superbloque_fd, superbloque.bitmap, bitmap_size);
	close(superbloque_fd);

	sem_post(&MUTEX_SUPERBLOQUE_BITMAP);
	sem_post(&MUTEX_SUPERBLOQUE_PATH);
	//log_debug(logger, "SuperBloque.ims actualizado (BITMAP)");
}

//Sincroniza de manera forzosa el archivo de Blocks.ims con el espacio de memoria mapeado
void blocks_actualizar_archivo_con_delay(int _delay)
{
	sleep(_delay);//log_debug(logger, "I-Se ingresa a blocks_actualizar_archivo");
	msync(blocks_address, blocks_size, MS_SYNC);
	log_debug(logger, "Blocks.ims actualizado");
}

void blocks_actualizar_archivo()
{
	//log_debug(logger, "I-Se ingresa a blocks_actualizar_archivo");
	msync(blocks_address, blocks_size, MS_SYNC);
	log_debug(logger, "Blocks.ims actualizado");
}


////////////COMIENZO GENERAR

//Genera la cantidad dada por cantidad del recurso dado por codigo_recurso
int recurso_generar_cantidad(t_recurso_data* recurso_data, int cantidad)
{
	//log_debug(logger, "Se ingresa a recurso_generar_cantidad para el recurso %s y la cantidad %d", recurso_data->nombre, cantidad);
	int actualizacion_de_archivos_requerida = 0;

	if(cantidad > 0)
	{
		switch(recurso_data->caracter_llenado)
		{
			case 'O':
				sem_wait(&MUTEX_OXIGENO_MD);
				break;
			case 'C':
				sem_wait(&MUTEX_COMIDA_MD);
				break;
			case 'B':
				sem_wait(&MUTEX_BASURA_MD);
				break;
			default:
				log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
		}

		recurso_validar_existencia_metadata_en_memoria(recurso_data);

		if(utils_existe_en_disco(recurso_data->ruta_completa) == 0)
		{
			utils_crear_archivo(recurso_data->ruta_completa);
			recurso_asignar_caracter_de_llenado_a_archivo(recurso_data);
			log_debug(logger, "Se asigno el caracter de llenado %c en el recien creado archivo %s",
					recurso_data->caracter_llenado, recurso_data->nombre_archivo);
		}

		metadata_generar_cantidad(recurso_data->metadata, cantidad);
		recurso_actualizar_archivo(recurso_data);
		actualizacion_de_archivos_requerida = 1;

		switch(recurso_data->caracter_llenado)
		{
			case 'O':
				sem_post(&MUTEX_OXIGENO_MD);
				break;
			case 'C':
				sem_post(&MUTEX_COMIDA_MD);
				break;
			case 'B':
				sem_post(&MUTEX_BASURA_MD);
				break;
			default:
				log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
		}
	}

	//log_debug(logger, "Finaliza recurso_generar_cantidad(%s, %d)", recurso_data->nombre, cantidad);
	return actualizacion_de_archivos_requerida;
}

void recurso_asignar_caracter_de_llenado_a_archivo(t_recurso_data* recurso_data)
{
	t_config* recurso_config = config_create(recurso_data->ruta_completa);

	char* caracter_str = string_from_format("%c", recurso_data->caracter_llenado);
	config_set_value(recurso_config, "CARACTER_LLENADO", caracter_str);
	free(caracter_str);

	config_save(recurso_config);
	config_destroy(recurso_config);
}

//Genera el recurso de la metadata dada guardando los cambios en esta misma tanto como en la copia de blocks en memoria
void metadata_generar_cantidad(t_recurso_md* recurso_md, int cantidad)
{
	//log_debug(logger, "Info-Entro a metadata_generar_cantidad con C_LL %s y cantidad %d", recurso_md->caracter_llenado, cantidad);
	//log_debug(logger, "En metadata_generar_cantidad: cantidad %d SIZE %d, BLOCKS %s",
			//cantidad, recurso_md->size, recurso_md->blocks);
	int bloque_a_cargar;
	if(metadata_tiene_espacio_en_ultimo_bloque(recurso_md))
	{
		bloque_a_cargar = metadata_ultimo_bloque_usado(recurso_md);
		metadata_cargar_parcialmente_bloque(recurso_md, &cantidad, bloque_a_cargar);
	}
	while(cantidad > 0)
	{
		bloque_a_cargar = superbloque_obtener_bloque_libre();
		if(bloque_a_cargar >= 0)
		{
			metadata_agregar_bloque_a_lista_de_blocks(recurso_md, bloque_a_cargar);
		}
		else
		{
			break;
		}

		if(cantidad >= superbloque.block_size)
		{
			metadata_cargar_bloque_completo(recurso_md, bloque_a_cargar);
			cantidad -= superbloque.block_size;
		}
		else
		{
			metadata_cargar_parcialmente_bloque(recurso_md, &cantidad, bloque_a_cargar);
		}
	}
	metadata_actualizar_md5(recurso_md);

	if(cantidad == 0)
	{
		log_debug(logger, "Se genero la cantidad solicitada teniendo finalmente el recurso con C_LL %s, SIZE %d, BLOCKS %s y el MD5 %s",
				recurso_md->caracter_llenado, recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
	}
	else
	{
		log_error(logger, "No se llego a generar completamente la cantidad solicitada debido a falta de bloques libres, quedaron sin cargar %d",
				cantidad);
	}
	//log_debug(logger, "Saliendo de metadata_generar_cantidad con C_LL %s, SIZE %d, BLOCKS %s y el MD5 %s", recurso_md->caracter_llenado,
				//recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
}


//Verifica si el ultimo bloque usado por el recurso tiene espacio disponible
int metadata_tiene_espacio_en_ultimo_bloque(t_recurso_md* recurso_md)
{
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	//log_debug(logger, "En metadata_tiene_espacio_en_ultimo_bloque con SIZE %d, BLOCKS %s y superbloque.block_size %d (1 SI, 0 NO): %d",
			//recurso_md->size, recurso_md->blocks, superbloque.block_size, recurso_md->size < cantidad_bloques * superbloque.block_size);
	return (recurso_md->size < cantidad_bloques * superbloque.block_size);
}
//Devuelve el ultimo bloque registrado en la lista de Blocks del metadata del recurso
int metadata_ultimo_bloque_usado(t_recurso_md* recurso_md)
{
	//log_debug(logger, "IO-Ingreso a metadata_ultimo_bloque_usado con BLOCKS %s", recurso_md->blocks);
	int ultimo_bloque = cadena_ultimo_entero_en_lista_de_enteros(recurso_md->blocks);
	return ultimo_bloque;
}

//Carga recursos en el bloque dado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_parcialmente_bloque(t_recurso_md* recurso_md, int* cantidad, int bloque)
{
	//log_debug(logger, "I-Se ingresa a metadata_cargar_parcialmente_en_bloque con caracter %s, cantidad %d y bloque %d", recurso_md->caracter_llenado, *cantidad, bloque);

	int posicion_en_bloque = recurso_md->size % superbloque.block_size;
	int posicion_inicio_bloque = bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque; //esta ok sumar un puntero con 2 ints?

	log_debug(logger, "Posicion_en_bloque %d, posicion_inicio_bloque %d posicion block_addres %p posicion absoluta %p",
			posicion_en_bloque, posicion_inicio_bloque, blocks_address, posicion_absoluta);

	char caracter_llenado = recurso_md->caracter_llenado[0];

	while(posicion_en_bloque < superbloque.block_size && (*cantidad) > 0)
	{
		*posicion_absoluta = caracter_llenado;

		(*cantidad)--;
		recurso_md->size++;
		posicion_absoluta++;
		posicion_en_bloque++;
	}
	//Centinela al momento de cargar en bloque//Necesario??
	if(posicion_en_bloque < superbloque.block_size)
	{
		*posicion_absoluta = 0;
	}
	log_debug(logger, "Se cargo en bloque %d con caracter %s quedando por cargar %d recurso(s)", bloque, recurso_md->caracter_llenado, *cantidad);
}

//Busca y devuelve el primer bloque libre en el bitmap, si no existiera finaliza con error
int superbloque_obtener_bloque_libre()
{
	//log_debug(logger, "I-Se ingresa a superbloque_obtener_bloque_libre");

	sem_wait(&MUTEX_SUPERBLOQUE_BITMAP);

	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, bitmap_size, LSB_FIRST);
	int posicion;
	int posicion_maxima = superbloque.blocks - 1;

	for(posicion = 0; posicion < posicion_maxima && bitarray_test_bit(bitmap, posicion); posicion++);

	if(bitarray_test_bit(bitmap, posicion))
	{
		log_error(logger, "O-No hay bloques libres para almacenar nuevos recursos en el bitmap del superbloque");//. Se finaliza por falta de espacio en los bloques"); exit(EXIT_FAILURE);
		posicion = ERROR; //VALOR DE ERROR
	}
	else
	{
		bitarray_set_bit(bitmap, posicion);
		//log_debug(logger, "O-Bloque libre obtenido %d", posicion);
	}

	bitarray_destroy(bitmap);

	sem_post(&MUTEX_SUPERBLOQUE_BITMAP);
	return posicion;
}

//Agrega a la lista de blocks de la metadata un bloque nuevo obtenido a traves del bitmap del superbloque y a su vez lo devuelve
void metadata_agregar_bloque_a_lista_de_blocks(t_recurso_md* recurso_md, int bloque)
{
	//log_debug(logger, "I-Se ingresa a metadata_agregar_bloque_a_lista_de_blocks con blocks %s, block_count %d y bloque a agregar %d", recurso_md->blocks, recurso_md->block_count, bloque);
	cadena_agregar_entero_a_lista_de_enteros(&recurso_md->blocks, bloque);
	recurso_md->block_count++;
	log_debug(logger, "Bloque %d agregado a blocks quedando BLOCK_COUNT %d y BLOCKS %s", bloque, recurso_md->block_count, recurso_md->blocks);
}

//Carga totalmente el bloque dado con el recurso dado
void metadata_cargar_bloque_completo(t_recurso_md* recurso_md, int bloque)
{
	//log_debug(logger, "I-Se ingresa a recurso_cargar_bloque_completo con caracter %s y bloque %d", recurso_md->caracter_llenado, bloque);

	int posicion_inicio_bloque = bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque; //esta ok sumar un puntero con 2 ints?

	log_debug(logger, "Posicion_inicio_bloque %d posicion absoluta %p o %d", posicion_inicio_bloque, posicion_absoluta, posicion_absoluta);

	char caracter_llenado = recurso_md->caracter_llenado[0];

	memset(posicion_absoluta, caracter_llenado, superbloque.block_size);

	recurso_md->size += superbloque.block_size;

	log_debug(logger, "O-Se cargo el bloque completo %d con caracter %s", bloque, recurso_md->caracter_llenado);
}

//Calcula y actualiza el valor del md5 del archivo teniendo en cuenta el conjunto de bloques que ocupa el recurso en el archivo de Blocks.MD
void metadata_actualizar_md5(t_recurso_md* recurso_md)
{
	//log_debug(logger, "I-Se ingresa a metadata_actualizar_md5");
	if(recurso_md->size > 0)
	{
		char* concatenado = blocks_obtener_concatenado_de_recurso(recurso_md);
		char md5_str[33];
		cadena_calcular_md5(concatenado, recurso_md->size, md5_str);
		strcpy(recurso_md->md5_archivo, md5_str);
		free(concatenado);
	}
	else
	{
		strcpy(recurso_md->md5_archivo, "d41d8cd98f00b204e9800998ecf8427e");
	}
	//log_debug(logger, "MD5 en metadata actualizado a %s", recurso_md->md5_archivo);
}


//Obtiene el conjunto de bloques concatenado que el recurso esta ocupando en el archivo de Blocks.ims
char* blocks_obtener_concatenado_de_recurso(t_recurso_md* recurso_md)
{
	//log_debug(logger, "I-Se ingresa a blocks_obtener_concatenado_de_recurso");
	char* concatenado = malloc(recurso_md->size);

	if(concatenado != NULL)
	{
		int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
		int indice_ultimo_bloque = cantidad_bloques - 1; //Indice en base a la cantidad de bloques de la lista en el recurso
		int bloque;//Numero de bloque en base al total de bloques del sistema
		char** lista_bloques = string_get_string_as_array(recurso_md->blocks);
		char* posicion_desde; //Posicion de la copia de Blocks.ims desde la que arranca el bloque que se copiara
		char* posicion_hacia = concatenado; //Posicion del espacio de memoria donde se almacenara cada bloque con memcpy

		for(int i = 0; i < indice_ultimo_bloque; i++)
		{
			bloque = atoi(lista_bloques[i]);
			posicion_desde = blocks_address + bloque * superbloque.block_size;
			memcpy(posicion_hacia, posicion_desde, superbloque.block_size);
			posicion_hacia += superbloque.block_size;
		}

		bloque = atoi(lista_bloques[indice_ultimo_bloque]);
		posicion_desde = blocks_address + bloque * superbloque.block_size;
		int cantidad_en_ultimo_bloque = recurso_md->size % superbloque.block_size;
		//log_info(logger,"cantidad_en_ultimo_bloque %d", cantidad_en_ultimo_bloque);
		memcpy(posicion_hacia, posicion_desde, cantidad_en_ultimo_bloque);
		cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);
		//log_debug(logger, "Total de caracteres del recurso concatenado en direccion de memoria %p", concatenado);
	}
	else
	{
		log_error(logger, "No hay memoria suficiente como para obtener el concatendao de bloques del recurso con C_LL %s", recurso_md->caracter_llenado);
	}

	return concatenado;
}

////////////FIN GENERAR

/////////////COMIENZO CONSUMIR

//Consume la cantidad dada por cantidad del recurso dado por codigo_recurso
int recurso_consumir_cantidad(t_recurso_data* recurso_data, int cantidad)
{
	//log_debug(logger, "Se ingresa a recurso_consumir_cantidad para el recurso %s y la cantidad %d", recurso_data->nombre, cantidad);
	int actualizacion_de_archivos_requerida = 0;

	if(cantidad > 0)
	{
		switch(recurso_data->caracter_llenado)
		{
			case 'O':
				sem_wait(&MUTEX_OXIGENO_MD);
				break;
			case 'C':
				sem_wait(&MUTEX_COMIDA_MD);
				break;
			case 'B':
				sem_wait(&MUTEX_BASURA_MD);
				break;
			default:
				log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
		}

		if(utils_existe_en_disco(recurso_data->ruta_completa) == 0)
		{
			log_info(logger, "No existe el archivo %s para el cual queria consumirse", recurso_data->nombre_archivo);
			log_debug(logger, "Finaliza recurso_consumir_cantidad(%s, %d)", recurso_data->nombre, cantidad);
			return actualizacion_de_archivos_requerida;
		}

		recurso_validar_existencia_metadata_en_memoria(recurso_data);
		metadata_consumir_cantidad(recurso_data->metadata, cantidad);
		recurso_actualizar_archivo(recurso_data);
		actualizacion_de_archivos_requerida = 1;

		switch(recurso_data->caracter_llenado)
		{
			case 'O':
				sem_post(&MUTEX_OXIGENO_MD);
				break;
			case 'C':
				sem_post(&MUTEX_COMIDA_MD);
				break;
			case 'B':
				sem_post(&MUTEX_BASURA_MD);
				break;
			default:
				log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
		}

	}

	log_debug(logger, "Finaliza recurso_consumir_cantidad(%s, %d)", recurso_data->nombre, cantidad);
	return actualizacion_de_archivos_requerida;
}

//Carga la cantidad cantidad de los recursos del codigo_recurso en el ultimo bloque o en nuevos bloques que esten libres agregandolos a la lista de bloques
void metadata_consumir_cantidad(t_recurso_md* recurso_md, int cantidad)
{
	//pthread_mutex_lock(&mutex_blocks); //sector critico porque se usa superbloque.ims  y blocks.ims
	log_debug(logger, "Entro a metadata_consumir_cantidad con C_LL %s y cantidad %d", recurso_md->caracter_llenado, cantidad);
	log_debug(logger, "Valores de metadata antes de consumir recursos: SIZE %d, BLOCK_COUNT %d, BLOCKS %s y MD5 %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo);

	if(cantidad > recurso_md->size)
	{
		metadata_descartar_caracteres_existentes(recurso_md);
		log_info(logger, "Quisieron eliminarse mas caracteres \"%s\" de los existentes", recurso_md->caracter_llenado);
		return;
	}

	int cantidad_de_caracteres_en_ultimo_bloque =
			recurso_md->size % superbloque.block_size ? recurso_md->size % superbloque.block_size : superbloque.block_size;
	int cantidad_de_bloques_a_eliminar = 0;//int ultimo_bloque = cadena_ultimo_entero_en_lista_de_enteros(recurso_md->blocks);
	//char* bloques_a_eliminar = string_duplicate("[]");

	if(cantidad >= cantidad_de_caracteres_en_ultimo_bloque && cantidad_de_caracteres_en_ultimo_bloque != superbloque.block_size)
	{
		cantidad -= cantidad_de_caracteres_en_ultimo_bloque;
		recurso_md->size -= cantidad_de_caracteres_en_ultimo_bloque;
		cantidad_de_bloques_a_eliminar++;
	}

	while(cantidad >= superbloque.block_size)
	{
		cantidad -= superbloque.block_size;
		recurso_md->size -= superbloque.block_size;
		cantidad_de_bloques_a_eliminar++;
	}

	if(cantidad_de_bloques_a_eliminar > 0){
		char* bloques_a_eliminar = cadena_obtener_n_ultimos_enteros(recurso_md->blocks, cantidad_de_bloques_a_eliminar);
		metadata_liberar_bloques_en_bitmap_y_en_blocks(bloques_a_eliminar);
		cadena_eliminar_ultimos_n_elementos_de_elementos_separados_por_comas(recurso_md->blocks, cantidad_de_bloques_a_eliminar);
		recurso_md->block_count -= cantidad_de_bloques_a_eliminar;
		free(bloques_a_eliminar);
	}

	if(cantidad > 0)
	{
		metadata_consumir_en_ultimo_bloque(recurso_md, cantidad);
	}

	metadata_actualizar_md5(recurso_md);
	log_info(logger, "Se consumio la cantidad solicitada teniendo finalmente el recurso con C_LL %s, SIZE %d, BLOCKS %s y el MD5 %s",
			recurso_md->caracter_llenado, recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
	//log_debug(logger, "Saliendo de metadata_consumir_cantidad con C_LL %s, SIZE %d, BLOCKS %s y el MD5 %s", recurso_md->caracter_llenado,
			//recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
	//free(bloques_a_eliminar);
}

char* cadena_obtener_n_ultimos_enteros(char* cadena, int n)
{
	//log_debug(logger, "Ingreso a cadena_obtener_n_ultimos_entero(\"%s\", %d)", cadena, n);
	char** lista_bloques = string_get_string_as_array(cadena);
	int posicion_ultimo_bloque = cadena_cantidad_elementos_en_lista(cadena) - 1;

	char* ultimos_bloques = string_duplicate("[]");

	if(n == 1)
	{
		cadena_sacar_ultimo_caracter(ultimos_bloques);
		int bloque = atoi(lista_bloques[posicion_ultimo_bloque--]);
		string_append_with_format(&ultimos_bloques, "%d]", bloque);
	}

	for(int i = 1; i < n; i++)
	{
		cadena_sacar_ultimo_caracter(ultimos_bloques);
		int bloque = atoi(lista_bloques[posicion_ultimo_bloque--]);
		string_append_with_format(&ultimos_bloques, ",%d]", bloque);
	}

	log_debug(logger, "Saliendo de cadena_obtener_n_ultimos_entero(\"%s\", %d) se obtuvo %s", cadena, n, ultimos_bloques);

	/*int i = 0;
	while(lista_bloques[i] != NULL){
		free(lista_bloques[i]);
		i++;
	}
	free(lista_bloques[i]);
	free(lista_bloques);*/

	return ultimos_bloques;
}

void cadena_eliminar_ultimos_n_elementos_de_elementos_separados_por_comas(char* cadena, int n)//[0,1,2] n=1
{
	char* puntero_auxiliar = cadena;

	int coma_a_reemplazar = cadena_cantidad_elementos_en_lista(cadena) - n;

	if(coma_a_reemplazar > 0)
	{
		for(int i = 0; i < coma_a_reemplazar; i++)
		{
		/*if(*puntero_auxiliar) != ',')
		{
			printf("%c\n", (*puntero_auxiliar));
			puntero_auxiliar++;
		}
		else
		{*/
			puntero_auxiliar++;
			while((*puntero_auxiliar) != ',')
			{
				puntero_auxiliar++;
			}
		//}
		}
		*puntero_auxiliar++ = ']';
		*puntero_auxiliar = '\0';
	}
	else
	{
		strcpy(cadena, "[]");
	}

	log_debug(logger, "Saliendo de cadena_eliminar_ultimos_n_elementos_de_elementos_separados_por_comas se obtiene la cadena \"%s\"", cadena);
}

//Carga recursos en el ultimo bloque registrado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_consumir_en_ultimo_bloque(t_recurso_md* recurso_md, int cantidad)
{
	//log_debug(logger, "Se ingresa a recurso_consumir_en_ultimo_bloque con caracter %s y cantidad %d", recurso_md->caracter_llenado, cantidad);
	//Si el valor del nuevo_bloque no es el valor default es porque ese bloque es el nuevo ultimo de la lista y no necesita calcularse
	int ultimo_bloque = metadata_ultimo_bloque_usado(recurso_md);
	int posicion_a_consumir_en_bloque = (recurso_md->size - 1) % superbloque.block_size - (cantidad - 1);
	int posicion_inicio_bloque = ultimo_bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_a_consumir_en_bloque; //esta ok sumar un puntero con 2 ints?

	log_debug(logger, "Ultimo_bloque %d, posicion_a_consumir_en_bloque %d, posicion_inicio_bloque %d posicion absoluta %p",
			ultimo_bloque, posicion_a_consumir_en_bloque, posicion_inicio_bloque, posicion_absoluta);

	char caracter_llenado = recurso_md->caracter_llenado[0];

	//forma lenta -- borrar
	//int posicion_ultimo_caracter_en_bloque = (recurso_md->size - 1) % superbloque.block_size;
	/*while(posicion_a_consumir_en_bloque >= 0 && cantidad > 0)
	{
		*posicion_absoluta = '\0';
		cantidad--;
		recurso_md->size--;
		posicion_absoluta--;
		//posicion_ultimo_caracter_en_bloque--;
	}*/
	/*int posicion_ultimo_caracter_en_bloque = 1 + (recurso_md->size - 1) % superbloque.block_size;
	while(posicion_a_consumir_en_bloque > 0 && cantidad > 0)
	{
		*posicion_absoluta = '\0';
		cantidad--;
		recurso_md->size--;
		posicion_absoluta--;
		posicion_ultimo_caracter_en_bloque--;
	}*/
	if(posicion_a_consumir_en_bloque > 0 && cantidad > 0)
	{
		*posicion_absoluta = '\0';
		//recurso_md->size--;
		recurso_md->size -= cantidad;
	}

	log_debug(logger, "Se consumio en ultimo bloque el caracter %s quedando por consumir %d recurso(s) -- variante rapida", recurso_md->caracter_llenado, cantidad);
}

/////////////FIN CONSUMIR

/////////////COMIENZO DESCARTAR

//Consume la cantidad dada por cantidad del recurso dado por codigo_recurso
int recurso_descartar(t_recurso_data* recurso_data, int cantidad)
{
	//log_debug(logger, "Se ingresa a recurso_descartar para el recurso %s y la cantidad %d", recurso_data->nombre, cantidad);
	int actualizacion_de_archivos_requerida = 0;

	if(cantidad == 0)
	{
		switch(recurso_data->caracter_llenado)
		{
			case 'O':
				sem_wait(&MUTEX_OXIGENO_MD);
				break;
			case 'C':
				sem_wait(&MUTEX_COMIDA_MD);
				break;
			case 'B':
				sem_wait(&MUTEX_BASURA_MD);
				break;
			default:
				log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
		}

		recurso_validar_existencia_metadata_en_memoria(recurso_data);//BORRAR COMMENT -- invocacion valida si conviene/se supone que hay que trabajar to.do el tiempo con los datos del disco

		if(utils_existe_en_disco(recurso_data->ruta_completa) == 0)
		{
			log_info(logger, "No existe el archivo %s como para descartarse", recurso_data->nombre_archivo);
			log_debug(logger, "Saliendo de recurso_descartar(%s, %d)", recurso_data->nombre, cantidad);
			switch(recurso_data->caracter_llenado)
			{
				case 'O':
					sem_post(&MUTEX_OXIGENO_MD);
					break;
				case 'C':
					sem_post(&MUTEX_COMIDA_MD);
					break;
				case 'B':
					sem_post(&MUTEX_BASURA_MD);
					break;
				default:
					log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
			}
			return actualizacion_de_archivos_requerida;
		}

		if(metadata_tiene_caracteres_en_blocks(recurso_data->metadata))
		{
			metadata_descartar_caracteres_existentes(recurso_data->metadata);
			log_debug(logger, "Se seteo a los valores default la metadata en memoria del recurso %s", recurso_data->nombre);
			actualizacion_de_archivos_requerida = 1;
		}
		//recurso_descartar_archivo(recurso_data);
		switch(recurso_data->caracter_llenado)
		{
			case 'O':
				sem_post(&MUTEX_OXIGENO_MD);
				break;
			case 'C':
				sem_post(&MUTEX_COMIDA_MD);
				break;
			case 'B':
				sem_post(&MUTEX_BASURA_MD);
				break;
			default:
				log_error(logger, "NO TENDRIA QUE LLEGAR ACA!! (En recurso_actualizar_archivo");
		}

		recurso_descartar_archivo(recurso_data);//OK ACA? SANTI

	}
	else
	{
		log_info(logger, "La cantidad debería ser igual a 0 al querer descartar el recurso %s", recurso_data->nombre);
	}

	//log_debug(logger, "Saliendo de recurso_descartar(%s, %d)", recurso_data->nombre, cantidad);
	return actualizacion_de_archivos_requerida;
}

//Devuelve un valor distinto de 0 si en la lista de blocks del recurso este tiene caracteres en Blocks
int metadata_tiene_caracteres_en_blocks(t_recurso_md* recurso_md)
{
	return recurso_md->size;
}

//Descarta del recurso todos los caracteres y bloques, registrados en Blocks, en la metadata y en el superbloque
void metadata_descartar_caracteres_existentes(t_recurso_md* recurso_md)
{
	metadata_liberar_bloques_en_bitmap_y_en_blocks(recurso_md->blocks);
	free(recurso_md->blocks);
	metadata_setear_con_valores_default_en_memoria(recurso_md);
	log_debug(logger, "Se descartaron los caracteres \"%s\" que existian", recurso_md->caracter_llenado);
}

//Setea en el bitmap del superbloque las posiciones dadas por la cadena blocks a 0
void metadata_liberar_bloques_en_bitmap_y_en_blocks(char* blocks)
{
	//log_debug(logger, "I-Ingreso a superbloque_liberar_bloques_en_bitmap con BLOCKS %s", blocks);
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(blocks);
	int bloque;
	char** lista_bloques = string_get_string_as_array(blocks);

	sem_wait(&MUTEX_SUPERBLOQUE_BITMAP);

	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, bitmap_size, LSB_FIRST);

	for(int i = 0; i < cantidad_bloques; i++)
	{
		bloque = atoi(lista_bloques[i]);
		bitarray_clean_bit(bitmap, bloque);
		//log_debug(logger, "Bloque %d eliminado del bitmap del superbloque", bloque);
		blocks_eliminar_bloque(bloque);
	}

	bitarray_destroy(bitmap);

	sem_post(&MUTEX_SUPERBLOQUE_BITMAP);
	cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);
	log_debug(logger, "Los bloques %s fueron liberados en el bitmap del superbloque y en la copia de Blocks.ims", blocks);
}

//Registra un caracter de fin de cadena al comienzo de cada uno de los bloques incluidos en la lista de bloques
void blocks_eliminar_bloque(int bloque)
{
	//log_debug(logger, "I-blocks_eliminar_bloque con bloque %d", bloque);

	int posicion_inicio_bloque = bloque * superbloque.block_size;
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque;
	//*posicion_absoluta = '\0';
	memset(posicion_absoluta, 0, superbloque.block_size);

	//log_debug(logger, "Bloque %d eliminado de la copia de blocks - version rapida//version lenta", bloque);
}

//Verifica si el archivo del recurso_data existe en el disco y si lo encuentra lo elimina
void recurso_descartar_archivo(t_recurso_data* recurso_data)
{
	remove(recurso_data->ruta_completa);
	log_info(logger, "Se elimino el archivo de la metadata del recurso %s", recurso_data->nombre);
}

/////////////FIN DESCARTAR

////////////////////////FIN DE FUNCIONES NECESARIAS PARA REALIZAR TAREAS///////////////////////


////////////////////////FUNCIONES NECESARIAS PARA CARGAR BITACORAS/////////////////////////

int cargar_bitacora(char* tid_str, char* mensaje)
{
	//log_debug(logger, "Se recibe al tripulante %s y el mensaje \"%s\"", tid_str, mensaje);

	int tid = atoi(tid_str);

	t_bitacora_data* bitacora_data = bitacora_cargar_estructura_completa(tid);

	if(bitacora_guardar_log(bitacora_data, mensaje) == ERROR)
	{
		bitacora_borrar_estructura_completa(bitacora_data);
		return ERROR;
	}
	else
	{
		bitacora_borrar_estructura_completa(bitacora_data);
		return OK;
	}
}

//Carga la data de la bitacora correspondiente al tripulante indicado por el valor de tid
t_bitacora_data* bitacora_cargar_estructura_completa(int tid)
{
	t_bitacora_data* bitacora_data = malloc(sizeof(t_bitacora_data));

	bitacora_data->tid = tid;
	bitacora_data->ruta_completa = string_from_format("%s/Tripulante%d.ims", bitacoras_path, bitacora_data->tid);

	if(utils_existe_en_disco(bitacora_data->ruta_completa))
	{
		bitacora_levantar_metadata_de_archivo(bitacora_data);
	}
	else
	{
		bitacora_cargar_metadata_default(bitacora_data);

	}
	//log_debug(logger, "Salgo de bitacora_cargar_data para el tripulante %d", tid);
	return bitacora_data;
}

//Carga en memoria la metadata para el tripulante indicado en bitacora_data segun el archivo asociado en la misma data
void bitacora_levantar_metadata_de_archivo(t_bitacora_data* bitacora_data)
{
	bitacora_data->metadata = malloc(sizeof(t_bitacora_md));

	t_config* bitacora_config =  config_create(bitacora_data->ruta_completa);
	bitacora_data->metadata->size = config_get_int_value(bitacora_config, "SIZE");
	bitacora_data->metadata->blocks = string_duplicate(config_get_string_value(bitacora_config, "BLOCKS"));
	config_destroy(bitacora_config);

	//log_debug(logger, "Metadata de tripulante %d levantada en memoria con size = %d y blocks = %s", bitacora_data->tid,
			//bitacora_data->metadata->size, bitacora_data->metadata->blocks);
}

//Carga en memoria la metadata default para el tripulante indicado en bitacora_data
void bitacora_cargar_metadata_default(t_bitacora_data* bitacora_data)
{
	t_bitacora_md* metadata = malloc(sizeof(t_bitacora_md));

	metadata->size = 0;
	metadata->blocks = string_duplicate("[]");

	bitacora_data->metadata = metadata;

	//log_debug(logger, "Metadata de tripulante %d default cargada en memoria (size = %d y blocks = %s)", bitacora_data->tid,
			//bitacora_data->metadata->size, bitacora_data->metadata->blocks);
}

//Escribe la cadena log en el ultimo bloque escrito si tuviera espacio libre y si no en los blocks del superbloque que se encuentren libres
int bitacora_guardar_log(t_bitacora_data* bitacora_data, char* mensaje)
{
	//log_debug(logger, "Info-Se ingresa a recurso_generar_cantidad, se debe generar %d %s(s)", cantidad, recurso_data->nombre);
	if(strlen(mensaje) == 0)
	{
		log_error(logger, "No me llego ningun log para guardar");
		return ERROR;
	}

	//Se usa bitacora_md solo para facilitar la lectura del codigo
	t_bitacora_md* bitacora_md = bitacora_data->metadata;

	int bloque;
	char* mensaje_auxiliar = string_from_format("%s\n", mensaje);
	char* posicion_asignada = mensaje_auxiliar;

	if(bitacora_tiene_espacio_en_ultimo_bloque(bitacora_md))
	{
		bloque = cadena_ultimo_entero_en_lista_de_enteros(bitacora_md->blocks);
		bitacora_escribir_en_bloque(bitacora_md, &mensaje_auxiliar, bloque);
	}
	while(*mensaje_auxiliar != '\0')
	{
		bloque = superbloque_obtener_bloque_libre();

		if(bloque >= 0)
		{
			cadena_agregar_entero_a_lista_de_enteros(&bitacora_md->blocks, bloque);
			bitacora_escribir_en_bloque(bitacora_md, &mensaje_auxiliar, bloque);
		}
		else
		{
			log_error(logger, "No se puede escribir el log de la bitacora por no haber bloques libres (no deberia llegar aca)");
			return ERROR;
		}
	}

	free(posicion_asignada);

	superbloque_actualizar_bitmap_en_archivo();
	//blocks_actualizar_archivo();
	bitacora_actualizar_archivo(bitacora_data);

	return OK;
}

//Verifica si el ultimo bloque usado por el recurso tiene espacio disponible
int bitacora_tiene_espacio_en_ultimo_bloque(t_bitacora_md* bitacora_md)
{
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(bitacora_md->blocks);
	//log_debug(logger, "En bitacora_tiene_espacio_en_ultimo_bloque con SIZE %d, BLOCKS %s y superbloque.block_size %d (1 SI, 0 NO): %d",
			//bitacora_md->size, bitacora_md->blocks, superbloque.block_size, bitacora_md->size < cantidad_bloques * superbloque.block_size);
	return (bitacora_md->size < cantidad_bloques * superbloque.block_size);
}

//Escribe el log recibido para la bitacora dada por bitacora_md en el bloque indicado, hasta escribirlo completo o hasta que el bloque quede sin espacio
void bitacora_escribir_en_bloque(t_bitacora_md* bitacora_md, char** mensaje, int bloque)
{
	int posicion_en_bloque = bitacora_md->size % superbloque.block_size;
	int posicion_inicio_bloque = bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque;

	//log_debug(logger, "Posicion_en_bloque %d, posicion_inicio_bloque %d posicion block_addres %p posicion absoluta %p",
			//posicion_en_bloque, posicion_inicio_bloque, blocks_address, posicion_absoluta);
	while(posicion_en_bloque < superbloque.block_size && **mensaje != '\0')
	{
		*posicion_absoluta = **mensaje;

		bitacora_md->size++;
		(*mensaje)++;
		posicion_absoluta++;
		posicion_en_bloque++;
	}

	//log_debug(logger, "Al salir de bitacora_escribir_en_bloque posicion_en_bloque %d, posicion_inicio_bloque %d posicion block_addres %p "
			//"posicion absoluta %p proximo caracter a escribir en codigo ascii %d", posicion_en_bloque, posicion_inicio_bloque, blocks_address,
			//posicion_absoluta, **mensaje);
	//log_debug(logger, "Saliendo bitacora_escribir_en_bloque");
}

//Actualiza el archivo de la bitacora con la informacion que ya tiene en memoria
void bitacora_actualizar_archivo(t_bitacora_data* bitacora_data)
{
	if(utils_existe_en_disco(bitacora_data->ruta_completa) == 0)
	{
		utils_crear_archivo(bitacora_data->ruta_completa);
	}

	t_config* bitacora_config = config_create(bitacora_data->ruta_completa);
	char* size_str = string_itoa(bitacora_data->metadata->size);
	config_set_value(bitacora_config, "SIZE", size_str);
	config_set_value(bitacora_config, "BLOCKS", bitacora_data->metadata->blocks);
	free(size_str);

	config_save(bitacora_config);
	config_destroy(bitacora_config);
	//log_debug(logger, "%s actualizado para tripulante %d", bitacora_data->ruta_completa, bitacora_data->tid);
}

void bitacora_borrar_estructura_completa(t_bitacora_data* bitacora_data)
{
	free(bitacora_data->metadata->blocks);
	free(bitacora_data->metadata);
	free(bitacora_data->ruta_completa);
	free(bitacora_data);
}

////////////FIN DE FUNCIONES NECESARIAS PARA CARGAR BITACORAS//////////////////////////


/////////////////////////////////////FUNCIONES NECESARIAS PARA FSCK/////////////////////////////////////////////////////////

//TODO ver si lo que se tiene en memoria es valido o no para verificar sabotajes (y repararlos)

//Inicia la secuencia de chequeos y reparaciones del File System Catastrophe Killer
int fsck_iniciar()
{
	log_debug(logger, "Se inicia fsck");
	//munmap(blocks_address, blocks_size);
	//blocks_mapear_archivo_a_memoria();

	blocks_actualizar_archivo();
	sem_wait(&MUTEX_BLOCKS);
	//fsck_chequeo_de_sabotajes_en_superbloque();
	superbloque_validar_integridad_cantidad_de_bloques();
	fsck_chequeo_de_sabotajes_en_files();
	superbloque_validar_integridad_bitmap();

	sem_post(&MUTEX_BLOCKS);
	log_debug(logger, "Se finaliza fsck");
	return OK;
}

//Realiza los chequeos propios del superbloque (en campos blocks y bitmap)
void fsck_chequeo_de_sabotajes_en_superbloque()
{
	log_debug(logger, "Se inicia fsck_chequeo_de_sabotajes_en_superbloque()");
	superbloque_validar_integridad_cantidad_de_bloques();
	//utils_esperar_a_usuario();
	superbloque_validar_integridad_bitmap();
	log_debug(logger, "Se finaliza fsck_chequeo_de_sabotajes_en_superbloque()");
}

//Valida que la cantidad de bloques en el archivo Superbloque.ims se corresponda con la cantidad real en el recurso fisico de Blocks.ims
void superbloque_validar_integridad_cantidad_de_bloques()
{
	log_debug(logger, "Se inicia superbloque_validar_integridad_cantidad_de_bloques()");
	struct stat* blocks_stat = malloc(sizeof(struct stat));
	if(stat(blocks_path, blocks_stat) != 0)
	{
		log_error(logger, "No se pudo acceder a la informacion del archivo Blocks.ims, por lo tanto no podra validarse si hubo un sabotaje a la cantidad de bloques");
		exit(EXIT_FAILURE);
	}
	//blocks_size es tamanio del archivo de Blocks.ims a diferencia de block_size que es el tamanio de cada unidad de block
	blocks_size = blocks_stat->st_size; //Tamanio real del archivo //no se podria usar directamente blocks_size??

	int desplazamiento = 0;
	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.block_size, sizeof(uint32_t));//aca lo mismo, la estructura en memoria no pudo ser alterada, no se podria directamente tomar sus datos??
	desplazamiento += sizeof(uint32_t);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));

	int blocks_real = blocks_size / superbloque.block_size;
	if(superbloque.blocks != blocks_real)
	{
		log_info(logger, "Alguien saboteo la cantidad de bloques en el SuperBloque.ims");
		superbloque.blocks = blocks_real;
		lseek(superbloque_fd, desplazamiento, SEEK_SET);
		write(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));
		log_info(logger, "El valor correcto de blocks es %d y ya fue almacenado, ademas se tiene que el tamanio de los bloques es %d",
				superbloque.blocks, superbloque.block_size);
	}
	else
	{
		log_info(logger, "Parece que esta vez nadie se animo a sabotear la cantidad de bloques en el SuperBloque.ims");
	}

	free(blocks_stat);
	close(superbloque_fd);
	log_debug(logger, "Se finaliza superbloque_validar_integridad_cantidad_de_bloques()");
}

//Valida que los bloques indicados en el bitmap del superbloque realmente sean los unicos ocupados
void superbloque_validar_integridad_bitmap()
{
	log_debug(logger, "Info-Se ingresa a superbloque_validar_integridad_bitmap");

	/*superbloque_levantar_estructura_desde_archivo();

	char* bitmap_real = superbloque_obtener_bitmap_correcto_segun_bloques_ocupados();
	char bitmap_real_md5[33];
	cadena_calcular_md5(bitmap_real, bitmap_size, bitmap_real_md5);

	char bitmap_actual_md5[33];
	cadena_calcular_md5(superbloque.bitmap, bitmap_size, bitmap_actual_md5);
log_info(logger, "bitmap_real_md5: %s", bitmap_real_md5);
log_info(logger, "bitmap_actual_md5: %s", bitmap_actual_md5);*/
//utils_esperar_a_usuario();

	if(strcmp(md5_bitmap_real_inicial, md5_bitmap_actual))
	//if(strcmp(bitmap_actual_md5, bitmap_real_md5))
	{
		log_info(logger, "Alguien saboteo el bitmap en el SuperBloque.ims");
		/*memcpy(superbloque.bitmap, bitmap_real, bitmap_size);

		int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);

		int desplazamiento = 2 * sizeof(uint32_t);
		lseek(superbloque_fd, desplazamiento, SEEK_SET);
		write(superbloque_fd, superbloque.bitmap, bitmap_size);

		close(superbloque_fd);*/
		log_info(logger, "El bitmap correcto ya fue persistido");
	}
	else
	{
		log_info(logger, "Parece que esta vez nadie se animo a sabotear el bitmap");
	}

	//log_info(logger, "bitmap_real_md5: %s", bitmap_real_md5);
	//log_info(logger, "bitmap_actual_md5: %s", bitmap_actual_md5);
	//utils_esperar_a_usuario();

	//free(bitmap_real);
	log_debug(logger, "Se finaliza superbloque_validar_integridad_bitmap()");
}
/* nueva de edu:
void superbloque_validar_integridad_bitmap()
{
    log_debug(logger, "Info-Se ingresa a superbloque_validar_integridad_bitmap");
    superbloque_levantar_estructura_desde_archivo();
    char* bitmap_real = superbloque_obtener_bitmap_correcto_segun_bloques_ocupados();
    char md5_bitmap_real[33];
    cadena_calcular_md5(bitmap_real, bitmap_size, md5_bitmap_real);
    char md5_bitmap_actual[33];
    int desplazamiento = 2 * sizeof(uint32_t);
    char* bitmap_actual = malloc(bitmap_size);
    int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
    lseek(superbloque_fd, desplazamiento, SEEK_SET);
    read(superbloque_fd, bitmap_actual, bitmap_size);
    close(superbloque_fd);
    cadena_calcular_md5(bitmap_actual, bitmap_size, md5_bitmap_actual);
    if(strcmp(md5_bitmap_actual, md5_bitmap_real))
    {
        log_info(logger, "Alguien saboteo el bitmap en el SuperBloque.ims");
        int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
        lseek(superbloque_fd, desplazamiento, SEEK_SET);
        write(superbloque_fd, bitmap_real, bitmap_size);
        close(superbloque_fd);
        log_info(logger, "El bitmap correcto ya fue persistido");
    }
    else
    {
        log_info(logger, "Parece que esta vez nadie se animo a sabotear el bitmap");
    }
    free(bitmap_real);
    log_debug(logger, "Se finaliza superbloque_validar_integridad_bitmap()");
}
 */

//Setea al bitmap apuntado por el campo bitmap de la estructura superbloque segun los bloques ocupados que recibe como parametro
char* superbloque_obtener_bitmap_correcto_segun_bloques_ocupados()
{
	char* bitmap = malloc(bitmap_size);
	memset(bitmap, 0, bitmap_size);

	char* bloques_ocupados = files_obtener_cadena_con_el_total_de_bloques_ocupados();
	int cantidad_bloques_ocupados = cadena_cantidad_elementos_en_lista(bloques_ocupados);
	char** lista_bloques_ocupados = string_get_string_as_array(bloques_ocupados);

	t_bitarray* bitarray_auxiliar = bitarray_create_with_mode(bitmap, bitmap_size, LSB_FIRST);

	int bloque;
	for(int i = 0; i < cantidad_bloques_ocupados; i++)
	{
		bloque = atoi(lista_bloques_ocupados[i]);
		bitarray_set_bit(bitarray_auxiliar, bloque);
	}

	bitarray_destroy(bitarray_auxiliar);
	cadena_eliminar_array_de_cadenas(&lista_bloques_ocupados, cantidad_bloques_ocupados);
	free(bloques_ocupados);

	return bitmap;
}

//Devuelve una lista en formato cadena con los bloques ocupados por el total de archivos en Files (recursos + bitacoras)
char* files_obtener_cadena_con_el_total_de_bloques_ocupados()
{
	log_debug(logger, "Se ingresa a files_obtener_cadena_con_bloques_ocupados");

	char* bloques_de_recursos = recursos_obtener_cadena_con_el_total_de_bloques_ocupados();
	char* bloques_ocupados = bloques_de_recursos;
	char* bloques_de_bitacoras = bitacoras_obtener_cadena_con_el_total_de_bloques_ocupados();

	cadena_agregar_a_lista_existente_valores_de_otra_lista(&bloques_ocupados, bloques_de_bitacoras);

	free(bloques_de_bitacoras);
	log_debug(logger, "El total de bloques ocupados es %s", bloques_ocupados);
	return bloques_ocupados;
}

//Devuelve una lista en formato cadena con los bloques ocupados por el total de recursos
char* recursos_obtener_cadena_con_el_total_de_bloques_ocupados()
{
	//log_debug(logger, "Se ingresa a recursos_obtener_cadena_con_el_total_de_bloques_ocupados");
	int cantidad_recursos = recursos_cantidad();
	int posicion_ultimo_recurso = cantidad_recursos - 1;

	//char* bloques = string_duplicate(lista_recursos[0].metadata->blocks);//Se podría usar la informacion de los recursos en memoria??
	char* bloques = string_duplicate("[]");

	for(int i = 0; i < cantidad_recursos; i++)
	{
		char* string_buffer = metadata_obtener_bloques_desde_archivo(lista_recursos[i].ruta_completa);

		cadena_agregar_a_lista_existente_valores_de_otra_lista(&bloques, string_buffer);

		free(string_buffer);
	}

	log_debug(logger, "Los bloques ocupados por el total de recursos son %s", bloques);
	return bloques;
}

//Devuelve una lista en formato cadena con los bloques indicados por la key "BLOCKS" en el archivo metadata de la ruta path
char* metadata_obtener_bloques_desde_archivo(char* path)
{
	char* bloques ;
	if(utils_existe_en_disco(path))
	{
		t_config* path_config = config_create(path);
		bloques = string_duplicate(config_get_string_value(path_config, "BLOCKS"));
		config_destroy(path_config);
		log_debug(logger, "Bloques en \"%s\": %s", path, bloques);
	}
	else
	{
		bloques = string_duplicate("[]");
		log_debug(logger, "No existe el archivo \"%s\"", path);
	}

	return bloques;
}

//Agrega a lista_original los valores que se tengan en lista_a_adicionar
void cadena_agregar_a_lista_existente_valores_de_otra_lista(char** lista_original, char* lista_a_adicionar)
{
	if(cadena_cantidad_elementos_en_lista(lista_a_adicionar) > 0)
	{
		if(cadena_cantidad_elementos_en_lista(*lista_original) == 0)
		{
			(**lista_original) = '\0';//(*lista_original)[0] = '\0';
		}
		else
		{
			cadena_sacar_ultimo_caracter(*lista_original);
			lista_a_adicionar[0] = ',';
		}
		string_append(lista_original, lista_a_adicionar);
	}
}

//Devuelve un listado en formato cadena con el total de bloques ocupado por las bitacoras
char* bitacoras_obtener_cadena_con_el_total_de_bloques_ocupados()
{
	log_debug(logger, "Se ingresa a bitacoras_obtener_cadena_con_el_total_de_bloques_ocupados()");

	t_list* lista_paths = bitacoras_obtener_lista_con_rutas_completas();
	int cantidad_bitacoras = list_size(lista_paths);

    char* bloques = string_duplicate("[]");
	for(int i = 0; i < cantidad_bitacoras; i++)
	{
		char* path_buffer = list_get(lista_paths, i);
		char* string_buffer = metadata_obtener_bloques_desde_archivo(path_buffer);
		cadena_agregar_a_lista_existente_valores_de_otra_lista(&bloques, string_buffer);
		free(string_buffer);
	}

	//log_debug(logger, "en bitacoras_obtener_cadena_con_el_total_de_bloques_ocupados() antes de borrar lista de bitacoras");
	list_destroy_and_destroy_elements(lista_paths,(void*)destruir_lista_paquete); //OJO ESTA ROMPIENDO ESTO

	log_debug(logger, "Los bloques ocupados por el total de las bitacoras son %s", bloques);
	return bloques;
}

//Obtiene la lista con todas las rutas completas de los archivos existentes en la carpeta bitacora
t_list* bitacoras_obtener_lista_con_rutas_completas()
{
	t_list* lista_paths = list_create();

	struct dirent* elemento; //res, resource, recurso
	struct stat bitacoras_stat;

	if(stat(bitacoras_path, &bitacoras_stat) == 0 && S_ISDIR(bitacoras_stat.st_mode))
	{
		DIR *folder = opendir(bitacoras_path);

		if(folder != NULL)
		{
			while((elemento = readdir(folder)))
			{
				if(strcmp(elemento->d_name, ".") && strcmp(elemento->d_name, "..") && strncmp(elemento->d_name, "Tripulante", 10) == 0)
				{
					char *ruta_temporal = string_from_format("%s/%s", bitacoras_path, elemento->d_name);
					printf("La ruta a agregar a la lista queda: %s\n", ruta_temporal);
					list_add(lista_paths, ruta_temporal);
				}
			}
			closedir(folder);
		}
		else
		{
			log_error(logger, "No se pudo abrir el directorio");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		log_error(logger, "%s no es un directorio valido", bitacoras_path);
		exit(EXIT_FAILURE);
	}
	log_debug(logger, "salgo de t_list* bitacoras_obtener_lista_con_rutas_completas()");
	return lista_paths;
}

//Realiza los chequeos propios de los recursos
void fsck_chequeo_de_sabotajes_en_files()
{
	int cantidad_recursos = recursos_cantidad();

	for(int i = 0; i < cantidad_recursos; i++)
	{
		t_recurso_data* recurso_data = &lista_recursos[i];
		if(utils_existe_en_disco(recurso_data->ruta_completa))
		{
			recurso_validar_blocks_bloque_mayor_a_fs(recurso_data);
			recurso_validar_blocks_bloque_ocupado(recurso_data);
			recurso_validar_size(recurso_data);
			recurso_validar_block_count(recurso_data);
			recurso_validar_blocks_orden_bloques(recurso_data);
		}
		else
		{
			log_info(logger, "No existe en disco el archivo \"%s\" por lo tanto no pudo ser saboteado", recurso_data->nombre_archivo);
		}
	}
}

void recurso_validar_blocks_bloque_mayor_a_fs(t_recurso_data* recurso_data)
{
	log_debug(logger, "Se ingresa a recurso_validar_blocks_bloque_mayor_a_fs()");

	t_config* recurso_config = config_create(recurso_data->ruta_completa);
	char* blocks_actual = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));

	char* lista_blocks_con_numeros_validos = string_duplicate("[]");

	char** blocks_actual_array = string_get_string_as_array(blocks_actual);

	int cantidad_bloques_actual = cadena_cantidad_elementos_en_lista(blocks_actual);
	int bloque_maximo = superbloque.blocks -1;

	for(int i=0; i < cantidad_bloques_actual; i++){
		int bloque = atoi(blocks_actual_array[i]);
		if(bloque <= bloque_maximo){
			cadena_agregar_entero_a_lista_de_enteros(&lista_blocks_con_numeros_validos, bloque);
		}
	}

	if(strcmp(lista_blocks_con_numeros_validos, blocks_actual) != 0)
	{
	    log_info(logger, "El recurso %s tiene su blocks saboteado (se agrego un bloque mayor a la cantidad de bloques del FS), "
	    		"pero no te preocupes, ya lo arreglaremos", recurso_data->nombre);
	    config_set_value(recurso_config, "BLOCKS", lista_blocks_con_numeros_validos);
	    config_save(recurso_config);
	    log_info(logger, "Listo, ya podes confiar que los bloques no son incoherentes (no son mayores al maximo) en el blocks \"%s\" para el recurso %s",
	    		recurso_data->metadata->blocks, recurso_data->nombre);
	}
	else
	{
		log_info(logger, "Por lo menos al blocks del recurso %s no se le agregaron un bloque mayor a la cantidad de bloques del FS, enhorabuena!!!",
				recurso_data->nombre);
	}

	log_debug(logger, "Se finaliza recurso_validar_blocks_bloque_mayor_a_fs()");

	free(blocks_actual);
	free(lista_blocks_con_numeros_validos);
	cadena_eliminar_array_de_cadenas(&blocks_actual_array, cantidad_bloques_actual);
	config_destroy(recurso_config);
}

void recurso_validar_blocks_bloque_ocupado(t_recurso_data* recurso_data)
{
	log_debug(logger, "Se ingresa a recurso_validar_blocks_bloque_vacio()");

	t_config* recurso_config = config_create(recurso_data->ruta_completa);
	char* blocks_actual = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));

	char* lista_blocks_con_bloques_ocupados = string_duplicate("[]");

	char** blocks_actual_array = string_get_string_as_array(blocks_actual);

	int cantidad_bloques_actual = cadena_cantidad_elementos_en_lista(blocks_actual);
	//int bloque_maximo = superbloque.blocks -1;

	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
	int desplazamiento = 2 * sizeof(uint32_t);

	lseek(superbloque_fd, desplazamiento, SEEK_SET);

	//free(superbloque.bitmap);
	read(superbloque_fd, superbloque.bitmap, bitmap_size);

	close(superbloque_fd);


	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, bitmap_size, LSB_FIRST);

	for(int i=0; i < cantidad_bloques_actual; i++){
		int bloque = atoi(blocks_actual_array[i]);
		if(bitarray_test_bit(bitmap, bloque))
		{
			cadena_agregar_entero_a_lista_de_enteros(&lista_blocks_con_bloques_ocupados, bloque);
		}
	}

	bitarray_destroy(bitmap);

	if(strcmp(lista_blocks_con_bloques_ocupados, blocks_actual) != 0)
	{
	    log_info(logger, "El recurso %s tiene su blocks saboteado (se agrego al menos un bloque vacio), "
	    		"pero no te preocupes, ya lo arreglaremos", recurso_data->nombre);
	    config_set_value(recurso_config, "BLOCKS", lista_blocks_con_bloques_ocupados);
	    config_save(recurso_config);
	    log_info(logger, "Listo, ya podes confiar que no hay ningun bloque vacio en el blocks \"%s\" para el recurso %s",
	    		recurso_data->metadata->blocks, recurso_data->nombre);
	}
	else
	{
		log_info(logger, "Por lo menos al blocks del recurso %s no se le agrego ningun bloque vacio, enhorabuena!!!",
				recurso_data->nombre);
	}

	log_debug(logger, "Se finaliza recurso_validar_blocks_bloque_ocupado()");

	free(blocks_actual);
	free(lista_blocks_con_bloques_ocupados);
	cadena_eliminar_array_de_cadenas(&blocks_actual_array, cantidad_bloques_actual);
	config_destroy(recurso_config);
}

//Antes de sabotear tengo: blocks=[0,1,2,3]  size=350 (suponiendo que los bloques son de 100 bytes)
//[1,1,1,1,1]
//[0,1,2,3]

//Verifica el valor size del recurso dado por recurso_data respecto del valor que tendria que tener y lo reemplaze si difiere
void recurso_validar_size(t_recurso_data* recurso_data)
{
	t_config* recurso_config = config_create(recurso_data->ruta_completa);

	int size_actual = config_get_int_value(recurso_config, "SIZE");

	free(recurso_data->metadata->blocks);
	recurso_data->metadata->blocks = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));

	int size_real = recurso_obtener_size_real(recurso_data);

	if(size_real != size_actual)
	{
		log_info(logger, "El recurso %s tiene su size saboteado, pero no te preocupes, ya lo arreglaremos", recurso_data->nombre);
		log_debug(logger, "Size actual %d size real %d", size_actual, size_real);

		char* size_real_str = string_itoa(size_real);
		config_set_value(recurso_config, "SIZE", size_real_str);
		free(size_real_str);
		config_save(recurso_config);

		log_info(logger, "Listo, ya podes confiar en el size del recurso %s", recurso_data->nombre);
	}
	else
	{
		log_info(logger, "Por lo menos el size del recurso %s no fue alterado, enhorabuena!!!", recurso_data->nombre);
	}
	config_destroy(recurso_config);
}

//Obtiene el valor de size que tendria que tener el recurso dado en funcion de la ocupacion de sus bloques en Blocks.ims
int recurso_obtener_size_real(t_recurso_data* recurso_data)
{
	int size_real = 0;
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_data->metadata->blocks);
	char** lista_bloques = string_get_string_as_array(recurso_data->metadata->blocks);

	if(cantidad_bloques > 0)
	{
		int bloque;
		int esta_lleno;

		for(int i = 0; i < cantidad_bloques; i++)
		{
			bloque = atoi(lista_bloques[i]);
			esta_lleno = blocks_esta_lleno_bloque(bloque);

			if(esta_lleno == 0)
			{
				log_debug(logger, "bloque %d NO esta lleno", bloque);
				i = cantidad_bloques;
			}
		}

		if(esta_lleno == 0)
		{
			int cantidad_en_bloque_incompleto = metadata_cantidad_del_caracter_en_bloque(recurso_data->caracter_llenado, bloque);
			log_debug(logger, "cantidad_en_bloque_incompleto %d ", cantidad_en_bloque_incompleto);
			int cantidad_restante_de_bloques = cantidad_bloques -  1;
			size_real += cantidad_en_bloque_incompleto + cantidad_restante_de_bloques * superbloque.block_size;
		}
		else
		{
			size_real += cantidad_bloques * superbloque.block_size;
		}
	}

	return size_real;
}

//Devuelve un valor distinto de 0 en el caso de que el bloque dado no tenga ningun caracter nulo
int blocks_esta_lleno_bloque(int bloque)
{
	int esta_lleno = 0;
	int posicion_en_bloque = 0;
	int posicion_inicio_bloque = bloque * superbloque.block_size;
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque;

	while((*posicion_absoluta) != '\0' && posicion_en_bloque < superbloque.block_size)
	{
		posicion_en_bloque++;
		posicion_absoluta++;
	}

	if(posicion_en_bloque == superbloque.block_size)
	{
		esta_lleno = 1;
	}

	return esta_lleno;
}

//Devuelve la cantidad que tiene el bloque dado del caracter dado hasta el caracter nulo o el tamanio del bloque de no encontrarlo
int metadata_cantidad_del_caracter_en_bloque(char caracter, int bloque)
{
	int posicion_en_bloque = 0;
	int posicion_inicio_bloque = bloque * superbloque.block_size;
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque;

	while((*posicion_absoluta) == caracter && posicion_en_bloque < superbloque.block_size)
	{
		posicion_en_bloque++;
		posicion_absoluta++;
	}
	return posicion_en_bloque;
}

//Verifica el valor block_count en disco del recurso dado respecto del valor que tendria que tener y lo reemplaze si difiere
void recurso_validar_block_count(t_recurso_data* recurso_data)
{
	int block_count_real = recurso_obtener_block_count_real(recurso_data);

	t_config* recurso_config = config_create(recurso_data->ruta_completa);
	int block_count_actual = config_get_int_value(recurso_config, "BLOCK_COUNT");

	if(block_count_actual != block_count_real)
	{
		log_info(logger, "El recurso %s tiene su block_count saboteado, pero no te preocupes, ya lo arreglaremos", recurso_data->nombre);
		log_debug(logger, "Block_count en archivo %d block_count real %d", block_count_actual, block_count_real);

		char* block_count_real_str = string_itoa(block_count_real);
		config_set_value(recurso_config, "BLOCK_COUNT", block_count_real_str);
		free(block_count_real_str);
		config_save(recurso_config);

		log_debug(logger, "Block_count modificado en archivo a %d", config_get_int_value(recurso_config, "BLOCK_COUNT"));
		log_info(logger, "Listo, ya podes confiar en el block_count del recurso %s", recurso_data->nombre);
	}
	else
	{
		log_info(logger, "Por lo menos el block_count del recurso %s no fue alterado, enhorabuena!!!", recurso_data->nombre);
	}
	config_destroy(recurso_config);
}

//Obtiene el valor de block_count que tendria que tener el recurso dado en funcion de su lista de blocks
int recurso_obtener_block_count_real(t_recurso_data* recurso_data)
{
	//Cadena de blocks seteada en recurso_validar_size
	return cadena_cantidad_elementos_en_lista(recurso_data->metadata->blocks);
}

//FIXME Si encuentra una diferencia entre los md5 de los concatenados restaura el archivo en blocks con los blocks del archivo metadata
void recurso_validar_blocks_orden_bloques(t_recurso_data* recurso_data)
{
    t_recurso_md* recurso_md = recurso_data->metadata;
    free(recurso_md->blocks);//OK?
    recurso_levantar_de_archivo_a_memoria_valores_variables(recurso_data);

    char md5_segun_blocks_actual[33];
    if(recurso_md->size > 0)
    {
        char* concatenado_segun_blocks_actual = blocks_obtener_concatenado_de_recurso(recurso_data->metadata);
        cadena_calcular_md5(concatenado_segun_blocks_actual, recurso_data->metadata->size, md5_segun_blocks_actual);
        free(concatenado_segun_blocks_actual);
    }
    else
    {
        strcpy(md5_segun_blocks_actual, "d41d8cd98f00b204e9800998ecf8427e");
    }

    if(strcmp(recurso_md->md5_archivo, md5_segun_blocks_actual) != 0)
    {

        log_info(logger, "El recurso %s tiene su blocks saboteado, pero no te preocupes, ya lo arreglaremos restaurando los bloques en Blocks.ims",
                recurso_data->nombre);

        metadata_restaurar_en_blocks(recurso_data->metadata);
        //no haría falta recalcular el md5 ni setearlo nuevamente ya que nunca fue modificado ni tendría que modificarse
        //recurso_actualizar_archivo(recurso_data);
        log_info(logger, "Listo, ya podes confiar en el blocks \"%s\" para el recurso %s", recurso_data->metadata->blocks, recurso_data->nombre);
    }
    else
    {
        log_info(logger, "Por lo menos el orden del blocks del recurso %s no fue alterado, enhorabuena!!!", recurso_data->nombre);
    }
}


void metadata_restaurar_en_blocks(t_recurso_md* recurso_md)
{
	int cantidad_en_ultimo_bloque = recurso_md->size % superbloque.block_size;
	recurso_md->size = 0;

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	int indice_ultimo_bloque = cantidad_bloques - 1; //Indice en base a la cantidad de bloques de la lista en el recurso
	int bloque_a_cargar;//Numero de bloque en base al total de bloques del sistema
	char** lista_bloques = string_get_string_as_array(recurso_md->blocks);

	for(int i = 0; i < indice_ultimo_bloque; i++)
	{
		bloque_a_cargar = atoi(lista_bloques[i]);
		metadata_cargar_bloque_completo(recurso_md, bloque_a_cargar);
	}

	bloque_a_cargar = atoi(lista_bloques[indice_ultimo_bloque]);

	if(cantidad_en_ultimo_bloque != 0)
	{
		metadata_cargar_parcialmente_bloque(recurso_md, &cantidad_en_ultimo_bloque, bloque_a_cargar);
	}
	else
	{
		metadata_cargar_bloque_completo(recurso_md, bloque_a_cargar);
	}

	log_debug(logger, "Saliendo de metadata_restaurar_en_blocks");

}

/////////////////////////////////////FIN DE FUNCIONES NECESARIAS PARA FSCK/////////////////////////////////////////////////////////


////////////////////////////FUNCIONES UTILS (SOBRETODO PARA MANEJO DE ARCHIVOS)/////////////////////////

//Crea el directorio indicado por path, en caso de no poder finaliza con error
void utils_crear_directorio_si_no_existe(char* path)
{
	//Si no existe el directorio lo crea con el mkdir y automaticamente pregunta si se creo correctamente, si no fue asi finaliza con error
	if (utils_existe_en_disco(path) == 0 && mkdir(path, 0777) == -1)
	{
		log_error(logger, "No existia el directorio %s desde antes ni puede crearse. No pueden crearse el total de directorios del File System", path);
		exit(EXIT_FAILURE);
	}
}

//Verifica que un determinado path exista chequeando que sea accesible
int utils_existe_en_disco(char* path)
{
	return (access(path, F_OK) == 0);
}

//Abre el archivo indicado por path para lectura/escritura retornando el file descriptor. Finaliza en caso de error
int utils_abrir_archivo_para_lectura_escritura(char* path)
{
	int file_descriptor;

	file_descriptor = open(path, O_RDWR);
	if(file_descriptor == -1)
	{
		log_error(logger, "%s no pudo ser abierto para lectura/escritura. Se finaliza el programa", path);
		exit(EXIT_FAILURE);
	}

	return file_descriptor;
}

//Crea el archivo indicado por path. Si no es posible finaliza con error
void utils_crear_archivo(char* path)
{
	int file_descriptor;

	file_descriptor = creat(path, 0777);
	if(file_descriptor == -1)
	{
		log_error(logger, "El archivo con ruta completa %s no pudo ser creado. Se finaliza el programa", path);
		exit(EXIT_FAILURE);
	}

	close(file_descriptor);
	//log_debug(logger, "IO-Se creo el archivo dado por la ruta completa \"%s\"", path);
}

//Da al archivo obtenido por la ruta path el tamanio indicado por length
void utils_dar_tamanio_a_archivo(char* path, size_t length)
{
	if(truncate(path, length) == -1)
	{
		log_error(logger, "No se puede asignar el tamanio %d al archivo de ruta %s", length, path);
		exit(EXIT_FAILURE);
	}
	//log_debug(logger, "O-Se da el tamanio exitosamente a archivo con ruta %s", path);
}

void utils_esperar_a_usuario()
{

	printf("Escribe algun caracter mas Enter para continuar\n");
	getchar();
	getchar();
}

////////////////////////////FIN DE FUNCIONES UTILS (SOBRETODO PARA MANEJO DE ARCHIVOS)/////////////////////////

////////////////////////////FUNCIONES ADICIONALES PARA MANEJO DE CADENAS DE TEXTO/////////////////////////

//Calcula la cantidad de elementos de una lista en formato de cadena de texto. Ej: "[algo, y, otro, algo]" => 4 o "[4,6]" => 2
int cadena_cantidad_elementos_en_lista(char* cadena)
{
	char** lista = string_get_string_as_array(cadena);

	int posicion;
	for(posicion = 0; lista[posicion] != NULL; posicion++);

	cadena_eliminar_array_de_cadenas(&lista, posicion);
	return posicion;
}

//Elimina un array con la cantidad de cadenas dada liberando la memoria
void cadena_eliminar_array_de_cadenas(char*** puntero_a_array_de_punteros, int cantidad_cadenas)
{
	for(int i = 0; i < cantidad_cadenas; i++)
	{
		free((*puntero_a_array_de_punteros)[i]);
	}

	free(*puntero_a_array_de_punteros);
	*puntero_a_array_de_punteros = NULL;
}

//Devuelve el ultimo entero de una lista de enteros
int cadena_ultimo_entero_en_lista_de_enteros(char* lista_de_enteros)
{
	char** lista = string_get_string_as_array(lista_de_enteros);
	int cantidad_enteros = cadena_cantidad_elementos_en_lista(lista_de_enteros);

	if(cantidad_enteros == 0)
	{
		log_error(logger, "La lista de la que se desea tener el ultimo entero esta vacia");
		return -1;
	}

	int ultimo_entero = atoi(lista[cantidad_enteros - 1]);
	cadena_eliminar_array_de_cadenas(&lista, cantidad_enteros);
	return ultimo_entero;
}

//Agrega a la lista de blocks de la metadata un bloque nuevo obtenido a traves del bitmap del superbloque y a su vez lo devuelve
void cadena_agregar_entero_a_lista_de_enteros(char** lista_de_enteros, int entero)
{
	//log_debug(logger, "I-Se ingresa a metadata_agregar_bloque_a_lista_de_blocks con blocks %s, block_count %d y bloque a agregar %d", recurso_md->blocks, recurso_md->block_count, bloque);
	int cantidad_de_enteros_original = cadena_cantidad_elementos_en_lista(*lista_de_enteros);
	cadena_sacar_ultimo_caracter(*lista_de_enteros);

	if(cantidad_de_enteros_original != 0)
	{
		string_append_with_format(lista_de_enteros, ",%d]", entero);
	}
	else
	{
		string_append_with_format(lista_de_enteros, "%d]", entero);
	}
	//log_debug(logger, "Entero %d agregado a lista_de_enteros quedando %s", *lista_de_enteros);  OJO QUE NOSE SI ANDA BIENN. IMPRIME RARO.
}

//Saca el ultimo caracter a una cadena de caracteres redimensionando el espacio de memoria
void cadena_sacar_ultimo_caracter(char* cadena)
{
	int tamanio_final_cadena = strlen(cadena);
	cadena[tamanio_final_cadena - 1] = '\0';
}

//Calcula el valor de md5 de la cadena de texto apuntada por el puntero cadena y de tamanio length, y lo guarda en el char array apuntado por md5_str
void cadena_calcular_md5(const char *cadena, int length, char* md5_33str)
{
    MD5_CTX c;
    unsigned char digest[16];

    MD5_Init(&c);

    while(length > 0)
    {
        if(length > 512)
        {
            MD5_Update(&c, cadena, 512);
        }
        else
        {
            MD5_Update(&c, cadena, length);
        }

        length -= 512;
        cadena += 512;
    }

    MD5_Final(digest, &c);

    for(int n = 0; n < 16; ++n)
    {
        snprintf(&(md5_33str[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
}

////////////////////////////FIN DE FUNCIONES ADICIONALES PARA MANEJO DE CADENAS DE TEXTO/////////////////////////


/////////////////////////FUNCIONES A BORRAR (O NO)/////////////////////

//Funcion para encontrar falla de carga en superbloque
void verificar_superbloque_temporal()
{
	int fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
	lseek(fd, 0, SEEK_SET);
	int sb_b_s, sb_b;
	read(fd, &sb_b_s, sizeof(uint32_t));
	lseek(fd, sizeof(uint32_t), SEEK_SET);
	read(fd, &sb_b, sizeof(uint32_t));
	log_info(logger, "PARA DEBUG SB.B_S %d Y SB.BLOCKS %d y en archivo SB.B_S %d Y SB.BLOCKS %d", superbloque.block_size, superbloque.blocks, sb_b_s, sb_b);
	close(fd);
}

//Prueba de tareas random
void prueba_de_tareas_random()
{
	printf("tarea inexistente con cantidad mayor a 0\n");
	recurso_realizar_tarea("lalala", "1");
	printf("tarea inexistente con cantidad igual a 0\n");
	recurso_realizar_tarea("lalala", "0");
	utils_esperar_a_usuario();
}

//Funcion para probar generar recurso
void prueba_de_generar_recursos()
{
	int block_size = superbloque.block_size;
	int tamanios[4] = {block_size, block_size + 1, block_size - 1, 0};

	for(int i = 0; i < 3; i++)
	{
		char* nombre_tarea = string_duplicate(lista_tareas_disponibles[i].nombre);
		for(int j = 0; j < 4; j++)
		{
			char* cantidad_str = string_itoa(tamanios[j]);
			recurso_realizar_tarea(nombre_tarea, cantidad_str);
			free(cantidad_str);
		}
		free(nombre_tarea);
		utils_esperar_a_usuario();
	}
}

//Funcion para probar consumir recursos
void prueba_de_consumir_recursos()
{
	int block_size = superbloque.block_size;
	int tamanios[4] = {block_size, block_size + 1, block_size - 1, 0};

	char* nombre_tarea;

	for(int i = 3; i < 5; i++)
	{
		nombre_tarea = string_duplicate(lista_tareas_disponibles[i].nombre);
		for(int j = 0; j < 4; j++)
		{
			char* cantidad_str = string_itoa(tamanios[j]);
			recurso_realizar_tarea(nombre_tarea, cantidad_str);
			free(cantidad_str);
			utils_esperar_a_usuario();
		}
		free(nombre_tarea);
	}
}

//Funcion para probar descartar recurso
void prueba_de_descartar_basura()
{
	int block_size = superbloque.block_size;
	int tamanios[] = {-1, 1, 0, 0, 1};

	char* nombre_tarea = string_duplicate("DESCARTAR_BASURA");

	for(int i = 0; i < 4; i++)
	{
		char* cantidad_str = string_itoa(tamanios[i]);
		recurso_realizar_tarea(nombre_tarea, cantidad_str);
		free(cantidad_str);
		utils_esperar_a_usuario();
	}
	free(nombre_tarea);
}

//Funcion para probar carga de bitacoras
void prueba_de_cargar_n_bitacoras_con_mensaje(int n, char* mensaje_de_prueba)
{
	for(int tid = 0; tid < n; tid++)
	{
		char* tid_str = string_itoa(tid);
		char* mensaje = string_from_format("%s de %d", mensaje_de_prueba, tid);
		cargar_bitacora(tid_str, mensaje);
		free(tid_str);
		free(mensaje);
		utils_esperar_a_usuario();
	}
}

void prueba_de_cargar_bitacora_de_tripulante_n_con_m_mensajes(int n, int m, char* mensaje_de_prueba)
{
	/*char* tid_str = string_itoa(n);
	for(int i = 0; i < m; i++)
	{
		char* mensaje = string_from_format("%s de %d, bitacora %d", mensaje_de_prueba, n, i);
		cargar_bitacora(tid_str, mensaje);
	}
	free(tid_str);
	free(mensaje_de_prueba);
	utils_esperar_a_usuario();*/
	for(int tid = 0; tid < n; tid++)
	{
		char* tid_str = string_itoa(tid);
		char* mensaje = string_from_format("%s de %d", mensaje_de_prueba, tid);
		cargar_bitacora(tid_str, mensaje);
		free(tid_str);
		free(mensaje);
		utils_esperar_a_usuario();
	}
	for(int tid = 0; tid < n; tid++)
		{
			char* tid_str = string_itoa(tid);
			char* mensaje = string_from_format("%s de %d", mensaje_de_prueba, tid);
			cargar_bitacora(tid_str, mensaje);
			free(tid_str);
			free(mensaje);
			utils_esperar_a_usuario();
		}
	for(int tid = 0; tid < n; tid++)
		{
			char* tid_str = string_itoa(tid);
			char* mensaje = string_from_format("%s de %d", mensaje_de_prueba, tid);
			cargar_bitacora(tid_str, mensaje);
			free(tid_str);
			free(mensaje);
			utils_esperar_a_usuario();
		}
}

//////////////////////// OBTENER BITACORA ////////////
//Obtiene el conjunto de bloques concatenado que el recurso esta ocupando en el mapeo en memoria del archivo Blocks.ims
/*char* blocks_obtener_concatenado_de_recurso(t_recurso_md* recurso_md)
{
	return blocks_obtener_concatenado_de_bloques_segun_tamanio(recurso_md->blocks, recurso_md->size);
}*/

//Obtiene el conjunto de bloques concatenado que la bitacora esta ocupando en el mapeo en memoria del archivo Blocks.ims
char* bitacora_obtener_mensajes_de_tripulante(int tid)
{
	log_debug(logger, "Se ingresa bitacora_obtener_mensajes_de_tripulante(int tid) con tripulante %d", tid);

	t_bitacora_data* bitacora_data = bitacora_cargar_estructura_completa(tid);
	char* concatenado = NULL;

	if(utils_existe_en_disco(bitacora_data->ruta_completa))
	{
		int size = bitacora_data->metadata->size;
		char* blocks = bitacora_data->metadata->blocks;
		//concatenado = malloc(size);
		concatenado = blocks_obtener_concatenado_de_bloques_segun_tamanio(blocks, size);
		bitacora_borrar_estructura_completa(bitacora_data);
	}

	log_debug(logger, "Posicion de memoria del concatenado de los bloques Tripulante%d.ims", tid);
	return concatenado;
}

//Obtiene el concatenado de los bloques segun lo que ocupan estos en el mapeo del archivo Blocks.ims hasta la cantidad tamanio de bytes
char* blocks_obtener_concatenado_de_bloques_segun_tamanio(char* bloques, size_t tamanio)
{
	//log_debug(logger, "I-Se ingresa a blocks_obtener_concatenado_de_recurso");
	char* concatenado = malloc(tamanio);

	if(concatenado != NULL)
	{
		int cantidad_bloques = cadena_cantidad_elementos_en_lista(bloques);
		int indice_ultimo_bloque = cantidad_bloques - 1; //Indice en base a la cantidad de bloques de la lista en el recurso
		int bloque;//Numero de bloque en base al total de bloques del sistema
		char** lista_bloques = string_get_string_as_array(bloques);
		char* posicion_desde; //Posicion de la copia de Blocks.ims desde la que arranca el bloque que se copiara
		char* posicion_hacia = concatenado; //Posicion del espacio de memoria donde se almacenara cada bloque con memcpy

		for(int i = 0; i < indice_ultimo_bloque; i++)
		{
			bloque = atoi(lista_bloques[i]);
			posicion_desde = blocks_address + bloque * superbloque.block_size;
			memcpy(posicion_hacia, posicion_desde, superbloque.block_size);
			posicion_hacia += superbloque.block_size;
		}

		bloque = atoi(lista_bloques[indice_ultimo_bloque]);
		posicion_desde = blocks_address + bloque * superbloque.block_size;
		int cantidad_en_ultimo_bloque = tamanio % superbloque.block_size;
		memcpy(posicion_hacia, posicion_desde, cantidad_en_ultimo_bloque);
		cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);

		concatenado[tamanio] = '\0';
		log_debug(logger, "Total de caracteres del recurso concatenado en direccion de memoria %p", concatenado);
	}
	else
	{
		log_error(logger, "No hay memoria suficiente como para obtener el concatenado para los bloques %s con el tamanio %d", bloques, tamanio);
	}

	return concatenado;
}
