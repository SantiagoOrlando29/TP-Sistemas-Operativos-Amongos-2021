//TODO: Ver de donde sale IP
#include "utils_mongostore.h"

t_recurso_data lista_recursos[] = {
	{"OXIGENO", 'O', "Oxigeno.ims", NULL, NULL},
	{"COMIDA", 'C', "Comida.ims", NULL, NULL},
	{"BASURA", 'B', "Basura.ims", NULL, NULL}
};

int variable_servidor = -1;
int socket_servidor;
int cant_sabotaje = 0;
int socket_cliente_sabotaje = 0;
int numero_tripulante = 1;
int hilo_discordiador = 1;


//FUNCIONES MAS RECIENTES (AGUS + SANTI)

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
	return NULL;
}

void* recibir_buffer(uint32_t* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void destruir_lista_paquete(char* contenido){
	free(contenido);
}

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

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

int recibir_operacion(int socket_cliente)
{
    int cod_op;
    if(recv(socket_cliente, &cod_op, sizeof(uint32_t), MSG_WAITALL) !=0 ){
        //log_info(logger, "cod_op %d",cod_op);
        return cod_op;
    }
    else{
        close(socket_cliente);
        return -1;
    }
}

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

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
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
	while(variable_servidor != 0){

		socket_cliente = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);

		if(socket_cliente>0){
			hilo ++ ;
			log_info(logger, "Estableciendo conexión desde %d", dir_cliente.sin_port);
			log_info(logger, "Creando hilo");

			pthread_t hilo_cliente=(char)hilo;
			pthread_create(&hilo_cliente,NULL,(void*)funcion_cliente,(void*)socket_cliente);
			pthread_detach(hilo_cliente);

		}
	}
	//printf("me fui");

	//log_destroy(logger);

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

    //while(variable_servidor != 0){

    socket_cliente_sabotaje = accept(socket_servidor, (struct sockaddr *) &dir_cliente, &tam_direccion);

	if(socket_cliente_sabotaje>0){
		log_info(logger, "2 Estableciendo conexión desde %d", dir_cliente.sin_port);
		log_info(logger, "2 Creando hilo");

		pthread_t hilo_cliente_discordiador_sabotaje;
		pthread_create(&hilo_cliente_discordiador_sabotaje,NULL,(void*)funcionx ,(void*)socket_cliente_sabotaje);
		pthread_detach(hilo_cliente_discordiador_sabotaje);

	}
    //}
    //printf("me fui");

    //log_destroy(logger);
}

int funcionx(int socket_cliente_sabotaje){
    //log_info(logger, "funcionx");
    return 1;
}

int funcion_cliente(int socket_cliente)
{
	//log_info(logger, "in funcion_cliente");

	int tipoMensajeRecibido = -1;
	log_info(logger, "Se conecto este socket a mi %d",socket_cliente);
	//printf("Elegi un tipo de operacion (0 para una tarea, 1 para cargar una bitacora, 2 para iniciar el fsck, 3 para salir)\n");
	//scanf("%d", &operacion);
	int tripulante_id;
	t_list* lista;
	char* parametro_char;
	char* tid_char;

	if(hilo_discordiador != 1){
		//hacer cosas de bitacora
	}
	hilo_discordiador = 0;

	t_bitacora_data* bitacora_data;
	int primera_vez = 1;


	while(1)
	{
		tipoMensajeRecibido = recibir_operacion(socket_cliente);

		switch(tipoMensajeRecibido)
		{
			case REALIZAR_TAREA:
				//recurso_realizar_tarea();
				lista = recibir_paquete(socket_cliente);
				char* nombre_tarea = list_get(lista,0);

				parametro_char = list_get(lista,1);
				int parametro = (int)atoi(parametro_char);

				log_info(logger, "tarea %s  param %d", nombre_tarea, parametro);

				if(strcmp(nombre_tarea, "GENERAR_OXIGENO") == 0){

				}else if(strcmp(nombre_tarea, "CONSUMIR_OXIGENO") == 0){

				}else if(strcmp(nombre_tarea, "GENERAR_COMIDA") == 0){

				}else if(strcmp(nombre_tarea, "CONSUMIR_COMIDA") == 0){

				}else if(strcmp(nombre_tarea, "GENERAR_BASURA") == 0){

				}else if(strcmp(nombre_tarea, "DESCARTAR_BASURA") == 0){

				}else{
					log_debug(logger, "se recibio una tarea distinta a las 6 de E/S");
				}

				char* mensaje2 = "ok";
				enviar_mensaje(mensaje2, socket_cliente);

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;


			case CARGAR_BITACORA:; //PARA PROBAR LO DE MONGO
				lista = recibir_paquete(socket_cliente);

				tid_char = list_get(lista,0);
				tripulante_id = (int)atoi(tid_char);

				char* mens = list_get(lista,1);
				log_info(logger, "tid %d  %s",tripulante_id, mens);  //Mens es lo que hay que cargar en blocks

				if(primera_vez == 1){ //solo entra la primera vez para crear la bitacora
					bitacora_data = malloc(sizeof(t_bitacora_data));

					bitacora_cargar_data_agregando_valores_default_en_metadata(bitacora_data, tripulante_id);
					bitacora_levantar_metadata_de_archivo_si_existe(bitacora_data);
					primera_vez = 0;
				}

				bitacora_guardar_log(bitacora_data, mens);


				char* mensaje = "ok";
				enviar_mensaje(mensaje, socket_cliente);

				list_destroy_and_destroy_elements(lista, (void*)destruir_lista_paquete);

				break;

			case INICIAR_FSCK:
				log_info(logger, "En fsck");

				fsck_iniciar();

				char* mensaje3 = "ok";
				enviar_mensaje(mensaje3, socket_cliente);

				break;

			case FIN:
				log_error(logger, "el discordiador finalizo el programa. Terminando servidor");
				variable_servidor = 0;
				//shutdown(socket_servidor, SHUT_RD);//A VECES ANDA Y A VECES NO
				close(socket_servidor);
				close(socket_cliente);

				log_info(logger, "aaaaaaAAAAA");
				log_destroy(logger);
				//config_destroy(archConfig);
				return EXIT_FAILURE;

			case -1:
				log_warning(logger, "el cliente se desconecto.");
				return EXIT_FAILURE;

			case SALIR:
				printf("Gracias, vuelva prontos\n");
				break;
			default:
				printf("A esa operacion no la tengo, proba con otra\n");
		}
	}

}

void enviar_header(op_code tipo, int socket_cliente)
{
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->mensajeOperacion = tipo;

    void * magic = malloc(sizeof(paquete->mensajeOperacion));
    memcpy(magic, &(paquete->mensajeOperacion), sizeof(uint32_t));

    send(socket_cliente, magic, sizeof(paquete->mensajeOperacion), 0);

    free(magic);
}


void sig_handler(int signum){
    if(signum == SIGUSR1){
        log_info(logger, "SIGUSR1\n");
        notificar_sabotaje();
    }
}

void notificar_sabotaje(){

	char* mensaje4 = configuracion.posiciones_sabotaje[cant_sabotaje];
	/*for(int i = 0; configuracion.posiciones_sabotaje[i]!=NULL;i++){
		log_info(logger, "Posicion %s", configuracion.posiciones_sabotaje[i]); //Para ver las posiciones nomas
	}*/
	enviar_mensaje(mensaje4, socket_cliente_sabotaje);
	cant_sabotaje++;
}


