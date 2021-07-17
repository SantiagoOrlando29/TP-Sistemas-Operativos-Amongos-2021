//utils_mongostore.c 15072021 03.16
//TODO: Ver si es necesario almacenar las posiciones de los sabotajes con coordenadas
//TODO: Ver de donde sale IP
//msync: The implementation shall require that addr be a multiple of the page size as returned by sysconf().
//long int sysconf(int nombre); 	SC_PAGESIZE _SC_PAGE_SIZE El tamaño de una página (en bytes).
#include "utils_mongostore.h"

t_recurso_data lista_recursos[] = {
	{"OXIGENO", 'O', "Oxigeno.ims", NULL, NULL},
	{"COMIDA", 'C', "Comida.ims", NULL, NULL},
	{"BASURA", 'B', "Basura.ims", NULL, NULL}
};

////////////////////FUNCIONES DEL FILE SYSTEM PARA INICIARLO EL FILE SYSTEM//////////////////////////////////////////////////////////////

//Lee la configuracion contenida en el archivo de configuracion definicion y la carga en el sistema
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

    //Puede desecharse la estructura t_config ya que no se volvera a consultar ni se seteara ningun valor
    config_destroy(config);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//Crea el directorio indicado por path, en caso de no poder finaliza con error
void crear_directorio_si_no_existe(char* path)
{
	log_debug(logger, "Se ingresa a crear_directorio_si_no_existe para %s", path);

	//Si no existe el directorio lo crea con el mkdir y automaticamente pregunta si se creo correctamente, si no fue asi finaliza con error
	if (existe_en_disco(path) == 0 && mkdir(path, 0777) == -1)
	{
		log_error(logger, "No existia el directorio %s desde antes ni puede crearse. No pueden crearse el total de directorios del File System", path);
		exit(EXIT_FAILURE);
	}
}

//Verifica que un determinado path exista chequeando que sea accesible
int existe_en_disco(char* path)
{
	return (access(path, F_OK) == 0);
}

//Abre el archivo indicado por path para lectura/escritura retornando el file descriptor. Finaliza en caso de error
int abrir_archivo_para_lectura_escritura(char* path)
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
void crear_archivo(char* path)
{
	int file_descriptor;

	file_descriptor = open(path, O_CREAT|O_RDWR, 0777);
	if(file_descriptor == -1)
	{
		log_error(logger, "El archivo con ruta completa %s no pudo ser creado. Se finaliza el programa", path);
		exit(EXIT_FAILURE);
	}

	close(file_descriptor);
	log_info(logger, "Se creo el archivo dado por la ruta completa \"%s\"", path);
}

//Abre el archivo indicado por path para escritura retornando el puntero a FILE. Finaliza en caso de error
FILE* abrir_archivo_para_escritura(char* path)
{
	FILE* file = fopen(path, "w");

	if(file == NULL)
	{
		log_error(logger, "%s no pudo ser abierto para escritura. Se finaliza el programa", path);
		exit(EXIT_FAILURE);
	}

	return file;
}

//Da el tamanio indicado por length al archivo dado por la ruta path. Finaliza si no puede realizarse con exito
void dar_tamanio_a_archivo(char* path, size_t length)
{
	log_debug(logger, "Se ingresa a dar_tamanio_a_archivo");

	if(truncate(path, length) == -1)
	{
		log_error(logger, "No se puede asignar el tamanio %d al archivo de ruta %s", length, path);
		exit(EXIT_FAILURE);
	}

	log_debug(logger, "Se da el tamanio exitosamente a archivo con ruta %s", path);
}

//Calcula la cantidad de elementos de una lista en formato de cadena de texto. Ej: "[algo, y, otro, algo]" => 4 o "[4,6]" => 2
int cadena_cantidad_elementos_en_lista(char* cadena)
{
	return (sizeof(string_get_string_as_array(cadena)) / sizeof(char*));
}

//Saca el ultimo caracter a una cadena de caracteres redimensionando el espacio de memoria
void cadena_sacar_ultimo_caracter(char* cadena)
{
	log_debug(logger, "Cadena original %s", cadena);

	int tamanio_final_cadena = strlen(cadena);
	cadena = realloc(cadena, tamanio_final_cadena);
	cadena[tamanio_final_cadena - 1] = '\0';

	log_debug(logger, "Cadena despues de sacarle el ultimo caracter %s", cadena);
}