char* contar_archivos(char* path){

	t_list* lista_paths = list_create();

	log_debug(logger, "Se ingresa a obtener todos los bloques de bitacoras");

    size_t count = 0;
    struct dirent *res;
    struct stat sb;
    printf("El path es: %s\n", path);

    char *path_local = string_new();
	string_append(&path_local, path);
	string_append(&path_local, "/");
    printf("El path local es: %s\n", path_local);

    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)){
        DIR *folder = opendir ( path );

        if (access ( path, F_OK ) != -1 ){
            if ( folder ){
                while ( ( res = readdir ( folder ) ) ){
                    if ( strcmp( res->d_name, "." ) && strcmp( res->d_name, ".." ) ){

                        char *ruta_temporal = string_new();
                        string_append(&ruta_temporal, path_local);
                        string_append(&ruta_temporal, res->d_name);
                        list_add(lista_paths, ruta_temporal);
                        printf("La ruta queda: %s\n", ruta_temporal);
                        count++;
                    }
                }

                closedir ( folder );
            }else{
                perror( "No se pudo abrir el directorio\n" );
                exit( EXIT_FAILURE);
            }
        }

    }else{
        printf("%s no se pudo abrir o no es un directorio\n", path);
        exit( EXIT_FAILURE);
    }

    int cantidad_bitacoras = count;

    char* bloques;
    if(cantidad_bitacoras > 0){
        char* listita = list_get(lista_paths, 0);


    	bloques = files_obtener_bloques_de_archivo_metadata(listita);
    	log_debug(logger, "PRIMER BLOQUE : %s", bloques);



    	for(int i = 1; i < cantidad_bitacoras; i++)
    	{
    		cadena_sacar_ultimo_caracter(bloques);
    		listita = list_get(lista_paths, i);
    		char* string_buffer = files_obtener_bloques_de_archivo_metadata(listita);
    		log_debug(logger, "BLOQUE BUFFER : %s", string_buffer);
    		string_buffer[0] = ',';
    		string_append(&bloques, string_buffer);
    		free(string_buffer);
    	}

    	log_debug(logger, "Los bloques ocupados por el total de las bitacoras son %s", bloques);
    }else{
    	log_info(logger, "no hay bitacoras");
    	bloques = "[]";
    }

	free(path_local);
	list_destroy_and_destroy_elements(lista_paths,(void*)destruir_lista_paquete);
	return bloques;

}

void destruir_lista(char* contenido){
    free(contenido);
}



//Funciones agregadas para generar bitacoras


void funcion_cliente_BITACORAS(int tid, char* log)
{

	t_bitacora_data* bitacora_data = malloc(sizeof(t_bitacora_data));

	bitacora_cargar_data_agregando_valores_default_en_metadata(bitacora_data, tid);
	bitacora_levantar_metadata_de_archivo_si_existe(bitacora_data);
	bitacora_guardar_log(bitacora_data, log);
}

void bitacora_cargar_data_agregando_valores_default_en_metadata(t_bitacora_data* bitacora_data, int tid)
{
	bitacora_data->tid = tid;
	bitacora_data->ruta_completa = string_from_format("%s/Tripulante%d.ims", bitacoras_path, bitacora_data->tid);

	t_bitacora_md* metadata = malloc(sizeof(t_bitacora_md));
	metadata->size = (int)0;
	metadata->blocks = string_duplicate("[]");
	bitacora_data->metadata = metadata;
	log_debug(logger, "Bitacora de tripulante %d cargada en memoria con size = %d y blocks = %s", bitacora_data->tid,
			bitacora_data->metadata->size, bitacora_data->metadata->blocks);
}

void bitacora_levantar_metadata_de_archivo_si_existe(t_bitacora_data* bitacora_data)
{
	log_debug(logger, "en bitacora_validar_existencia_metadata: bitacora_data->ruta_completa: %s", bitacora_data->ruta_completa);
	if(utils_existe_en_disco(bitacora_data->ruta_completa))
	{
		t_config* bitacora_config =  config_create(bitacora_data->ruta_completa);

		bitacora_data->metadata->size = config_get_int_value(bitacora_config, "SIZE");
		bitacora_data->metadata->blocks = string_duplicate(config_get_string_value(bitacora_config, "BLOCKS"));

		config_destroy(bitacora_config);

		log_debug(logger, "Bitacora de tripulante %d levantada en memoria con size = %d y blocks = %s", bitacora_data->tid,
				bitacora_data->metadata->size, bitacora_data->metadata->blocks);
	}
}

//Escribe el log reibido en el ultimo bloque escrito si tuviera espacio libre y si no o en los blocks del superbloque que se encuentren libres
void bitacora_guardar_log(t_bitacora_data* bitacora_data, char* log)
{
	//log_debug(logger, "Info-Se ingresa a recurso_generar_cantidad, se debe generar %d %s(s)", cantidad, recurso_data->nombre);
	if(strlen(log) == 0)
	{
		log_error(logger, "No me llego ningun log para guardar");
		return;
	}

	t_bitacora_md* bitacora_md = bitacora_data->metadata;
	int bloque;
	//wait superbloque y blocks
	if(bitacora_tiene_espacio_en_ultimo_bloque(bitacora_md))
	{
		bloque = cadena_ultimo_entero_en_lista_de_enteros(bitacora_md->blocks);
		bitacora_escribir_en_bloque(bitacora_md, &log, bloque);
	}
	while(*log != '\0')
	{
		bloque = superbloque_obtener_bloque_libre();
		cadena_agregar_entero_a_lista_de_enteros(&bitacora_md->blocks, bloque);
		int j =0;
		if(bloque >= 0)
		{
			bitacora_escribir_en_bloque(bitacora_md, &log, bloque);
		}
		else
		{
			log_error(logger, "No se puede escribir el log de la bitacora por no haber bloques libres (no deberia llegar aca)");
		}
	}
	superbloque_actualizar_bitmap_en_archivo();
	blocks_actualizar_archivo();
	bitacora_actualizar_archivo(bitacora_data);
}

//Verifica si el ultimo bloque usado por el recurso tiene espacio disponible
int bitacora_tiene_espacio_en_ultimo_bloque(t_bitacora_md* bitacora_md)
{
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(bitacora_md->blocks);
	log_debug(logger, "En bitacora tiene_espacio_en_ultimo_bloque con SIZE %d, BLOCKS %s y superbloque.block_size %d: (%d)",
			bitacora_md->size, bitacora_md->blocks, superbloque.block_size, bitacora_md->size < cantidad_bloques * superbloque.block_size);
	return (bitacora_md->size < cantidad_bloques * superbloque.block_size);
}

void bitacora_cargar_numero_de_tripulante(t_bitacora_data* bitacora_data, int tid)
{
	bitacora_data->tid = tid;
}


void bitacora_cargar_ruta_completa(t_bitacora_data* bitacora_data)
{
	bitacora_data->ruta_completa = string_from_format("%s/Tripulante%d.ims", bitacoras_path, bitacora_data->tid);
}


void bitacora_crear_metadata_en_archivo_y_memoria_con_recupero(t_bitacora_data* bitacora_data)
{
	log_debug(logger, "bitacora_data->ruta_completa: %s", bitacora_data->ruta_completa);
	if(utils_existe_en_disco(bitacora_data->ruta_completa) == 0)
	{
		bitacora_setear_con_valores_default_en_memoria(bitacora_data);
	}
	else
	{
		bitacora_levantar_archivo(bitacora_data);
	}
	log_debug(logger, "Bitacora de tripulante en memoria %d con size = %d y blocks = %s", bitacora_data->tid,
			bitacora_data->metadata->size, bitacora_data->metadata->blocks);
}

//Setea en memoria los valores variables de la metadata del recurso dado con los valores default
void bitacora_setear_con_valores_default_en_memoria(t_bitacora_data* bitacora_data)
{
	t_bitacora_md* metadata = malloc(sizeof(t_bitacora_md));
	metadata->size = (int)0;
	metadata->blocks = string_duplicate("[]");
	bitacora_data->metadata = metadata;
}


void bitacora_levantar_archivo(t_bitacora_data* bitacora_data)
{
	t_config* bitacora_config =  config_create(bitacora_data->ruta_completa);

	bitacora_data->metadata->size = config_get_int_value(bitacora_config, "SIZE");
	bitacora_data->metadata->blocks = string_duplicate(config_get_string_value(bitacora_config, "BLOCKS"));

	config_destroy(bitacora_config);
}

//Escribe el log reibido en el ultimo bloque escrito si tuviera espacio libre y si no o en los blocks del superbloque que se encuentren libres
void bitacora_escribir_log_en_blocks(t_bitacora_data* bitacora_data, char* log)
{
    log_debug(logger, "I-Se ingresa a escribir_bitacora_en_blocks con log \"%s\"", log);

	/*if(bitacora_tiene_espacio_en_ultimo_bloque(bitacora_md))
	{
		bloque_a_cargar = metadata_ultimo_bloque_usado(bitacora_md);
		bitacora_cargar_parcialmente_bloque(bitacora_md, &cantidad, bloque_a_cargar);
	}
	while(cantidad > 0)
	{
		bloque_a_cargar = superbloque_obtener_bloque_libre();
		if(bloque_a_cargar >= 0)
		{

		}
	}*/

    t_bitacora_md* bitacora_md = bitacora_data->metadata;

    int posicion_en_bloque = bitacora_md->size % superbloque.block_size;
    int bloque = cadena_ultimo_entero_en_lista_de_enteros(bitacora_md->blocks);
    int posicion_inicio_bloque = bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
    char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque; //esta ok sumar un puntero con 2 ints?

    log_debug(logger, "Posicion_en_bloque %d, posicion_inicio_bloque %d posicion block_addres %p posicion absoluta %p",
            posicion_en_bloque, posicion_inicio_bloque, blocks_address, posicion_absoluta);

    while(*log != '\0')
    {
        while(posicion_en_bloque < superbloque.block_size && *log != '\0')
        {
            *posicion_absoluta = *log;

            bitacora_md->size++;
            log++;
            posicion_absoluta++;
            posicion_en_bloque++;
        }
        if(*log != '\0')
        {
        	bloque = superbloque_obtener_bloque_libre();
        	cadena_agregar_entero_a_lista_de_enteros(&bitacora_md->blocks, bloque);
        }
    }

    log_debug(logger, "O-Se cargo el log de la bitacora en la copia de blocks en memoria");
}

void bitacora_escribir_en_bloque(t_bitacora_md* bitacora_md, char** log, int bloque)
{
	int posicion_en_bloque = bitacora_md->size % superbloque.block_size;
	int posicion_inicio_bloque = bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque; //esta ok sumar un puntero con 2 ints?

	log_debug(logger, "Posicion_en_bloque %d, posicion_inicio_bloque %d posicion block_addres %p posicion absoluta %p",
			posicion_en_bloque, posicion_inicio_bloque, blocks_address, posicion_absoluta);
	while(posicion_en_bloque < superbloque.block_size && **log != '\0')
	{
		*posicion_absoluta = **log;

		bitacora_md->size++;
		(*log)++;
		posicion_absoluta++;
		posicion_en_bloque++;
	}
	log_debug(logger, "pase por bitacora_escribir_en_bloque(t_bitacora_md* bitacora_md, char** log, int bloque)");
}

//Actualiza el archivo de la bitacora con la informacion que ya tiene en memoria
void bitacora_actualizar_archivo(t_bitacora_data* bitacora_data)
{
	if(utils_existe_en_disco(bitacora_data->ruta_completa) == 0)
	{
		utils_crear_archivo(bitacora_data->ruta_completa);
	}

	t_config* bitacora_config = config_create(bitacora_data->ruta_completa);
	char* size_str = string_duplicate(string_itoa(bitacora_data->metadata->size));
	config_set_value(bitacora_config, "SIZE", size_str);
	config_set_value(bitacora_config, "BLOCKS", bitacora_data->metadata->blocks);
	free(size_str);

	config_save(bitacora_config);
	config_destroy(bitacora_config);
	log_debug(logger, "Se actualizo el archivo bitacora del tripulante %d en la ruta %s", bitacora_data->tid, bitacora_data->ruta_completa);
}


///////////////////////////////FIN FUNCIONES MAS RECIENTES///////

//Funcion temporal solo para encontrar falla//borrar
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

///////////////////////FUNCIONES ORDENADAS "CRONOLOGICAMENTE"//////////////////////////////////////////////////////////////

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

//Inicia el File System restaurando a memoria los archivos existentes si los hubiera
void file_system_iniciar()
{
	//log_debug(logger, "Info-Comienza iniciar_file_system");
	utils_crear_directorio_si_no_existe(configuracion.punto_montaje);
	file_system_generar_rutas_completas();

	superbloque_validar_existencia();
	blocks_validar_existencia();

	files_crear_directorios_inexistentes(); //Esto se asegura de que ya esten o los crea

	log_info(logger, "File System iniciado");
}

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

//Genera el total de rutas que se usaran para el File System almacenandolas en variables globales
void file_system_generar_rutas_completas()
{
	superbloque_path = string_from_format("%s/%s", configuracion.punto_montaje, "SuperBloque.ims");
	blocks_path = string_from_format("%s/%s", configuracion.punto_montaje, "Blocks.ims");
	files_path = string_from_format("%s/%s", configuracion.punto_montaje, "Files");
	files_cargar_rutas_de_recursos();
	bitacoras_path = string_from_format("%s/%s", files_path, "Bitacoras");
}

//Carga en el sistema las rutas usadas por los recursos existentes en la lista de recursos
void files_cargar_rutas_de_recursos()
{
	int cantidad_recursos = files_cantidad_recursos();

	for(int i = 0; i < cantidad_recursos; i++)
	{
		lista_recursos[i].ruta_completa = string_from_format("%s/%s", files_path, lista_recursos[i].nombre_archivo);
	}
}

//Devuelve la cantidad de recursos que tiene la carpeta files
int files_cantidad_recursos()
{
	return (sizeof(lista_recursos) / sizeof(t_recurso_data));
}

//Verifica la existencia de la estructura superbloque, el archivo SuperBloque.ims y de su mapeo a la memoria. Si no existieran se generan
void superbloque_validar_existencia()
{
	//log_debug(logger, "I-Se ingresa a superbloque_validar_existencia");
	if(utils_existe_en_disco(superbloque_path))
	{
		superbloque_generar_estructura_desde_archivo();
	}
	else
	{
		file_system_eliminar_archivos_previos(); //WARNING!!! Elimina t.odo lo previo por no estar el superbloque
		superbloque_generar_estructura_con_valores_tomados_por_consola();
		utils_crear_archivo(superbloque_path);
		superbloque_cargar_archivo();
	}

	//verificar_superbloque_temporal(); //borrar
	log_info(logger, "Existencia de SuperBloque validada: estructura en memoria, archivo y mapeo a memoria para actualizacion");
}