//Elimina un array con la cantidad de cadenas dada liberando la memoria
void cadena_eliminar_array_de_cadenas(char*** puntero_a_array_de_punteros, int cantidad_cadenas)
{
	log_debug(logger, "Entro a cadena_eliminar_array_de_cadenas");
	for(int i = 0; i < cantidad_cadenas; i++)
	{
		free((*puntero_a_array_de_punteros)[i]);
	}

	free(*puntero_a_array_de_punteros);
	*puntero_a_array_de_punteros = NULL;
	log_debug(logger, "Salgo de cadena_eliminar_array_de_cadenas");
}

//Calcula el valor de md5 de la cadena de texto apuntada por el puntero cadena y de tamanio length, y lo guarda en el char array apuntado por md5_str
//prec: md5_str debe ser un char array de 33 caracteres
void cadena_calcular_md5(const char *cadena, int length, char* md5_str)
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
        snprintf(&(md5_str[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
}

//Calcula el valor de md5 de la cadena de texto apuntada por el puntero cadena y de tamanio length, y devuelve el puntero a donde se almacena
char* cadena_calcular_md5_alter(const char *cadena, int length)
{
    MD5_CTX c;
    unsigned char digest[16];
    char *md5_str = (char*)malloc(33);

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
        snprintf(&(md5_str[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return md5_str;
}

///////////////////////////////////////////////////////////////////

//Inicia el File System restaurando a memoria los archivos existentes si los hubiera
void file_system_iniciar()
{
	log_debug(logger, "Comienza iniciar_file_system");

	file_system_generar_rutas_completas();
	file_system_crear_directorios_inexistentes(); //Esto se asegura de que ya esten o los crea

	superbloque_validar_existencia();
	blocks_validar_existencia();

	log_info(logger, "File System iniciado");
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
	int cantidad_recursos = sizeof(lista_recursos) / sizeof(t_recurso_data);

	for(int i = 0; i < cantidad_recursos; i++)
	{
		lista_recursos[i].ruta_completa = string_from_format("%s/%s", files_path, lista_recursos[i].nombre_archivo);
	}
}

//Crea las carpetas necesarias para persistir la informacion del File System
void file_system_crear_directorios_inexistentes()
{
	log_debug(logger, "Se ingresa a file_system_crear_directorios_inexistentes");

	crear_directorio_si_no_existe(configuracion.punto_montaje);
	crear_directorio_si_no_existe(files_path);
	crear_directorio_si_no_existe(bitacoras_path);

	log_info(logger, "Directorios del File System confirmados");
}

///////////////////////FUNCIONES DEL SUPERBLOQUE RELATIVAS A LA INICIALIZACION DEL FILE SYSTEM//////////////////////////////////////////

//Verifica la existencia de la estructura superbloque, el archivo SuperBloque.ims y de su mapeo a la memoria. Si no existieran se generan
void superbloque_validar_existencia()
{
	log_debug(logger, "Se ingresa a superbloque_validar_existencia");

	superbloque_generar_estructura();
	superbloque_validar_existencia_del_archivo();
	superbloque_mapear_archivo_a_memoria();

	log_info(logger, "Existencia de SuperBloque validada");
}

//Dispatcher se como se generara la estructura en memoria del superbloque
void superbloque_generar_estructura()
{
	log_debug(logger, "Se ingresa a superbloque_generar_estructura");

	if(existe_en_disco(superbloque_path))
	{
		superbloque_generar_estructura_desde_archivo();
		superbloque_es_nuevo = 0;
	}
	else
	{
		superbloque_generar_estructura_con_valores_ingresados_por_consola();
		superbloque_es_nuevo = 1;
	}

	log_debug(logger, "Estructura de superbloque generada");
}

//Genera la estructura en memoria del superbloque levantandola del archivo
void superbloque_generar_estructura_desde_archivo()
{
	log_debug(logger, "Se ingresa a superbloque_generar_estructura_desde_archivo");

	int superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	//Los siguientes datos UNICAMENTE podrian ser levantados correctamente del archivo
	//si previamente se almacenaron en el orden definido por el enunciado
	lseek(superbloque_fd, 0, SEEK_SET);
	read(superbloque_fd, &superbloque.block_size, sizeof(uint32_t));
	read(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));

	superbloque_asignar_memoria_a_bitmap();
	read(superbloque_fd, superbloque.bitmap, bitmap_size);

	log_debug(logger, "Superbloque generado desde archivo");
}

//Setea el tamanio del bitmap en bytes en base a la cantidad de bloques que tendra Blocks.ims y le asigna dicho espacio en memoria
void superbloque_asignar_memoria_a_bitmap()
{
	//Tamanio del bitmap teniendo en cuenta que pueda haber un byte incompleto
	bitmap_size = 1 + (superbloque.blocks - 1) / CHAR_BIT;
	superbloque.bitmap = malloc(bitmap_size);
}

//Genera la estructura en memoria del superbloque tomando los valores de block_size y blocks por consola
void superbloque_generar_estructura_con_valores_ingresados_por_consola()
{
	log_debug(logger, "Se ingresa a superbloque_generar_estructura_con_valores_ingresados_por_consola");

	//Obtencion de block_size y blocks por consola que se guardan directamente en la estructura superbloque
	superbloque_tomar_valores_desde_consola();

	superbloque_asignar_memoria_a_bitmap();
	superbloque_setear_bitmap_a_cero();

	log_debug(logger, "Superbloque generado con valores ingresados por consola");
}

//Toma por consola los valores del tamanio de bloques y de cantidad de bloques que setearan a Superbloque.ims
void superbloque_tomar_valores_desde_consola()
{
	log_debug(logger, "Se ingresa a superbloque_tomar_valores abriendo la consola para recibir del usuario el tamanio y la cantidad de bloques");

	printf("Se procedera a crear el File System del modulo i-Mongo-Store para AmongOs...\n"
			"Indique por favor cual sera el tamanio que tendran los bloques en el sistema:\n");
	scanf("%d", &superbloque.block_size);

	printf("Cual sera la cantidad de bloques:\n");
	scanf("%d", &superbloque.blocks);

	printf("Muchas gracias!\n");
	log_debug(logger, "Se cierra consola. Se ingreso por el usuario block_size %d y blocks %d", superbloque.block_size, superbloque.blocks);
}

//Setea el total de bytes del bitmap en el SuperBloque en memoria a 0
void superbloque_setear_bitmap_a_cero()
{
	memset(superbloque.bitmap, 0, bitmap_size);
}

//Verifico si existe el archivo del superbloque y si no existe se crea con los valores de la estructura superbloque generada
void superbloque_validar_existencia_del_archivo()
{
	log_debug(logger, "Se ingresa a superbloque_validar_existencia_del_archivo");

	if(existe_en_disco(superbloque_path) == 0)
	{
		crear_archivo(superbloque_path);
		superbloque_cargar_archivo();
	}

	log_debug(logger, "Existencia del archivo SuperBloque.ims validada");
}

//Se carga el archivo SuperBloque.ims con los valores de la estructura superbloque
void superbloque_cargar_archivo()
{
	log_debug(logger, "Se ingresa a superbloque_cargar_archivo");

	int superbloque_fd;
	superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	lseek(superbloque_fd, 0, SEEK_SET);
	write(superbloque_fd, &superbloque.block_size, sizeof(uint32_t));      //si hace falta, agregar lseek despues de cada escritura
	write(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));
	write(superbloque_fd, superbloque.bitmap, bitmap_size);

	close(superbloque_fd);

	log_info(logger, "Archivo SuperBloque.ims cargado");
}

//Mapea a memoria el contenido del archivo SuperBloque.ims
void superbloque_mapear_archivo_a_memoria()
{
	log_debug(logger, "Se ingresa a superbloque_mapear_archivo_a_memoria");

	int superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	size_t superbloque_size = 2 * sizeof(uint32_t) + bitmap_size;

	superbloque_address = mmap(NULL, superbloque_size, PROT_WRITE | PROT_READ, MAP_SHARED, superbloque_fd, 0);

	close(superbloque_fd);

	log_debug(logger, "Superbloque mapeado del archivo a la memoria");
}

//FUNCIONES DEL BLOCKS RELATIVAS A LA INICIALIZACION DEL FILE SYSTEM
//Verifica la existencia de Blocks y su copia en la memoria. Si no existiera se generan
void blocks_validar_existencia()
{
	log_debug(logger, "Se ingresa a blocks_validar_existencia");

	blocks_validar_existencia_del_archivo();
	blocks_mapear_archivo_a_memoria();

	log_info(logger, "Existencia de Blocks validada");
}

//Verifica si existe el archivo del superbloque y si no existe se crea inicializandolo
void blocks_validar_existencia_del_archivo()
{
	log_debug(logger, "Se ingresa a blocks_validar_existencia_del_archivo");

	//Calculo del tamanio del archivo de Blocks.ims
	blocks_size = superbloque.blocks * superbloque.block_size;

	if(superbloque_es_nuevo || existe_en_disco(blocks_path) == 0)
	{
		crear_archivo(blocks_path);
		dar_tamanio_a_archivo(blocks_path, blocks_size);
	}

	log_debug(logger, "Existencia del archivo Blocks.ims validada");
}

//Mapea a memoria el contenido del archivo Blocks.ims
void blocks_mapear_archivo_a_memoria()
{
	log_debug(logger, "Se ingresa a blocks_mapear_archivo_a_memoria");

	int blocks_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	blocks_address = mmap(NULL, blocks_size, PROT_WRITE | PROT_READ, MAP_SHARED, blocks_fd, 0);

	close(blocks_fd);

	log_debug(logger, "Blocks mapeado del archivo a la memoria");
}


//////////////////////FUNCIONES DEL SUPERBLOQUE RELATIVAS AL USO POSTERIOR A LA INICIALIZACION DEL FILE SYSTEM///////////////////////////

//Sincroniza de manera forzosa SuperBloque.ims con el espacio de memoria mapeado
void superbloque_actualizar_archivo()
{
	log_debug(logger, "Se ingresa a superbloque_actualizar_archivo");
	msync(superbloque_address, superbloque_size, MS_SYNC);
	log_debug(logger, "Archivo SuperBloque.ims actualizado con los valores de la memoria");
}

//Busca y devuelve el primer bloque libre en el bitmap, si no existiera finaliza con error
int superbloque_obtener_bloque_libre()
{
	//Por alguna razon convendria crear y destruir el bitarray en cada ocasion?
	log_debug(logger, "Se ingresa a superbloque_obtener_bloque_libre");

	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, bitmap_size, LSB_FIRST);
	int bloque;

	for(bloque = 0; bloque < superbloque.blocks && bitarray_test_bit(bitmap, bloque); bloque++);

	if(bitarray_test_bit(bitmap, bloque))
	{
		log_error(logger, "No hay bloques libres para almacenar nuevos recursos en el bitmap del superbloque");//. Se finaliza por falta de espacio en los bloques"); exit(EXIT_FAILURE);
		bloque = -1; //VALOR DE ERROR
	}

	bitarray_destroy(bitmap);
	return bloque;
}

/////////////////////FUNCION DEL BLOCKS RELATIVAS AL USO POSTERIOR A LA INICIALIZACION DEL FILE SYSTEM/////////////////////

//TODO AGREGAR LO DE LA COPIA DE LA COPIA EN MEMORIA

//Sincroniza de manera forzosa el archivo de Blocks.ims con el espacio de memoria mapeado
void blocks_actualizar_archivo()
{
	log_debug(logger, "Se ingresa a blocks_actualizar_archivo");
	msync(blocks_address, blocks_size, MS_SYNC);
	log_debug(logger, "Archivo Blocks.ims actualizado con los valores de la memoria");
}

//Obtiene el conjunto de bloques concatenado que el recurso esta ocupando en el archivo de Blocks.ims devolviendo el tamanio del mismo
int blocks_obtener_concatenado_segun_lista(t_recurso_md* recurso_md, char** concatenado)
{
	log_debug(logger, "Entro a blocks_obtener_concatenado_segun_lista");
	char** lista_de_bloques = malloc(sizeof(string_get_string_as_array(recurso_md->blocks)));
	lista_de_bloques = string_get_string_as_array(recurso_md->blocks);

	int cantidad_bloques = sizeof(lista_de_bloques) / sizeof(char*);

	int tamanio_concatenado = superbloque.block_size * cantidad_bloques;
	*concatenado = malloc(tamanio_concatenado);

	if(*concatenado == NULL)
	{
		tamanio_concatenado = -1; //Valor de retorno con error manejado en la funcion que hace el llamado
	}
	else
	{
		char* posicion_desde;
		char* posicion_hacia = *concatenado;
		int bloque;

		for(int i = 0; i < cantidad_bloques; i++)
		{
			bloque = atoi(lista_de_bloques[i]);
			posicion_desde = blocks_address + bloque * superbloque.block_size;
			memcpy(posicion_hacia, posicion_desde, superbloque.block_size);
			posicion_hacia += superbloque.block_size;
		}
	}
	cadena_eliminar_array_de_cadenas(&lista_de_bloques, cantidad_bloques);

	log_debug(logger, "Bloques concatenados en direccion de memoria %p con tamanio %d", *concatenado, tamanio_concatenado);
	return tamanio_concatenado;
}

/////////////////////FUNCIONES DE LOS RECURSOS RELATIVAS A LA REALIZACION DE LAS TAREAS//////////////////////////////////////

//Genera el recurso indicado por codigo_recurso con la cantidad indicada por cantidad
void recurso_generar_cantidad(recurso_code codigo_recurso, int cantidad)
{
	t_recurso_data* recurso_data = &lista_recursos[codigo_recurso];

	log_debug(logger, "Se ingresa a recurso_generar_cantidad, se debe generar %d %s(s)", cantidad, recurso_data->nombre);

	//wait recurso_data
	recurso_validar_existencia_metadata_en_memoria(recurso_data);
	//wait superbloque y blocks
	metadata_generar_cantidad(recurso_data->metadata, cantidad);

	superbloque_actualizar_archivo();
	blocks_actualizar_archivo(); //TODO lo de sincronizar la copia de la copia usando el tiempo de sincronizacion de config
	recurso_actualizar_archivo(recurso_data);
	//signal recurso_data superbloque y blocks //quiza se puede ser mas especifico

	log_info(logger, "Finaliza recurso_generar_cantidad de %s", recurso_data->nombre);

}

//Si no existe la metadata en memoria, la genera con los valores del archivo o con sus valores default si este ultimo tampoco estuviera
void recurso_validar_existencia_metadata_en_memoria(t_recurso_data* recurso_data)
{
	log_debug(logger, "Entro a recurso_validar_existencia_metadata_en_memoria para el recurso %s", recurso_data->nombre);

	if(recurso_data->metadata == NULL)
	{
		recurso_data->metadata = malloc(sizeof(t_recurso_md));

		recurso_data->metadata->caracter_llenado = string_from_format("%c", recurso_data->caracter_llenado);

		if(existe_en_disco(recurso_data->ruta_completa))
		{
			metadata_levantar_de_archivo_a_memoria_valores_variables(recurso_data->metadata, recurso_data->ruta_completa);
		}
		else
		{
			metadata_setear_con_valores_default_en_memoria(recurso_data->metadata);
		}
	}

	log_debug(logger, "Existencia de metadata para el recurso %s validada", recurso_data->nombre);
}

//Actualiza la metadata del recurso en memoria al archivo. Si este no existe lo crea con los valores actualizados
void recurso_actualizar_archivo(t_recurso_data* recurso_data)
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
	log_info(logger, "Se actualizo metadata de %s de memoria a archivo %s", recurso_data->nombre, recurso_data->ruta_completa);
}

//Descarta la metadata y el archivo correspondiente del recurso dado por parametro siempre y cuando la cantidad recibida sea igual a 0
void recurso_descartar_cantidad(recurso_code codigo_recurso, int cantidad)
{
	t_recurso_data* recurso_data = &lista_recursos[codigo_recurso];
	log_debug(logger, "Se ingresa a recurso_descartar_cantidad para el recurso %s y la cantidad %d", recurso_data->nombre, cantidad);

	if(cantidad == 0)
	{
		if(existe_en_disco(recurso_data->ruta_completa) == 0)
		{
			log_info(logger, "No existe el archivo %s como para descartarlo", recurso_data->nombre_archivo);
		}
		else
		{
			remove(recurso_data->ruta_completa);
			log_info(logger, "Se elimino el archivo de la metadata del recurso %s", recurso_data->nombre);
		}

		if(recurso_data->metadata != NULL)
		{
			free(recurso_data->metadata->blocks);
			metadata_setear_con_valores_default_en_memoria(recurso_data->metadata);
			log_debug(logger, "Se seteo a los valores default la metadata en memoria del recurso %s", recurso_data->nombre);
		}
	}
	else
	{
		log_info(logger, "La cantidad debería ser igual a 0 al querer descartar el recurso %s", recurso_data->nombre);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////
//Carga la cantidad cantidad de los recursos del codigo_recurso en el ultimo bloque o en nuevos bloques que esten libres agregandolos a la lista de bloques
void metadata_generar_cantidad(t_recurso_md* recurso_md, int cantidad)
{
	//pthread_mutex_lock(&mutex_blocks); //sector critico porque se usa superbloque.ims  y blocks.ims

	log_debug(logger, "Entro a metadata_generar_cantidad con C_LL %s y cantidad %d", recurso_md->caracter_llenado, cantidad);
	log_debug(logger, "Valores de metadata antes de generar recursos: SIZE %d, BLOCK_COUNT %d, BLOCKS %s y MD5 %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo);

	int bloque_a_cargar = -1;

	while(cantidad > 0)
	{
		if(bloque_a_cargar >= 0 || metadata_tiene_espacio_en_ultimo_bloque(recurso_md))
		{
			bloque_a_cargar = bloque_a_cargar >= 0 ? bloque_a_cargar : metadata_ultimo_bloque_usado(recurso_md);
			metadata_cargar_en_bloque(recurso_md, &cantidad, bloque_a_cargar);
		}
		if(cantidad > 0)
		{
			bloque_a_cargar = superbloque_obtener_bloque_libre();
			if(bloque_a_cargar == -1)
			{
				log_debug(logger, "No se puede generar la cantidad solicitada ya que no hay bloques libres");
				break;
			}
			else
			{
				metadata_agregar_bloque_a_lista_de_blocks(recurso_md, bloque_a_cargar);
			}
		}
	}
	metadata_actualizar_md5(recurso_md);

	log_info(logger, "Se genero la cantidad solicitada teniendo finalmente el recurso con C_LL %s, SIZE %s, BLOCKS %s y el MD5 %s",
			recurso_md->caracter_llenado, recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
}

//Agrega a la lista de blocks de la metadata un bloque nuevo obtenido a traves del bitmap del superbloque y a su vez lo devuelve
void metadata_agregar_bloque_a_lista_de_blocks(t_recurso_md* recurso_md, int bloque)
{
	log_debug(logger, "Se ingresa a metadata_agregar_bloque_a_lista_de_blocks con blocks %s, block_count %d y bloque a agregar %s",
			recurso_md->blocks, recurso_md->block_count, bloque);

	cadena_sacar_ultimo_caracter(recurso_md->blocks);
	log_debug(logger, "Blocks despues de sacarle el ultimo caracter %s", recurso_md->blocks);

	string_append_with_format(&recurso_md->blocks, ",%d]", bloque);
	recurso_md->block_count++;

	log_debug(logger, "Se agrega bloque libre %s a blocks quedando BLOCK_COUNT %s y BLOCKS %s",
			bloque, recurso_md->block_count, recurso_md->blocks);
}

//Verifica si el ultimo bloque usado por el recurso tiene espacio disponible
int metadata_tiene_espacio_en_ultimo_bloque(t_recurso_md* recurso_md)
{
	log_debug(logger, "Ingreso a metadata_tiene_espacio_en_ultimo_bloque con SIZE %d, BLOCKS %s y superbloque.block_size %d",
			recurso_md->size, recurso_md->blocks, superbloque.block_size);

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	log_debug(logger, "Cantidad de bloques %s", cantidad_bloques);
	return (recurso_md->size < cantidad_bloques * superbloque.block_size);
}

//Carga recursos en el bloque dado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_bloque(t_recurso_md* recurso_md, int* cantidad, int bloque)
{
	log_debug(logger, "Se ingresa a recurso_cargar_en_bloque con caracter %s, cantidad %d y bloque %d", recurso_md->caracter_llenado, cantidad, bloque);

	int posicion_en_bloque = recurso_md->size % superbloque.block_size;
	int posicion_inicio_bloque = bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque; //esta ok sumar un puntero con 2 ints?

	log_debug(logger, "Posicion_en_bloque %d, posicion_inicio_bloque %d posicion absoluta %p o %d",
			posicion_en_bloque, posicion_inicio_bloque, posicion_absoluta, posicion_absoluta);

	char caracter_llenado = recurso_md->caracter_llenado[0];

	while(posicion_en_bloque < superbloque.block_size && (*cantidad) > 0)
	{
		*posicion_absoluta = caracter_llenado;

		(*cantidad)--;
		recurso_md->size++;
		posicion_absoluta++;
		posicion_en_bloque++;
	}

	log_debug(logger, "Se cargo en bloque %d con caracter %s quedando por cargar %d recurso(s)", bloque, recurso_md->caracter_llenado, cantidad);
}

//ALTER Carga recursos en el bloque dado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_bloque2(t_recurso_md* recurso_md, int* cantidad, int bloque)
{
	log_debug(logger, "Se ingresa a recurso_cargar_en_bloque con caracter %s, cantidad %d y bloque %d", recurso_md->caracter_llenado, cantidad, bloque);

	int posicion_en_bloque = recurso_md->size % superbloque.block_size;
	int posicion_inicio_bloque = bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
	char* posicion_absoluta = blocks_address + posicion_inicio_bloque + posicion_en_bloque; //esta ok sumar un puntero con 2 ints?

	log_debug(logger, "Posicion_en_bloque %d, posicion_inicio_bloque %d posicion absoluta %p o %d",
			posicion_en_bloque, posicion_inicio_bloque, posicion_absoluta, posicion_absoluta);

	char caracter_llenado = recurso_md->caracter_llenado[0];

	int espacio_libre_en_bloque = superbloque.block_size - posicion_en_bloque;
	int cantidad_a_cargar = espacio_libre_en_bloque < *cantidad ? espacio_libre_en_bloque : *cantidad;
	*cantidad -= cantidad_a_cargar;

	memset(posicion_absoluta, caracter_llenado, cantidad_a_cargar);

	recurso_md->size += cantidad_a_cargar;

	log_debug(logger, "Se cargo en bloque con caracter %s quedando por cargar %d recurso(s)", bloque, recurso_md->caracter_llenado, cantidad);
}

//DEPREC Carga recursos en el ultimo bloque registrado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_ultimo_bloque(t_recurso_md* recurso_md, int* cantidad, int nuevo_bloque)
{
	log_debug(logger, "Se ingresa a recurso_cargar_en_ultimo_bloque con caracter %s y cantidad %d", recurso_md->caracter_llenado, cantidad);

	//Si el valor del nuevo_bloque no es el valor default es porque ese bloque es el nuevo ultimo de la lista y no necesita calcularse
	int ultimo_bloque = nuevo_bloque == -1 ? metadata_ultimo_bloque_usado(recurso_md) : nuevo_bloque;
	int posicion_en_bloque = recurso_md->size % superbloque.block_size;
	int posicion_inicio_bloque = ultimo_bloque * superbloque.block_size; //se toma en cuenta que los bloques comienzan en 0
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

//DEPREC Carga recursos en el ultimo bloque registrado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_ultimo_bloque2(t_recurso_md* recurso_md, int* cantidad, int nuevo_bloque)
{
	log_debug(logger, "Se ingresa a recurso_cargar_en_ultimo_bloque con caracter %s y cantidad %d", recurso_md->caracter_llenado, cantidad);

	//Si el valor del nuevo_bloque no es el valor default es porque ese bloque es el nuevo ultimo de la lista y no necesita calcularse
	int ultimo_bloque = nuevo_bloque == -1 ? metadata_ultimo_bloque_usado(recurso_md) : nuevo_bloque;
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

//Levanta a la estructura metadata en memoria los valores variables contenidos en el archivo de metadata correspondiente
void metadata_levantar_de_archivo_a_memoria_valores_variables(t_recurso_md* recurso_md, char* recurso_path)
{
	log_debug(logger, "Se ingresa a metadata_levantar_de_archivo_a_memoria para el recurso con ruta %s", recurso_path);

	t_config* recurso_config = config_create(recurso_path);

	recurso_md->size = config_get_int_value(recurso_config, "SIZE");
	recurso_md->block_count = config_get_int_value(recurso_config, "BLOCK_COUNT");;
	recurso_md->blocks = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));
	strcpy(recurso_md->md5_archivo, config_get_string_value(recurso_config, "MD5_ARCHIVO"));

	config_destroy(recurso_config);

	log_debug(logger, "Valores variables levantados SIZE %d BLOCK_COUNT %d BLOCKS %sy MD5_ARCHIVO %s. Valor fijo CARACTER_LLENADO %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo, recurso_md->caracter_llenado);
	log_debug(logger, "Metadata levantada a memoria desde archivo %s", recurso_path);
}

//Setea en memoria los valores variables de la metadata del recurso dado con los valores default
void metadata_setear_con_valores_default_en_memoria(t_recurso_md* recurso_md)
{
	log_debug(logger, "Se ingresa a recurso_inicializar_metadata_en_memoria para el recurso con caracter de llenado %s", recurso_md->caracter_llenado);

	recurso_md->size = 0;
	recurso_md->block_count = 0;
	recurso_md->blocks = string_duplicate("[]");
	strcpy(recurso_md->md5_archivo, "d41d8cd98f00b204e9800998ecf8427e");

	log_debug(logger, "Valores seteados SIZE %d BLOCK_COUNT %d BLOCKS %s CARACTER_LLENADO %s y MD5_ARCHIVO %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->caracter_llenado, recurso_md->md5_archivo);
}

//Calcula y actualiza el valor del md5 del archivo teniendo en cuenta el conjunto de bloques que ocupa el recurso en el archivo de Blocks.ims
void metadata_actualizar_md5(t_recurso_md* recurso_md)
{
	log_debug(logger, "Se ingresa a metadata_actualizar_md5");

	char* concatenado = NULL;
	char md5_str[33];

	int tamanio_concatenado = blocks_obtener_concatenado_segun_lista(recurso_md, &concatenado);

	if(tamanio_concatenado >= 0)
	{
		cadena_calcular_md5(concatenado, tamanio_concatenado, md5_str);
		strcpy(recurso_md->md5_archivo, md5_str);
	}
	else
	{
		log_error(logger, "No hay memoria suficiente como para calcular el MD5 del recurso con caracter de llenado %s", recurso_md-> caracter_llenado);
	}
	free(concatenado);

	log_debug(logger, "MD5 en metadata actualizado a %s", recurso_md->md5_archivo);
}

//Calcula y actualiza el valor del md5 del archivo teniendo en cuenta el conjunto de bloques que ocupa el recurso en el archivo de Blocks.ims
void metadata_actualizar_md5_alter(t_recurso_md* recurso_md)
{
	log_debug(logger, "Se ingresa a metadata_actualizar_md5");

	char* concatenado = NULL;
	char md5_str[33];
	char* md5_buffer;

	int tamanio_concatenado = blocks_obtener_concatenado_segun_lista(recurso_md, &concatenado);

	if(tamanio_concatenado >= 0)
	{
		 md5_buffer = cadena_calcular_md5_alter(concatenado, tamanio_concatenado);
		 strcpy(recurso_md->md5_archivo, md5_buffer);
		 free(md5_buffer);
	}
	else
	{
		log_error(logger,"No hay memoria suficiente como para calcular el MD5 del recurso con caracter de llenado %s", recurso_md-> caracter_llenado);
	}
	free(concatenado);

	log_debug(logger, "MD5 en metadata actualizado a %s", recurso_md->md5_archivo);
}