//Genera la estructura en memoria del superbloque levantandola del archivo//TODO agregar
void superbloque_generar_estructura_desde_archivo()
{
	log_debug(logger, "I-Se ingresa a superbloque_generar_estructura_desde_archivo");

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
	//verificar_superbloque_temporal(); //borrar
	log_debug(logger, "O-Superbloque generado desde archivo");
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

//Setea el tamanio del bitmap en bytes en base a la cantidad de bloques de superbloque.blocks y le asigna dicho espacio en memoria
void superbloque_asignar_memoria_a_bitmap()
{
	//Tamanio del bitmap teniendo en cuenta que pueda haber un byte incompleto
	bitmap_size = 1 + (superbloque.blocks - 1) / CHAR_BIT;
	superbloque.bitmap = malloc(bitmap_size);
	log_debug(logger, "IO-Al salir de superbloque_asignar_memoria_a_bitmap se tiene bitmap_size %d", bitmap_size);
}

//Elimina los archivos que pudieran estar desde antes en el File System (para el caso en que deba iniciarse limpio)
void file_system_eliminar_archivos_previos()
{
	remove(blocks_path); //Elimina el archivo Blocks.ims//ex blocks_eliminar_archivo();
	files_eliminar_carpeta_completa();
	log_info(logger, "Se eliminan archivos previos ya que el superbloque no existe desde antes y por lo tanto no se puede levantar lo existente");
}

//si existe, limina la carpeta Files y t.odo su contenido (incluyendo la carpeta Bitacoras y su contenido)
void files_eliminar_carpeta_completa()
{
	if(utils_existe_en_disco(files_path))
	{
		char* buffer = string_from_format("rm -rf %s", files_path);
		system(buffer);
		free(buffer);
	}
}

//Toma por consola los valores del tamanio de bloques y de cantidad de bloques que setearan a SuperBloque.ims
void superbloque_generar_estructura_con_valores_tomados_por_consola()
{
	log_debug(logger, "I-Se ingresa a superbloque_generar_estructura_con_valores_tomados_por_consola para tomar por consola el tamanio y la cantidad de bloques");

	printf("Se procedera a crear el File System del modulo i-Mongo-Store para AmongOs...\n"
			"Indique por favor cual sera el tamanio que tendran los bloques en el sistema:\n");
	scanf("%d", &superbloque.block_size);

	printf("Cual sera la cantidad de bloques:\n");
	scanf("%d", &superbloque.blocks);

	printf("Muchas gracias!\n");

	superbloque_asignar_memoria_a_bitmap();
	superbloque_setear_bitmap_a_cero();
	//superbloque_setear_tamanio();
	log_debug(logger, "Se ingreso por block_size %d y blocks %d. Ademas se genero el bitmap seteado a vacio",
			superbloque.block_size, superbloque.blocks);
	log_debug(logger, "O-Se cierra consola");
}

//Setea el total de bytes del bitmap en el SuperBloque en memoria a 0
void superbloque_setear_bitmap_a_cero()
{
	memset(superbloque.bitmap, 0, bitmap_size);
}

//Crea el archivo indicado por path. Si no es posible finaliza con error
void utils_crear_archivo(char* path)
{
	int file_descriptor;

	file_descriptor = open(path, O_CREAT|O_RDWR, 0777);
	if(file_descriptor == -1)
	{
		log_error(logger, "El archivo con ruta completa %s no pudo ser creado. Se finaliza el programa", path);
		exit(EXIT_FAILURE);
	}

	close(file_descriptor);
	log_debug(logger, "IO-Se creo el archivo dado por la ruta completa \"%s\"", path);
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

	log_info(logger, "Existencia de Blocks validada");
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

//Mapea a memoria el contenido del archivo Blocks.ims
void blocks_mapear_archivo_a_memoria()
{
	log_debug(logger, "I-Se ingresa a blocks_mapear_archivo_a_memoria");

	int blocks_fd = utils_abrir_archivo_para_lectura_escritura(blocks_path);

	blocks_address = mmap(NULL, blocks_size, PROT_WRITE | PROT_READ, MAP_SHARED, blocks_fd, 0);

	close(blocks_fd);

	log_debug(logger, "O-Blocks mapeado del archivo a la memoria");
}

//Crea las carpetas necesarias para persistir la informacion delasignar File System
void files_crear_directorios_inexistentes()
{
	utils_crear_directorio_si_no_existe(files_path);
	utils_crear_directorio_si_no_existe(bitacoras_path);

	log_debug(logger, "Directorios del File System confirmados");
}
///////////////FIN DE FUNCIONES NECESARIAS PARA INICIAR EL FILE SYSTEM//////////////////////////////////////////////////////

//TODO lo de sincronizar la copia de la copia usando el tiempo de sincronizacion de config

///////////////COMIENZO DE FUNCIONES PARA REALIZAR TAREAS///////////////////////////////////////////////////////////////////

//Genera la cantidad dada por cantidad del recurso dado por codigo_recurso
void recurso_generar_cantidad(recurso_code codigo_recurso, int cantidad)
{
	if(recurso_es_valido(codigo_recurso))
	{
		t_recurso_data* recurso_data = &lista_recursos[codigo_recurso];
		//log_debug(logger, "Info-Se ingresa a recurso_generar_cantidad, se debe generar %d %s(s)", cantidad, recurso_data->nombre);
		if(cantidad <= 0)
		{
			log_error(logger, "La cantidad a generar debe ser mayor a 0 para que haya cambios en el sistema");
			return;
		}
		//wait recurso_data//se accede al archivo del recurso solo para levantar los datos
		recurso_validar_existencia_metadata_en_memoria(recurso_data);
		//wait superbloque y blocks
		metadata_generar_cantidad(recurso_data->metadata, cantidad);

		superbloque_actualizar_bitmap_en_archivo();
		blocks_actualizar_archivo();
		recurso_actualizar_archivo(recurso_data);
		//signal recurso_data superbloque y blocks //quiza se puede ser mas especifico

		log_info(logger, "Finaliza recurso_generar_cantidad de %s", recurso_data->nombre);
		//verificar_superbloque_temporal(); //borrar
	}
	else
	{
		log_info(logger, "El recurso indicado (%d) no es valido, prueba otra vez", codigo_recurso);
	}
}

//Devuelve un valor entero distinto de cero en caso de que el codigo_recurso este entre 0 y la cantidad de recursos
int recurso_es_valido(recurso_code codigo_recurso)
{
	return (codigo_recurso >= 0 && codigo_recurso <= files_cantidad_recursos());
}

//Si no existe la metadata en memoria, la genera con los valores del archivo o con sus valores default si este ultimo tampoco estuviera
void recurso_validar_existencia_metadata_en_memoria(t_recurso_data* recurso_data)
{
	//log_debug(logger, "I-Entro a recurso_validar_existencia_metadata_en_memoria para el recurso %s", recurso_data->nombre);
	if(recurso_data->metadata == NULL)
	{
		recurso_data->metadata = malloc(sizeof(t_recurso_md));

		recurso_data->metadata->caracter_llenado = string_from_format("%c", recurso_data->caracter_llenado);

		if(utils_existe_en_disco(recurso_data->ruta_completa))
		{
			metadata_levantar_de_archivo_a_memoria_valores_variables(recurso_data->metadata, recurso_data->ruta_completa);
		}
		else
		{
			metadata_setear_con_valores_default_en_memoria(recurso_data->metadata);
		}
	}
	//log_debug(logger, "O-Existencia de metadata para el recurso %s validada", recurso_data->nombre);
}

//Levanta a la estructura metadata en memoria los valores variables contenidos en el archivo de metadata correspondiente
void metadata_levantar_de_archivo_a_memoria_valores_variables(t_recurso_md* recurso_md, char* recurso_path)
{
	//log_debug(logger, "I-Se ingresa a metadata_levantar_de_archivo_a_memoria para el recurso con ruta %s", recurso_path);
	t_config* recurso_config = config_create(recurso_path);

	recurso_md->size = config_get_int_value(recurso_config, "SIZE");
	recurso_md->block_count = config_get_int_value(recurso_config, "BLOCK_COUNT");;
	recurso_md->blocks = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));
	strcpy(recurso_md->md5_archivo, config_get_string_value(recurso_config, "MD5_ARCHIVO"));

	config_destroy(recurso_config);

	log_debug(logger, "Valores variables levantados SIZE %d BLOCK_COUNT %d BLOCKS %s y MD5_ARCHIVO %s. Valor fijo CARACTER_LLENADO %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo, recurso_md->caracter_llenado);
	//log_debug(logger, "O-Metadata levantada a memoria desde archivo %s", recurso_path);
}

//Setea en memoria los valores variables de la metadata del recurso dado con los valores default
void metadata_setear_con_valores_default_en_memoria(t_recurso_md* recurso_md)
{
	//log_debug(logger, "I-Se ingresa a recurso_inicializar_metadata_en_memoria para el recurso con caracter de llenado %s", recurso_md->caracter_llenado);
	recurso_md->size = 0;
	recurso_md->block_count = 0;
	recurso_md->blocks = string_duplicate("[]");
	strcpy(recurso_md->md5_archivo, "d41d8cd98f00b204e9800998ecf8427e");

	log_debug(logger, "Valores default seteados en memoria: SIZE %d BLOCK_COUNT %d BLOCKS %s CARACTER_LLENADO %s y MD5_ARCHIVO %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->caracter_llenado, recurso_md->md5_archivo);
}

//Genera el recurso de la metadata dada guardando los cambios en esta misma tanto como en la copia de blocks en memoria
void metadata_generar_cantidad(t_recurso_md* recurso_md, int cantidad)
{
	//log_debug(logger, "Info-Entro a metadata_generar_cantidad con C_LL %s y cantidad %d", recurso_md->caracter_llenado, cantidad);
	log_debug(logger, "En metadata_generar_cantidad: cantidad %d SIZE %d, BLOCKS %s",
			cantidad, recurso_md->size, recurso_md->blocks);
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
	log_debug(logger, "Saliendo de metadata_generar_cantidad con C_LL %s, SIZE %d, BLOCKS %s y el MD5 %s", recurso_md->caracter_llenado,
				recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
}

//Verifica si el ultimo bloque usado por el recurso tiene espacio disponible
int metadata_tiene_espacio_en_ultimo_bloque(t_recurso_md* recurso_md)
{
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	log_debug(logger, "En metadata_tiene_espacio_en_ultimo_bloque con SIZE %d, BLOCKS %s y superbloque.block_size %d: (%d)",
			recurso_md->size, recurso_md->blocks, superbloque.block_size, recurso_md->size < cantidad_bloques * superbloque.block_size);
	return (recurso_md->size < cantidad_bloques * superbloque.block_size);
}

//Calcula la cantidad de elementos de una lista en formato de cadena de texto. Ej: "[algo, y, otro, algo]" => 4 o "[4,6]" => 2
int cadena_cantidad_elementos_en_lista(char* cadena)
{
	char** lista = string_get_string_as_array(cadena);
	//int posicion; for(posicion = 0; lista[posicion] != NULL; posicion++); //alter
	int posicion = -1;
	while(lista[++posicion] != NULL);
	cadena_eliminar_array_de_cadenas(&lista, posicion);

	//log_debug(logger, "IO-En cadena_cantidad_elementos_en_lista, la cantidad de elementos en cadena %s es %d", cadena, posicion);
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

//Devuelve el ultimo bloque registrado en la lista de Blocks del metadata del recurso
int metadata_ultimo_bloque_usado(t_recurso_md* recurso_md)
{
	log_debug(logger, "I-Ingreso a metadata_ultimo_bloque_usado con BLOCKS %s", recurso_md->blocks);

	int ultimo_bloque = cadena_ultimo_entero_en_lista_de_enteros(recurso_md->blocks);

	verificar_superbloque_temporal(); //borrar
	return ultimo_bloque;
}

//Devuelve el ultimo entero de una lista de enteros
int cadena_ultimo_entero_en_lista_de_enteros(char* lista_de_enteros)
{
	log_debug(logger, "I-Ingreso a cadena_ultimo_entero_en_lista_de_enteros(char* lista_de_enteros) %s", lista_de_enteros);

	char** lista = string_get_string_as_array(lista_de_enteros);

	int cantidad_enteros = cadena_cantidad_elementos_en_lista(lista_de_enteros);
	//log_info(logger, "cantidad_enteros %d", cantidad_enteros);

	if(cantidad_enteros == 0){

	}
	int ultimo_entero = atoi(lista[cantidad_enteros - 1]);

	//log_info(logger, "ultimo_entero %d", ultimo_entero);

	cadena_eliminar_array_de_cadenas(&lista, cantidad_enteros);

	log_debug(logger, "O-Ultimo_entero usado %d", ultimo_entero);

	return ultimo_entero;
}

//Carga recursos en el bloque dado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_parcialmente_bloque(t_recurso_md* recurso_md, int* cantidad, int bloque)
{
	log_debug(logger, "I-Se ingresa a metadata_cargar_parcialmente_en_bloque con caracter %s, cantidad %d y bloque %d", recurso_md->caracter_llenado, *cantidad, bloque);

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
	log_debug(logger, "I-Se ingresa a superbloque_obtener_bloque_libre");

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
		log_debug(logger, "O-Bloque libre obtenido %d", posicion);
	}

	bitarray_destroy(bitmap);
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
	log_debug(logger, "Entero %d agregado a lista_de_enteros quedando %s", *lista_de_enteros);
}

//Saca el ultimo caracter a una cadena de caracteres redimensionando el espacio de memoria
void cadena_sacar_ultimo_caracter(char* cadena)
{
	int tamanio_final_cadena = strlen(cadena);
	cadena[tamanio_final_cadena - 1] = '\0';
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

//Calcula y actualiza el valor del md5 del archivo teniendo en cuenta el conjunto de bloques que ocupa el recurso en el archivo de Blocks.ims
void metadata_actualizar_md5(t_recurso_md* recurso_md)
{
	log_debug(logger, "I-Se ingresa a metadata_actualizar_md5");

	char* concatenado;
	char md5_str[33];

	if(recurso_md->size > 0)
	{
		if(blocks_obtener_concatenado_de_recurso(recurso_md, &concatenado) == 0)
		{
			cadena_calcular_md5(concatenado, recurso_md->size, md5_str);
			strcpy(recurso_md->md5_archivo, md5_str);
		}
		else
		{
			log_error(logger, "No hay memoria suficiente como para calcular el MD5 del recurso con caracter de llenado %s", recurso_md-> caracter_llenado);
		}
		free(concatenado);
	}
	else
	{
		strcpy(recurso_md->md5_archivo, "d41d8cd98f00b204e9800998ecf8427e");
	}
	log_debug(logger, "O-MD5 en metadata actualizado a %s", recurso_md->md5_archivo);
}

//Obtiene el conjunto de bloques concatenado que el recurso esta ocupando en el archivo de Blocks.ims devolviendo el tamanio del mismo
int blocks_obtener_concatenado_de_recurso(t_recurso_md* recurso_md, char** concatenado)
{
	log_debug(logger, "I-Se ingresa a blocks_obtener_concatenado_de_recurso");

	*concatenado = malloc(recurso_md->size);
	if(*concatenado == NULL)
	{
		return ERROR; //Valor de retorno con error manejado en la funcion que hace el llamado
	}
	else
	{
		int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
		int i, indice_ultimo_bloque = cantidad_bloques - 1; //Indice en base a la cantidad de bloques de la lista en el recurso
		int bloque;//Numero de bloque en base al total de bloques del sistema
		char** lista_bloques = string_get_string_as_array(recurso_md->blocks);
		char* posicion_desde; //Posicion de la copia de Blocks.ims desde la que arranca el bloque que se copiara
		char* posicion_hacia = *concatenado; //Posicion del espacio de memoria donde se almacenara cada bloque con memcpy

		for(i = 0; i < indice_ultimo_bloque; i++)
		{
			bloque = atoi(lista_bloques[i]);
			posicion_desde = blocks_address + bloque * superbloque.block_size;
			memcpy(posicion_hacia, posicion_desde, superbloque.block_size);
			posicion_hacia += superbloque.block_size;
		}
		bloque = atoi(lista_bloques[i]);
		posicion_desde = blocks_address + bloque * superbloque.block_size;
		int cantidad_en_ultimo_bloque = recurso_md->size % superbloque.block_size;
		memcpy(posicion_hacia, posicion_desde, cantidad_en_ultimo_bloque);
		cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);
	}

	log_debug(logger, "O-Total de caracteres del recurso concatenados en direccion de memoria %p", *concatenado);
	return OK;
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

//Sincroniza de manera forzosa el archivo de Blocks.ims con el espacio de memoria mapeado
void superbloque_actualizar_bitmap_en_archivo()
{
	//log_debug(logger, "I-Se ingresa a superbloque_actualizar_bitmap_en_archivo address");// %p", bitmap_address);
	//msync(bitmap_address, bitmap_size, MS_SYNC);
	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
	lseek(superbloque_fd, 2 * sizeof(uint32_t), SEEK_SET);
	write(superbloque_fd, superbloque.bitmap, bitmap_size);
	close(superbloque_fd);
	log_debug(logger, "O-Bitmap en archivo SuperBloque.ims actualizado con los valores de la memoria");
}

//Sincroniza de manera forzosa el archivo de Blocks.ims con el espacio de memoria mapeado
void blocks_actualizar_archivo()
{
	//log_debug(logger, "I-Se ingresa a blocks_actualizar_archivo");
	msync(blocks_address, blocks_size, MS_SYNC);
	log_debug(logger, "O-Archivo Blocks.ims actualizado con los valores de la memoria");

}

//Actualiza la metadata del recurso en memoria al archivo. Si este no existe lo crea con los valores actualizados
void recurso_actualizar_archivo(t_recurso_data* recurso_data)
{
//	log_debug(logger, "Info-Se ingresa a recurso_actualizar_archivo de recurso %s", recurso_data->nombre);
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
	log_info(logger, "Se actualizo metadata de %s de memoria a archivo %s", recurso_data->nombre, recurso_data->ruta_completa);
}

//Descartar

//Descarta la metadata y el archivo correspondiente del recurso dado por parametro siempre y cuando la cantidad recibida sea igual a 0
void recurso_descartar_cantidad(recurso_code codigo_recurso, int cantidad)
{
	t_recurso_data* recurso_data = &lista_recursos[codigo_recurso];
	log_debug(logger, "Info-Se ingresa a recurso_descartar_cantidad para el recurso %s y la cantidad %d", recurso_data->nombre, cantidad);

	if(cantidad == 0)
	{
		recurso_validar_existencia_metadata_en_memoria(recurso_data);

		if(utils_existe_en_disco(recurso_data->ruta_completa) == 0)
		{
			log_info(logger, "No existe el archivo %s como para descartarlo", recurso_data->nombre_archivo);
		}
		else
		{
			remove(recurso_data->ruta_completa);
			log_info(logger, "Se elimino el archivo de la metadata del recurso %s", recurso_data->nombre);
			if(metadata_tiene_caracteres_en_blocks(recurso_data->metadata))
			{
				metadata_descartar_caracteres_existentes(recurso_data->metadata);
				superbloque_actualizar_bitmap_en_archivo();
				blocks_actualizar_archivo();
			}
		}
		log_debug(logger, "Se seteo a los valores default la metadata en memoria del recurso %s", recurso_data->nombre);
	}
	else
	{
		log_info(logger, "La cantidad debería ser igual a 0 al querer descartar el recurso %s", recurso_data->nombre);
	}
}

//Devuelve un valor distinto de 0 si segun la lista de blocks del recurso este tiene caracteres en Blocks
int metadata_tiene_caracteres_en_blocks(t_recurso_md* recurso_md)
{
	return cadena_cantidad_elementos_en_lista(recurso_md->blocks);
}

//Descarta del recurso todos los caracteres y bloques, registrados en Blocks, en la metadata y en el superbloque
void metadata_descartar_caracteres_existentes(t_recurso_md* recurso_md)
{
	superbloque_liberar_bloques_en_bitmap(recurso_md->blocks);
	blocks_eliminar_bloques(recurso_md->blocks);
	free(recurso_md->blocks);
	log_debug(logger, "Se descartaron los caracteres \"%s\" que existian", recurso_md->caracter_llenado);
	metadata_setear_con_valores_default_en_memoria(recurso_md);
}

//Setea en el bitmap del superbloque las posiciones dadas por la cadena blocks a 0
void superbloque_liberar_bloques_en_bitmap(char* blocks)
{
	//log_debug(logger, "I-Ingreso a superbloque_liberar_bloques_en_bitmap con BLOCKS %s", blocks);
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(blocks);
	int bloque;
	char** lista_bloques = string_get_string_as_array(blocks);
	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, bitmap_size, LSB_FIRST);

	for(int i = 0; i < cantidad_bloques; i++)
	{
		bloque = atoi(lista_bloques[i]);
		bitarray_clean_bit(bitmap, bloque);
	}

	bitarray_destroy(bitmap);
	cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);
	log_debug(logger, "O-Bloques liberados en bitmap del superbloque");
}

//Registra un caracter de fin de cadena al comienzo de cada uno de los bloques incluidos en la lista de bloques
void blocks_eliminar_bloques(char* blocks)
{
	log_debug(logger, "I-blocks_eliminar_bloques con BLOCKS %s", blocks);

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(blocks);
	char** lista_bloques = string_get_string_as_array(blocks);
	int bloque;

	for(int i = 0; i < cantidad_bloques; i++)
	{
		bloque = atoi(lista_bloques[i]);
		blocks_eliminar_bloque(bloque);
	}
	cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);
	log_debug(logger, "O-Bloques eliminados - version rapida//version lenta");
}

//Registra un caracter de fin de cadena al comienzo de cada uno de los bloques incluidos en la lista de bloques
void blocks_eliminar_bloque(int bloque)
{
	log_debug(logger, "I-blocks_eliminar_bloque con bloque %d", bloque);

	int posicion_inicio_bloque = bloque * superbloque.block_size;
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque;
	*posicion_absoluta = '\0';
	//memset(posicion_absoluta, 0, superbloque.block_size);

	log_debug(logger, "O-Bloque eliminado - version rapida//version lenta");
}

/*////////COMIENZO DE NO TERMINADO 2
//Elimina la cantidad cantidad de caracteres de llenado del archivo y de la metadata correspondiente al recurso dado
void recurso_consumir_cantidad(recurso_code codigo_recurso, int cantidad)
{
	t_recurso_data* recurso_data = &lista_recursos[codigo_recurso];
	log_debug(logger, "Se ingresa a recurso_consumir_cantidad para el recurso %s y la cantidad %d", recurso_data->nombre, cantidad);
	recurso_validar_existencia_metadata_en_memoria(recurso_data);
	metadata_consumir_cantidad(recurso_data->metadata, cantidad);
	superbloque_actualizar_archivo();
	blocks_actualizar_archivo();
	recurso_actualizar_archivo(recurso_data);
	//signal recurso_data superbloque y blocks //quiza se puede ser mas especifico
	if(recurso_data->metadata == NULL)
	{
		int bloques_llenos_a_eliminar = cantidad / superbloque.block_size;
		int resto_de_caracteres_a_eliminar = cantidad % superbloque.block_size;
	}
	log_info(logger, "Finaliza recurso_consumir_cantidad de %s", recurso_data->nombre);
}
//Actualiza la metadata del recurso en memoria al archivo. Si este no existe lo crea con los valores actualizados
void recurso_actualizar_archivo(t_recurso_data* recurso_data)//para modificar
{
	log_debug(logger, "Se ingresa a recurso_actualizar_archivo de recurso %s", recurso_data->nombre);
	if(existe_en_disco(recurso_data->ruta_completa) == 0)
	{
		crear_archivo(recurso_data->ruta_completa);
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
	log_info(logger, "Se actualizo metadata de %s de memoria de a archivo %s", recurso_data->nombre, recurso_data->ruta_completa);
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
		free(recurso_md->blocks);
		free(recurso_md->md5_archivo);
		metadata_setear_con_valores_default_en_memoria(recurso_md);
		log_info(logger, "Quisieron eliminarse mas caracteres \"%s\" de los existentes");
	}
	while(cantidad > 0)
	{
		if(metadata_tiene_espacio_en_ultimo_bloque(recurso_md))
		{
			metadata_consumir_en_ultimo_bloque(recurso_md, &cantidad, nuevo_bloque);
		}
		else
		{
			log_debug(logger, "En cargar_recursos_en_memoria (while-else 1): cantidad %d, blocks %s, recurso_md->size %d y "
					"superbloque.block_size %d", cantidad, recurso_md->blocks, recurso_md->size, superbloque.block_size);
			nuevo_bloque = superbloque_obtener_bloque_nuevo_a_usar();
			cadena_sacar_ultimo_caracter(recurso_md->blocks);
			log_debug(logger, "Cadena despues de sacarle el ultimo caracter %s", recurso_md->blocks);
			string_append_with_format(&recurso_md->blocks, ",%d]", nuevo_bloque);
			recurso_md->block_count++;
			log_debug(logger, "En cargar_recursos_en_memoria (while-else 2): blocks %s y recurso_md->block_count  %d",
					recurso_md->blocks, recurso_md->block_count);
			}
	}
	metadata_actualizar_md5(recurso_md);
	log_info(logger, "Se genero la cantidad solicitada teniendo finalmente el recurso con C_LL %s, SIZE %s, BLOCKS %s y el MD5 %s",
			recurso_md->caracter_llenado, recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
}
//Carga recursos en el ultimo bloque registrado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_consumir_en_ultimo_bloque(t_recurso_md* recurso_md, int* cantidad, int nuevo_bloque)
{
	log_debug(logger, "Se ingresa a recurso_cconsumir_en_ultimo_bloque con caracter %s y cantidad %d", recurso_md->caracter_llenado, cantidad);
	//Si el valor del nuevo_bloque no es el valor default es porque ese bloque es el nuevo ultimo de la lista y no necesita calcularse
	int ultimo_bloque = nuevo_bloque != -1 ? nuevo_bloque : metadata_ultimo_bloque_usado(recurso_md);
	int posicion_en_bloque = recurso_md->size % superbloque.block_size;
	int posicion_inicio_bloque = ultimo_bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	//La posicion absoluta es la suma entre la posicion de inicio del conjunto de bloques mas la posicion de inicio del bloque a cargar
	//relativa al inicio de bloques mas la posicion en el bloque relativa al inicio del bloque
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque; //esta ok sumar un puntero con 2 ints?
	log_debug(logger, "Ultimo_bloque %d, posicion_en_bloque %d, posicion_inicio_bloque %d posicion absoluta %p o %d",
			*cantidad, ultimo_bloque, posicion_en_bloque, posicion_inicio_bloque, posicion_absoluta, posicion_absoluta);
	char caracter_llenado = recurso_md->caracter_llenado[0];
	while(posicion_en_bloque < superbloque.block_size && (*cantidad) > 0)
	{
		*posicion_absoluta = caracter_llenado;
		(*cantidad)--;
		recurso_md->size++;
		posicion_absoluta++;
		posicion_en_bloque++;
	}
	log_debug(logger, "Se cargo en ultimo bloque con caracter %s quedando por cargar %d recurso(s)", recurso_md->caracter_llenado, cantidad);
}
//Carga recursos en el ultimo bloque registrado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_consumir_en_ultimo_bloque2(t_recurso_md* recurso_md, int* cantidad, int nuevo_bloque)
{
	log_debug(logger, "Se ingresa a recurso_cargar_en_ultimo_bloque con caracter %s y cantidad %d", recurso_md->caracter_llenado, cantidad);
	//Si el valor del nuevo_bloque no es el valor default es porque ese bloque es el nuevo ultimo de la lista y no necesita calcularse
	int ultimo_bloque = nuevo_bloque != -1 ? nuevo_bloque : metadata_ultimo_bloque_usado(recurso_md);
	int posicion_en_bloque = recurso_md->size % superbloque.block_size;
	int posicion_inicio_bloque = ultimo_bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque; //esta ok sumar un puntero con 2 ints?
	log_debug(logger, "Ultimo_bloque %d, posicion_en_bloque %d, posicion_inicio_bloque %d posicion absoluta %p o %d",
			*cantidad, ultimo_bloque, posicion_en_bloque, posicion_inicio_bloque, posicion_absoluta, posicion_absoluta);
	char caracter_llenado = recurso_md->caracter_llenado[0];
	int espacio_libre_en_bloque = superbloque.block_size - posicion_en_bloque;
	int cantidad_a_cargar = espacio_libre_en_bloque < *cantidad ? espacio_libre_en_bloque : *cantidad;
	*cantidad -= cantidad_a_cargar;
	memset(posicion_absoluta, caracter_llenado, cantidad_a_cargar);
	recurso_md->size += cantidad_a_cargar;
	log_debug(logger, "Se cargo en ultimo bloque con caracter %s quedando por cargar %d recurso(s)", recurso_md->caracter_llenado, cantidad);
}
//Devuelve el ultimo bloque registrado en la lista de Blocks del metadata del recurso
int metadata_ultimo_bloque_usado(t_recurso_md* recurso_md)
{
	log_debug(logger, "Ingreso a metadata_ultimo_bloque_usado con BLOCKS %s", recurso_md->blocks);
	char** lista_bloques = malloc(sizeof(string_get_string_as_array(recurso_md->blocks)));
	lista_bloques = string_get_string_as_array(recurso_md->blocks);
	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	int ultimo_bloque = atoi(lista_bloques[cantidad_bloques - 1]); //Probar strtol(data, NULL, 10);
	//puede hacer falta liberar uno por uno todos los bloques con un for(int i = 0; i < cantidad_bloques; i++)
	free(lista_bloques);
	log_debug(logger, "Ultimo_bloque usado %d", ultimo_bloque);
	return ultimo_bloque;
}
/////////FIN DE NO TERMINADO 2
 */


//FUNCIONES SUJETAS A COMO RECIBIMOS LOS DATOS DEL DISCORDIADOR
void recurso_realizar_tarea()
{
	int accion, recurso, cantidad;
	tomar_accion_recurso_y_cantidad(&accion, &recurso, &cantidad); //Funcion que despues tendria que tomar los valores desde el discordiador

	switch(accion)
	{
		case GENERAR:
			recurso_generar_cantidad(recurso, cantidad);
			break;
		case CONSUMIR:
			printf("Not yet pero ya casi (EN DESARROLLO)\n");//recurso_consumir_cantidad(recurso, cantidad);
			break;
		case DESCARTAR:
			recurso_descartar_cantidad(recurso, cantidad);
			break;
		default:
			printf("A esa accion todavia no la tengo, mejor proba de nuevo\n");
	}
}

void tomar_accion_recurso_y_cantidad(int* accion, int* recurso, int* cantidad)
{
	printf("Elegi que accion vas a realizar (0 para generar, 1 para consumir, 2 para descartar)\n");
	scanf("%d", accion);
	printf("Elegi para cual recurso (0 para oxigeno, 1 para comida, 2 para basura)\n");
	scanf("%d", recurso);
	printf("Elegi con que cantidad\n");
	scanf("%d", cantidad);
}

//FUNCIONES AUN EN DESARROLLO
//TODO Ver bien (con el video) que corresponde verificar y que no de los archivos, y/o inicializar la memoria con los datos de los archivos directamente

//Inicia la secuencia de chequeos y reparaciones del File System Catastrophe Killer
void fsck_iniciar()
{
	fsck_chequeo_de_sabotajes_en_superbloque();
	fsck_chequeo_de_sabotajes_en_files();
}

//Realiza los chequeos propios del superbloque (en campos blocks y bitmap)
void fsck_chequeo_de_sabotajes_en_superbloque()
{
	superbloque_validar_integridad_cantidad_de_bloques();
	superbloque_validar_integridad_bitmap();
}

//Valida que la cantidad de bloques en el archivo Superbloque.ims se corresponda con la cantidad real en el recurso fisico de Blocks.ims
void superbloque_validar_integridad_cantidad_de_bloques()
{
	struct stat *blocks_stat;
	if(stat(blocks_path, blocks_stat) != 0)
	{
		log_error(logger, "No se pudo acceder a la informacion del archivo Blocks.ims, por lo tanto no podra validarse si hubo un sabotaje a la cantidad de bloques");
		exit(EXIT_FAILURE);
	}
	//blocks_size es tamanio del archivo de Blocks.ims a diferencia de block_size que es el tamanio de cada unidad de block
	int blocks_size_real = blocks_stat->st_size; //Tamanio real del archivo //no se podria usar directamente blocks_size??

	int desplazamiento = 0;
	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.block_size, sizeof(uint32_t));//aca lo mismo, la estructura en memoria no pudo ser alterada, no se podria directamente tomar sus datos??
	desplazamiento += sizeof(uint32_t);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));

	int blocks_real = blocks_size_real / superbloque.block_size;

	if(superbloque.blocks != blocks_real)
	{
		superbloque.blocks = blocks_real;
		lseek(superbloque_fd, desplazamiento, SEEK_SET);
		write(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));
		close(superbloque_fd);
		log_info(logger, "Alguien saboteo la cantidad de bloques en el SuperBloque.ims, pero no te preocupes, ya esta arreglado");
	}
	else
	{
		close(superbloque_fd);
		log_info(logger, "Parece que esta vez nadie se animo a sabotear la cantidad de bloques en el SuperBloque.ims");
	}
}

//Sincroniza de manera forzosa SuperBloque.ims con el espacio de memoria mapeado
void superbloque_actualizar_blocks_en_archivo()
{
	//log_debug(logger, "I-Se ingresa a superbloque_actualizar_blocks_en_archivo");
	int superbloque_fd = utils_abrir_archivo_para_lectura_escritura(superbloque_path);

	lseek(superbloque_fd, sizeof(uint32_t), SEEK_SET);
	write(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));

	close(superbloque_fd);
	//verificar_superbloque_temporal(); //borrar
	log_debug(logger, "O-Blocks en archivo SuperBloque.ims actualizado");
}

//Valida que los bloques indicados en el bitmap del superbloque realmente sean los unicos ocupados
void superbloque_validar_integridad_bitmap()
{
	log_debug(logger, "Info-Se ingresa a superbloque_validar_integridad_bitmap");
	char* bloques_ocupados = files_obtener_cadena_con_el_total_de_bloques_ocupados();

	bitmap_size = 1 + (superbloque.blocks - 1) / CHAR_BIT; //Innecesario seguramente porque se supone que la estructura superbloque no puede corromperse
	superbloque_setear_bitmap_a_cero();
	superbloque_setear_bloques_en_bitmap(bloques_ocupados);
	superbloque_actualizar_bitmap_en_archivo();

	free(bloques_ocupados);
	log_info(logger, "Se actualizo el bitmap en el archivo SuperBloque.ims ante un probable sabotaje a este");
}

//Devuelve una lista en formato cadena con los bloques ocupados por el total de archivos en Files (recursos + bitacoras)
char* files_obtener_cadena_con_el_total_de_bloques_ocupados()
{
	log_debug(logger, "Se ingresa a files_obtener_cadena_con_bloques_ocupados");

	char* bloques_de_recursos = files_obtener_cadena_con_bloques_ocupados_por_recursos();
	char* bloques_de_bitacoras = files_obtener_cadena_con_bloques_ocupados_por_bitacoras();

	char* bloques_ocupados = bloques_de_recursos;
	cadena_sacar_ultimo_caracter(bloques_ocupados);
	bloques_de_bitacoras[0] = ',';
	string_append(&bloques_ocupados, bloques_de_bitacoras);

	free(bloques_de_recursos);
	free(bloques_de_bitacoras);

	log_debug(logger, "El total de bloques ocupados es %s", bloques_ocupados);
	return bloques_ocupados;
}

//Devuelve una lista en formato cadena con los bloques ocupados por el total de recursos
char* files_obtener_cadena_con_bloques_ocupados_por_recursos()
{
	log_debug(logger, "Se ingresa a files_obtener_cadena_con_bloques_ocupados_por_recursos");

	int cantidad_recursos = files_cantidad_recursos();
	int posicion_ultimo_recurso = cantidad_recursos - 1;
	//char* bloques = string_duplicate(lista_recursos[0].metadata->blocks);//Se podría usar la informacion de los recursos en memoria??
	char* bloques = files_obtener_bloques_de_archivo_metadata(lista_recursos[0].ruta_completa);

	for(int i = 1; i < cantidad_recursos; i++)
	{
		cadena_sacar_ultimo_caracter(bloques);
		char* string_buffer = string_duplicate(lista_recursos[i].metadata->blocks);
		string_buffer[0] = ',';
		string_append(&bloques, string_buffer);
		free(string_buffer);
	}

	log_debug(logger, "Los bloques ocupados por el total de recursos son %s"), bloques;
	return bloques;
}

//Devuelve una lista en formato cadena con los bloques indicados por la key "BLOCKS" en el archivo metadata de la ruta path
char* files_obtener_bloques_de_archivo_metadata(char* path)
{
	t_config* path_config = config_create(path);
	char* bloques = string_duplicate(config_get_string_value(path_config, "BLOCKS"));
	config_destroy(path_config);

	return bloques;
}

//FIXME (en base a la forma de recorrer las bitacoras) //Devuelve una lista en formato cadena con los bloques ocupados por el total de bitacoras
char* files_obtener_cadena_con_bloques_ocupados_por_bitacoras()
{
	log_debug(logger, "Se ingresa a files_obtener_cadena_con_bloques_ocupados_por_bitacoras");

	int cantidad_bitacoras = files_cantidad_bitacoras(); //tal vez sirva
	int posicion_ultimo_bitacora = cantidad_bitacoras - 1; //tal vez sirva
	char* tripulante_path = string_from_format("%s/%s", bitacoras_path, "Tripulante1.ims");
	char* bloques = files_obtener_bloques_de_archivo_metadata(tripulante_path);

	for(int i = 1; i < cantidad_bitacoras; i++)
	{
		cadena_sacar_ultimo_caracter(bloques);
		char* string_buffer = string_duplicate("otraBitacora");
		string_buffer[0] = ',';
		string_append_with_format(&bloques, "%s", string_buffer);
		free(string_buffer);
	}

	log_debug(logger, "Los bloques ocupados por el total de bitacoras son %s"), bloques;
	return bloques;
}

int files_cantidad_bitacoras()
{

	return 1;
}

//Setea al bitmap apuntado por el campo bitmap de la estructura superbloque segun los bloques ocupados que recibe como parametro
void superbloque_setear_bloques_en_bitmap(char* bloques_ocupados)
{
	log_debug(logger, "Ingreso a superbloque_setear_bloques_en_bitmap");

	char** lista_bloques = string_get_string_as_array(bloques_ocupados);
	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, bitmap_size, LSB_FIRST);

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(bloques_ocupados);

	int bloque;
	for(int i = 0; i < cantidad_bloques; i++)
	{
		bloque = atoi(lista_bloques[i]);
		bitarray_set_bit(bitmap, bloque);
	}

	cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);
	bitarray_destroy(bitmap);

	log_debug(logger, "Bitmap del SuperBloque actualizado en memoria");
}

//Realiza los chequeos propios de los recursos
void fsck_chequeo_de_sabotajes_en_files()
{
	int cantidad_recursos = files_cantidad_recursos();

	for(int i = 0; i < cantidad_recursos; i++)
	{
		t_recurso_data recurso_data = lista_recursos[i];
		recurso_validar_size(&recurso_data);
		recurso_validar_block_count(&recurso_data);
		recurso_validar_blocks(&recurso_data);
		recurso_validar_existencia_metadata_en_memoria(&recurso_data);
	}
}

void recurso_validar_size(t_recurso_data* recurso_data)
{
	int size_real = recurso_obtener_size_real(recurso_data);//files_obtener_bloques_ocupados()
	t_config* recurso_config = config_create(recurso_data->ruta_completa);
	int size_en_archivo = config_get_int_value(recurso_config, "SIZE");

	if(size_real != size_en_archivo)
	{
		log_info(logger, "El recurso %s tiene su size saboteado, pero no te preocupes, ya lo arreglaremos", recurso_data->nombre);
		log_debug(logger, "Size en archivo %d size real %d", size_en_archivo, size_real);

		char* size_real_str = string_itoa(size_real);
		config_set_value(recurso_config, "SIZE", size_real_str);
		free(size_real_str);

		log_info(logger, "Listo, ya podes confiar en el size del recurso %s", recurso_data->nombre);
	}
	else
	{
		log_info(logger, "Por lo menos el size del recurso %s no fue alterado, enhorabuena!!!", recurso_data->nombre);
	}

	config_destroy(recurso_config);
}

int recurso_obtener_size_real(t_recurso_data* recurso_data)
{
	return 1;//TODO
}

void recurso_validar_block_count(t_recurso_data* recurso_data)
{
	int block_count_real = recurso_obtener_block_count_real(recurso_data);
	t_config* recurso_config = config_create(recurso_data->ruta_completa);
	int block_count_en_archivo = config_get_int_value(recurso_config, "BLOCK_COUNT");

	if(block_count_real != block_count_en_archivo)
	{
		log_info(logger, "El recurso %s tiene su block_count saboteado, pero no te preocupes, ya lo arreglaremos", recurso_data->nombre);
		log_debug(logger, "Block_count en archivo %d block_count real %d", block_count_en_archivo, block_count_real);

		char* block_count_real_str = string_itoa(block_count_real);
		config_set_value(recurso_config, "BLOCK_COUNT", block_count_real_str);
		free(block_count_real_str);

		log_info(logger, "Listo, ya podes confiar en el block_count del recurso %s", recurso_data->nombre);
	}
	else
	{
		log_info(logger, "Por lo menos el block_count del recurso %s no fue alterado, enhorabuena!!!", recurso_data->nombre);
	}

	config_destroy(recurso_config);
}

int recurso_obtener_block_count_real(t_recurso_data* recurso_data)
{
	return 1;//TODO
}

void recurso_validar_blocks(t_recurso_data* recurso_data)
{
	char* blocks_real = recurso_obtener_blocks_real(recurso_data);
	t_config* recurso_config = config_create(recurso_data->ruta_completa);
	char* blocks_en_archivo = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));

	if(strcmp(blocks_real, blocks_en_archivo) != 0)
	{
		log_info(logger, "El recurso %s tiene su blocks saboteado, pero no te preocupes, ya lo arreglaremos", recurso_data->nombre);
		log_debug(logger, "Blocks en archivo %s blocks real %s", blocks_en_archivo, blocks_real);

		config_set_value(recurso_config, "BLOCKS", blocks_real);

		log_info(logger, "Listo, ya podes confiar en el blocks del recurso %s", recurso_data->nombre);
	}
	else
	{
		log_info(logger, "Por lo menos el blocks del recurso %s no fue alterado, enhorabuena!!!", recurso_data->nombre);
	}

	free(blocks_real);
	config_destroy(recurso_config);
}

//TODO Obtiene la lista de blocks real del recurso basado en el archivo de blocks (en consumir y descartar puede ponerse en 0 el primer elemento de cada bloque descartado para poder chequear en esta funcion)
char* recurso_obtener_blocks_real(t_recurso_data* recurso_data)
{
	char* algoLoco = malloc(3);
	strcpy(algoLoco, "[]");
	return algoLoco;
}
