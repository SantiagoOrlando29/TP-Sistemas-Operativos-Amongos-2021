//utils_mongostore.c 15072021 03.16 //TODO: Ver si es necesario almacenar las posiciones de los sabotajes con coordenadas //TODO: Ver de donde sale IP

//msync: The implementation shall require that addr be a multiple of the page size as returned by sysconf().
//long int sysconf(int nombre); 	SC_PAGESIZE _SC_PAGE_SIZE El tamaño de una página (en bytes).
#include "utils_mongostore.h"

t_recurso_data lista_recursos[] = {
	{"OXIGENO", 'O', "Oxigeno.ims", NULL, NULL},
	{"COMIDA", 'C', "Comida.ims", NULL, NULL},
	{"BASURA", 'B', "Basura.ims", NULL, NULL}
};

//TODO BITACORA, SIGNAL (LINKEADO A mandar_primer_ubicacion_sabotaje)

//TODO EN DESARROLLO FSCK (AL FONDO Y A LA DERECHA DE ESTE ARCHIVO) Y CONSUMIR_RECURSO (NO TERMINADO 2)

//Funcion temporal solo para verificar falla
void verificar_superbloque_temporal();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////FUNCIONES ORDENADAS "CRONOLOGICAMENTE"//////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

//Inicia el File System restaurando a memoria los archivos existentes si los hubiera
void file_system_iniciar()
{
	log_debug(logger, "Info-Comienza iniciar_file_system");

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
	int cantidad_recursos = files_cantidad_recursos();

	for(int i = 0; i < cantidad_recursos; i++)
	{
		lista_recursos[i].ruta_completa = string_from_format("%s/%s", files_path, lista_recursos[i].nombre_archivo);
	}
}

//Crea las carpetas necesarias para persistir la informacion delasignar File System
void file_system_crear_directorios_inexistentes()
{
	log_debug(logger, "Info-Se ingresa a file_system_crear_directorios_inexistentes");

	crear_directorio_si_no_existe(configuracion.punto_montaje);
	crear_directorio_si_no_existe(files_path);
	crear_directorio_si_no_existe(bitacoras_path);

	log_info(logger, "Directorios del File System confirmados");
}

//Crea el directorio indicado por path, en caso de no poder finaliza con error
void crear_directorio_si_no_existe(char* path)
{
	log_debug(logger, "IO-Se ingresa a crear_directorio_si_no_existe para %s", path);

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

//Verifica la existencia de la estructura superbloque, el archivo SuperBloque.ims y de su mapeo a la memoria. Si no existieran se generan
void superbloque_validar_existencia()
{
	log_debug(logger, "Info-Se ingresa a superbloque_validar_existencia");

	superbloque_generar_estructura();
	superbloque_validar_existencia_del_archivo();
	verificar_superbloque_temporal();
	log_info(logger, "Existencia de SuperBloque validada");
}

//Dispatcher se como se generara la estructura en memoria del superbloque
void superbloque_generar_estructura()
{
	log_debug(logger, "I-Se ingresa a superbloque_generar_estructura");

	if(existe_en_disco(superbloque_path))
	{
		verificar_superbloque_temporal();
		superbloque_generar_estructura_desde_archivo();
	}
	else
	{
		superbloque_generar_estructura_con_valores_ingresados_por_consola();
		file_system_eliminar_archivos_previos();//DANGER!!!
	}

	log_debug(logger, "O-Estructura de superbloque generada con tamanio de bloque %d y cantidad de bloques %d",
			superbloque.block_size, superbloque.blocks);
}

//Genera la estructura en memoria del superbloque levantandola del archivo
void superbloque_generar_estructura_desde_archivo()
{
	log_debug(logger, "I-Se ingresa a superbloque_generar_estructura_desde_archivo");

	int superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	verificar_superbloque_temporal();
	//Los siguientes datos UNICAMENTE podrian ser levantados correctamente del archivo
	//si previamente se almacenaron en el orden definido por el enunciado
	int desplazamiento = 0;
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.block_size, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	bitmap_size = 1 + (superbloque.blocks - 1) / CHAR_BIT;
	superbloque.bitmap = malloc(bitmap_size);
	lseek(superbloque_fd, desplazamiento, SEEK_SET);
	read(superbloque_fd, superbloque.bitmap, bitmap_size);

	close(superbloque_fd);

	verificar_superbloque_temporal();
	log_debug(logger, "O-Superbloque generado desde archivo");
}

//Genera la estructura en memoria del superbloque tomando los valores de block_size y blocks por consola
void superbloque_generar_estructura_con_valores_ingresados_por_consola()
{
	log_debug(logger, "I-Se ingresa a superbloque_generar_estructura_con_valores_ingresados_por_consola");

	//Obtencion de block_size y blocks por consola que se guardan directamente en la estructura superbloque
	superbloque_tomar_valores_desde_consola();

	superbloque_asignar_memoria_a_bitmap();
	superbloque_setear_bitmap_a_cero();

	log_debug(logger, "O-Superbloque generado con valores ingresados por consola");
}

//Toma por consola los valores del tamanio de bloques y de cantidad de bloques que setearan a Superbloque.ims
void superbloque_tomar_valores_desde_consola()
{
	log_debug(logger, "I-Se ingresa a superbloque_tomar_valores abriendo la consola para recibir del usuario el tamanio y la cantidad de bloques");

	printf("Se procedera a crear el File System del modulo i-Mongo-Store para AmongOs...\n"
			"Indique por favor cual sera el tamanio que tendran los bloques en el sistema:\n");
	scanf("%d", &superbloque.block_size);

	printf("Cual sera la cantidad de bloques:\n");
	scanf("%d", &superbloque.blocks);

	printf("Muchas gracias!\n");
	log_debug(logger, "O-Se cierra consola. Se ingreso por el usuario block_size %d y blocks %d", superbloque.block_size, superbloque.blocks);
}

//Setea el tamanio del bitmap en bytes en base a la cantidad de bloques que tendra Blocks.ims y le asigna dicho espacio en memoria
void superbloque_asignar_memoria_a_bitmap()
{
	//Tamanio del bitmap teniendo en cuenta que pueda haber un byte incompleto
	bitmap_size = 1 + (superbloque.blocks - 1) / CHAR_BIT;
	superbloque.bitmap = malloc(bitmap_size);
	log_debug(logger, "IO-Al salir de superbloque_asignar_memoria_a_bitmap se tiene superbloque.blocks %d y bitmap_size %d",
			superbloque.blocks, bitmap_size);
}

//Setea el total de bytes del bitmap en el SuperBloque en memoria a 0
void superbloque_setear_bitmap_a_cero()
{
	memset(superbloque.bitmap, 0, bitmap_size);
}

//Verifico si existe el archivo del superbloque y si no existe se crea con los valores de la estructura superbloque generada
void superbloque_validar_existencia_del_archivo()
{
	log_debug(logger, "I-Se ingresa a superbloque_validar_existencia_del_archivo");

	if(existe_en_disco(superbloque_path) == 0)
	{
		crear_archivo(superbloque_path);
		superbloque_cargar_archivo();
	}
	verificar_superbloque_temporal();
	log_debug(logger, "O-Existencia del archivo SuperBloque.ims validada");
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
	log_debug(logger, "IO-Se creo el archivo dado por la ruta completa \"%s\"", path);
}

//Se carga el archivo SuperBloque.ims con los valores de la estructura superbloque
void superbloque_cargar_archivo()
{
	log_debug(logger, "I-Se ingresa a superbloque_cargar_archivo");

	int superbloque_fd;
	superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

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
	verificar_superbloque_temporal();
	log_debug(logger, "O-Archivo SuperBloque.ims cargado");
}

//DEPREC Mapea a memoria el contenido del archivo SuperBloque.ims
void superbloque_mapear_bitmap_en_archivo_a_memoria()
{
	log_debug(logger, "I-Se ingresa a superbloque_mapear_bitmap_en_archivo_a_memoria");

	int superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	bitmap_address = mmap(NULL, bitmap_size, PROT_WRITE | PROT_READ, MAP_SHARED, superbloque_fd, 2 * sizeof(uint32_t));

	close(superbloque_fd);

	log_debug(logger, "O-Bitmap de Superbloque mapeado del archivo a la memoria");
}

//DEPREC Mapea a memoria el contenido del archivo SuperBloque.ims
void superbloque_mapear_archivo_a_memoria()
{
	log_debug(logger, "I-Se ingresa a superbloque_mapear_archivo_a_memoria");

	int superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	size_t superbloque_size = sizeof(superbloque.block_size) + sizeof(superbloque.blocks) + bitmap_size;

	superbloque_address = mmap(NULL, superbloque_size, PROT_WRITE | PROT_READ, MAP_SHARED, superbloque_fd, 0);

	close(superbloque_fd);

	log_debug(logger, "O-Superbloque mapeado del archivo a la memoria");
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

void file_system_eliminar_archivos_previos()
{
	blocks_eliminar_archivo();
	files_eliminar_archivos();
}

void blocks_eliminar_archivo()
{
	remove(blocks_path);
}

void files_eliminar_archivos()
{
	int cantidad_recursos = files_cantidad_recursos();

	for(int i = 0; i < cantidad_recursos; i++)
	{
		remove(lista_recursos[i].ruta_completa);
	}
}

//Verifica la existencia de Blocks y su copia en la memoria. Si no existiera se generan
void blocks_validar_existencia()
{
	log_debug(logger, "Info-Se ingresa a blocks_validar_existencia");

	blocks_validar_existencia_del_archivo();
	blocks_mapear_archivo_a_memoria();

	log_info(logger, "Existencia de Blocks validada");
}

//Verifica si existe el archivo del superbloque y si no existe se crea inicializandolo
void blocks_validar_existencia_del_archivo()
{
	log_debug(logger, "I-Se ingresa a blocks_validar_existencia_del_archivo");

	//Calculo del tamanio del archivo de Blocks.ims
	blocks_size = superbloque.blocks * superbloque.block_size;

	if(existe_en_disco(blocks_path) == 0)
	{
		crear_archivo(blocks_path);
		dar_tamanio_a_archivo(blocks_path, blocks_size);
	}

	log_debug(logger, "O-Existencia del archivo Blocks.ims validada");
}

//Da el tamanio indicado por length al archivo dado por la ruta path. Finaliza si no puede realizarse con exito
void dar_tamanio_a_archivo(char* path, size_t length)
{
	log_debug(logger, "I-Se ingresa a dar_tamanio_a_archivo");

	if(truncate(path, length) == -1)
	{
		log_error(logger, "No se puede asignar el tamanio %d al archivo de ruta %s", length, path);
		exit(EXIT_FAILURE);
	}

	log_debug(logger, "O-Se da el tamanio exitosamente a archivo con ruta %s", path);
}

//Mapea a memoria el contenido del archivo Blocks.ims
void blocks_mapear_archivo_a_memoria()
{
	log_debug(logger, "I-Se ingresa a blocks_mapear_archivo_a_memoria");

	int blocks_fd = abrir_archivo_para_lectura_escritura(blocks_path);

	blocks_address = mmap(NULL, blocks_size, PROT_WRITE | PROT_READ, MAP_SHARED, blocks_fd, 0);

	close(blocks_fd);

	log_debug(logger, "O-Blocks mapeado del archivo a la memoria");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////FIN DE FUNCIONES NECESARIAS PARA INICIAR EL FILE SYSTEM//////////////////////////////////////////////////////

///////////////COMIENZO DE FUNCIONES PARA REALIZAR TAREAS///////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TODO lo de sincronizar la copia de la copia usando el tiempo de sincronizacion de config

void recurso_generar_cantidad(recurso_code codigo_recurso, int cantidad)
{
	if(recurso_es_valido(codigo_recurso))
	{
		t_recurso_data* recurso_data = &lista_recursos[codigo_recurso];

		log_debug(logger, "Info-Se ingresa a recurso_generar_cantidad, se debe generar %d %s(s)", cantidad, recurso_data->nombre);

		//wait recurso_data
		recurso_validar_existencia_metadata_en_memoria(recurso_data);
		//wait superbloque y blocks
		metadata_generar_cantidad(recurso_data->metadata, cantidad);

		superbloque_actualizar_bitmap_en_archivo();
		blocks_actualizar_archivo();
		recurso_actualizar_archivo(recurso_data);
		//signal recurso_data superbloque y blocks //quiza se puede ser mas especifico

		log_info(logger, "Finaliza recurso_generar_cantidad de %s", recurso_data->nombre);

		//FIXME Despues borrar lo siguiente
		verificar_superbloque_temporal();
	}
	else
	{
		log_info(logger, "El recurso indicado (%d) no es valido, prueba otra vez", codigo_recurso);
	}

}

//Funcion solo para encontrar falla
void verificar_superbloque_temporal()
{
	int fd = abrir_archivo_para_lectura_escritura(superbloque_path);
	lseek(fd, 0, SEEK_SET);
	int sb_b_s, sb_b;
	read(fd, &sb_b_s, sizeof(uint32_t));
	lseek(fd, sizeof(uint32_t), SEEK_SET);
	read(fd, &sb_b, sizeof(uint32_t));
	log_info(logger, "PARA DEBUG SB.B_S %d Y SB.BLOCKS %d y en archivo SB.B_S %d Y SB.BLOCKS %d", superbloque.block_size, superbloque.blocks, sb_b_s, sb_b);
	close(fd);
}

int recurso_es_valido(recurso_code codigo_recurso)
{
	return (codigo_recurso >= 0 && codigo_recurso <= files_cantidad_recursos());
}

int files_cantidad_recursos()
{
	return (sizeof(lista_recursos) / sizeof(t_recurso_data));
}

//Si no existe la metadata en memoria, la genera con los valores del archivo o con sus valores default si este ultimo tampoco estuviera
void recurso_validar_existencia_metadata_en_memoria(t_recurso_data* recurso_data)
{
	log_debug(logger, "I-Entro a recurso_validar_existencia_metadata_en_memoria para el recurso %s", recurso_data->nombre);

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

	log_debug(logger, "O-Existencia de metadata para el recurso %s validada", recurso_data->nombre);
}

//Levanta a la estructura metadata en memoria los valores variables contenidos en el archivo de metadata correspondiente
void metadata_levantar_de_archivo_a_memoria_valores_variables(t_recurso_md* recurso_md, char* recurso_path)
{
	log_debug(logger, "I-Se ingresa a metadata_levantar_de_archivo_a_memoria para el recurso con ruta %s", recurso_path);

	t_config* recurso_config = config_create(recurso_path);

	recurso_md->size = config_get_int_value(recurso_config, "SIZE");
	recurso_md->block_count = config_get_int_value(recurso_config, "BLOCK_COUNT");;
	recurso_md->blocks = string_duplicate(config_get_string_value(recurso_config, "BLOCKS"));
	strcpy(recurso_md->md5_archivo, config_get_string_value(recurso_config, "MD5_ARCHIVO"));

	config_destroy(recurso_config);

	log_debug(logger, "O-Valores variables levantados SIZE %d BLOCK_COUNT %d BLOCKS %s y MD5_ARCHIVO %s. Valor fijo CARACTER_LLENADO %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo, recurso_md->caracter_llenado);
	log_debug(logger, "O-Metadata levantada a memoria desde archivo %s", recurso_path);
}

//Setea en memoria los valores variables de la metadata del recurso dado con los valores default
void metadata_setear_con_valores_default_en_memoria(t_recurso_md* recurso_md)
{
	log_debug(logger, "I-Se ingresa a recurso_inicializar_metadata_en_memoria para el recurso con caracter de llenado %s", recurso_md->caracter_llenado);

	recurso_md->size = 0;
	recurso_md->block_count = 0;
	recurso_md->blocks = string_duplicate("[]");
	strcpy(recurso_md->md5_archivo, "d41d8cd98f00b204e9800998ecf8427e");

	log_debug(logger, "O-Valores seteados SIZE %d BLOCK_COUNT %d BLOCKS %s CARACTER_LLENADO %s y MD5_ARCHIVO %s",
			recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->caracter_llenado, recurso_md->md5_archivo);
}

void metadata_generar_cantidad(t_recurso_md* recurso_md, int cantidad)
{
	//pthread_mutex_lock(&mutex_blocks); //sector critico porque se usa superbloque.ims  y blocks.ims

	log_debug(logger, "Info-Entro a metadata_generar_cantidad con C_LL %s y cantidad %d", recurso_md->caracter_llenado, cantidad);

	int bloque_a_cargar = -1;

	while(cantidad > 0)
	{
		log_debug(logger, "Valores de metadata en iteracion while, antes de generar recursos: SIZE %d, BLOCK_COUNT %d, BLOCKS %s y MD5 %s. "
				"Bloque a cargar %d", recurso_md->size, recurso_md->block_count, recurso_md->blocks, recurso_md->md5_archivo, bloque_a_cargar);

		if(bloque_a_cargar >= 0 || metadata_tiene_espacio_en_ultimo_bloque(recurso_md))
		{
			bloque_a_cargar = bloque_a_cargar >= 0 ? bloque_a_cargar : metadata_ultimo_bloque_usado(recurso_md);
			metadata_cargar_en_bloque(recurso_md, &cantidad, bloque_a_cargar);
		}
		if(cantidad > 0)
		{
			bloque_a_cargar = superbloque_obtener_bloque_libre();
			if(bloque_a_cargar >= 0)
			{
				metadata_agregar_bloque_a_lista_de_blocks(recurso_md, bloque_a_cargar);
			}
			else
			{
				//FIXME log_error+return o no dar bola a la condicion de que no hayan bloques libres ya que sino habria que se hacer un rollback trabajando en un buffer
				log_debug(logger, "No se puede generar la cantidad solicitada ya que no hay bloques libres");
				break;
			}
		}
	}
	metadata_actualizar_md5(recurso_md);

	log_info(logger, "Se genero la cantidad solicitada teniendo finalmente el recurso con C_LL %s, SIZE %d, BLOCKS %s y el MD5 %s",
			recurso_md->caracter_llenado, recurso_md->size, recurso_md->blocks, recurso_md->md5_archivo);
}

//Verifica si el ultimo bloque usado por el recurso tiene espacio disponible
int metadata_tiene_espacio_en_ultimo_bloque(t_recurso_md* recurso_md)
{
	log_debug(logger, "I-Ingreso a metadata_tiene_espacio_en_ultimo_bloque con SIZE %d, BLOCKS %s y superbloque.block_size %d",
			recurso_md->size, recurso_md->blocks, superbloque.block_size);

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	log_debug(logger, "O-Cantidad de bloques %d y tiene espacio en ultimo bloque: %d", cantidad_bloques, recurso_md->size < cantidad_bloques * superbloque.block_size);
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

	log_debug(logger, "IO-En cadena_cantidad_elementos_en_lista, la cantidad de elementos en cadena %s es %d", cadena, posicion);
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

	char** lista_bloques = string_get_string_as_array(recurso_md->blocks);

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);

	int ultimo_bloque = atoi(lista_bloques[cantidad_bloques - 1]); //Probar strtol(data, NULL, 10);

	cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);

	log_debug(logger, "O-Ultimo_bloque usado %d", ultimo_bloque);
	verificar_superbloque_temporal();
	return ultimo_bloque;
}

//Carga recursos en el bloque dado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_bloque(t_recurso_md* recurso_md, int* cantidad, int bloque)
{
	log_debug(logger, "I-Se ingresa a recurso_cargar_en_bloque con caracter %s, cantidad %d y bloque %d", recurso_md->caracter_llenado, *cantidad, bloque);

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

	log_debug(logger, "O-Se cargo en bloque %d con caracter %s quedando por cargar %d recurso(s)", bloque, recurso_md->caracter_llenado, *cantidad);
	verificar_superbloque_temporal();
}

//ALTER Carga recursos en el bloque dado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_bloque2(t_recurso_md* recurso_md, int* cantidad, int bloque)
{
	log_debug(logger, "I-Se ingresa a recurso_cargar_en_bloque con caracter %s, cantidad %d y bloque %d", recurso_md->caracter_llenado, cantidad, bloque);

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

	log_debug(logger, "O-Se cargo en bloque con caracter %s quedando por cargar %d recurso(s)", bloque, recurso_md->caracter_llenado, cantidad);
}

//DEPREC Carga recursos en el ultimo bloque registrado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_ultimo_bloque(t_recurso_md* recurso_md, int* cantidad, int nuevo_bloque)
{
	log_debug(logger, "I-Se ingresa a recurso_cargar_en_ultimo_bloque con caracter %s y cantidad %d", recurso_md->caracter_llenado, cantidad);

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

	log_debug(logger, "O-Se cargo en ultimo bloque con caracter %s quedando por cargar %d recurso(s)", recurso_md->caracter_llenado, cantidad);
}

//DEPREC Carga recursos en el ultimo bloque registrado hasta que no quede espacio en el o no hayan mas recursos para cargar
void metadata_cargar_en_ultimo_bloque2(t_recurso_md* recurso_md, int* cantidad, int nuevo_bloque)
{
	log_debug(logger, "I-Se ingresa a recurso_cargar_en_ultimo_bloque con caracter %s y cantidad %d", recurso_md->caracter_llenado, cantidad);

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

	log_debug(logger, "O-Se cargo en ultimo bloque con caracter %s quedando por cargar %d recurso(s)", recurso_md->caracter_llenado, cantidad);
}

//Busca y devuelve el primer bloque libre en el bitmap, si no existiera finaliza con error
int superbloque_obtener_bloque_libre()
{
	log_debug(logger, "I-Se ingresa a superbloque_obtener_bloque_libre");

	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, superbloque_size, LSB_FIRST);
	int posicion;
	int posicion_maxima = superbloque.blocks - 1;

	for(posicion = 0; posicion < posicion_maxima && bitarray_test_bit(bitmap, posicion); posicion++);

	if(bitarray_test_bit(bitmap, posicion))
	{
		log_error(logger, "No hay bloques libres para almacenar nuevos recursos en el bitmap del superbloque");//. Se finaliza por falta de espacio en los bloques"); exit(EXIT_FAILURE);
		posicion = -1; //VALOR DE ERROR
	}
	else
	{
		bitarray_set_bit(bitmap, posicion);
	}

	bitarray_destroy(bitmap);

	log_debug(logger, "O-Bloque libre obtenido %d", posicion);
	return posicion;
}

//Agrega a la lista de blocks de la metadata un bloque nuevo obtenido a traves del bitmap del superbloque y a su vez lo devuelve
void metadata_agregar_bloque_a_lista_de_blocks(t_recurso_md* recurso_md, int bloque)
{
	log_debug(logger, "I-Se ingresa a metadata_agregar_bloque_a_lista_de_blocks con blocks %s, block_count %d y bloque a agregar %d",
			recurso_md->blocks, recurso_md->block_count, bloque);
	int cantidad_bloques_original = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	cadena_sacar_ultimo_caracter(recurso_md->blocks);

	if(cantidad_bloques_original != 0)
	{
		string_append_with_format(&recurso_md->blocks, ",%d]", bloque);
	}
	else
	{
		string_append_with_format(&recurso_md->blocks, "%d]", bloque);
	}
	recurso_md->block_count++;

	log_debug(logger, "O-Se agrega bloque libre %d a blocks quedando BLOCK_COUNT %d y BLOCKS %s",
			bloque, recurso_md->block_count, recurso_md->blocks);
}

//Saca el ultimo caracter a una cadena de caracteres redimensionando el espacio de memoria
void cadena_sacar_ultimo_caracter(char* cadena)
{
	int tamanio_final_cadena = strlen(cadena);
	cadena[tamanio_final_cadena - 1] = '\0';
}

//Calcula y actualiza el valor del md5 del archivo teniendo en cuenta el conjunto de bloques que ocupa el recurso en el archivo de Blocks.ims
void metadata_actualizar_md5(t_recurso_md* recurso_md)
{
	log_debug(logger, "I-Se ingresa a metadata_actualizar_md5");

	char* concatenado;
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

	log_debug(logger, "O-MD5 en metadata actualizado a %s", recurso_md->md5_archivo);
}

//Calcula y actualiza el valor del md5 del archivo teniendo en cuenta el conjunto de bloques que ocupa el recurso en el archivo de Blocks.ims
void metadata_actualizar_md5_alter(t_recurso_md* recurso_md)
{
	log_debug(logger, "I-Se ingresa a metadata_actualizar_md5");

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
		log_error(logger,"O-No hay memoria suficiente como para calcular el MD5 del recurso con caracter de llenado %s", recurso_md-> caracter_llenado);
		return;
	}
	free(concatenado);

	log_debug(logger, "O-MD5 en metadata actualizado a %s", recurso_md->md5_archivo);
}

//Obtiene el conjunto de bloques concatenado que el recurso esta ocupando en el archivo de Blocks.ims devolviendo el tamanio del mismo
int blocks_obtener_concatenado_segun_lista(t_recurso_md* recurso_md, char** concatenado)
{
	log_debug(logger, "I-Se ingresa a blocks_obtener_concatenado_segun_lista");

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(recurso_md->blocks);
	int tamanio_concatenado = superbloque.block_size * cantidad_bloques;
	*concatenado = malloc(tamanio_concatenado);

	if(*concatenado == NULL)
	{
		tamanio_concatenado = -1; //Valor de retorno con error manejado en la funcion que hace el llamado
	}
	else
	{
		int bloque;
		char** lista_de_bloques = string_get_string_as_array(recurso_md->blocks);
		char* posicion_desde;
		char* posicion_hacia = *concatenado;

		for(int i = 0; i < cantidad_bloques; i++)
		{
			bloque = atoi(lista_de_bloques[i]);
			posicion_desde = blocks_address + bloque * superbloque.block_size;
			memcpy(posicion_hacia, posicion_desde, superbloque.block_size);
			posicion_hacia += superbloque.block_size;
		}
		cadena_eliminar_array_de_cadenas(&lista_de_bloques, cantidad_bloques);
	}

	log_debug(logger, "O-Bloques concatenados en direccion de memoria %p con tamanio %d", *concatenado, tamanio_concatenado);
	return tamanio_concatenado;
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

//DEPREC Calcula el valor de md5 de la cadena de texto apuntada por el puntero cadena y de tamanio length, y devuelve el puntero a donde se almacena
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

//Sincroniza de manera forzosa SuperBloque.ims con el espacio de memoria mapeado
void superbloque_actualizar_bitmap_en_archivo()
{
	log_debug(logger, "I-Se ingresa a superbloque_actualizar_bitmap_en_archivo");
	verificar_superbloque_temporal();
	int superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	lseek(superbloque_fd, 2 * sizeof(uint32_t), SEEK_SET);
	write(superbloque_fd, superbloque.bitmap, bitmap_size);

	close(superbloque_fd);
	verificar_superbloque_temporal();
	log_debug(logger, "O-Bitmap en archivo SuperBloque.ims actualizado");

}

//TODO AGREGAR LO DE LA COPIA DE LA COPIA EN MEMORIA

//Sincroniza de manera forzosa el archivo de Blocks.ims con el espacio de memoria mapeado
void blocks_actualizar_archivo()
{
	log_debug(logger, "I-Se ingresa a blocks_actualizar_archivo");
	msync(blocks_address, blocks_size, MS_SYNC);
	log_debug(logger, "O-Archivo Blocks.ims actualizado con los valores de la memoria");
}

//Actualiza la metadata del recurso en memoria al archivo. Si este no existe lo crea con los valores actualizados
void recurso_actualizar_archivo(t_recurso_data* recurso_data)
{
	log_debug(logger, "Info-Se ingresa a recurso_actualizar_archivo de recurso %s", recurso_data->nombre);

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
	log_debug(logger, "Info-Se ingresa a recurso_descartar_cantidad para el recurso %s y la cantidad %d", recurso_data->nombre, cantidad);

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
			superbloque_liberar_bloques_en_bitmap(recurso_data->metadata->blocks);
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

void superbloque_liberar_bloques_en_bitmap(char* blocks)
{
	log_debug(logger, "I-Ingreso a superbloque_liberar_bloques_en_bitmap con BLOCKS %s", blocks);

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(blocks);
	int bloque;
	char** lista_bloques = string_get_string_as_array(blocks);
	t_bitarray* bitmap = bitarray_create_with_mode(superbloque.bitmap, superbloque_size, LSB_FIRST);

	for(int i = 0; i < cantidad_bloques; i++)
	{
		bloque = atoi(lista_bloques[i]);
		bitarray_clean_bit(bitmap, bloque);
	}

	bitarray_destroy(bitmap);
	cadena_eliminar_array_de_cadenas(&lista_bloques, cantidad_bloques);

	log_debug(logger, "O-Bloques liberados en bitmap del superbloque");
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


//Sincroniza de manera forzosa SuperBloque.ims con el espacio de memoria mapeado
void superbloque_actualizar_blocks_en_archivo()
{
	log_debug(logger, "I-Se ingresa a superbloque_actualizar_blocks_en_archivo");
	int superbloque_fd = abrir_archivo_para_lectura_escritura(superbloque_path);

	lseek(superbloque_fd, sizeof(uint32_t), SEEK_SET);
	write(superbloque_fd, &superbloque.blocks, sizeof(uint32_t));

	close(superbloque_fd);
	verificar_superbloque_temporal();
	log_debug(logger, "O-Blocks en archivo SuperBloque.ims actualizado");
}

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

//FUNCIONES AUN EN DESARROLLO //TODO Ver bien (con el video) que corresponde verificar y que no de los archivos, y/o inicializar la memoria con los datos de los archivos directamente

//Inicia la secuencia de chequeos y reparaciones del File System Catastrophe Killer
void fsck_iniciar()
{
	fsck_chequeo_de_sabotajes_en_superbloque();
	fsck_chequeo_de_sabotajes_en_files();
}

//Realiza los chequeos propios del superbloque
void fsck_chequeo_de_sabotajes_en_superbloque()
{
	superbloque_validar_integridad_cantidad_de_bloques();
	superbloque_validar_integridad_bitmap();
	//agregar aca la actualizacion del archivo
}

//FIXME hacerlo en base a recursos fisicos! Valida que la cantidad de bloques en el archivo Superbloque.ims se corresponda con la cantidad real en el recurso fisico
void superbloque_validar_integridad_cantidad_de_bloques()
{
	superbloque.blocks = blocks_size / superbloque.block_size; //TODO usar el tamanio de bloque real del archivo Blocs.ims el cual debe ser equivalente
	memcpy(superbloque_address, &superbloque.blocks, sizeof(uint32_t));
	msync(superbloque_address, sizeof(uint32_t), MS_SYNC);
}

//Valida que los bloques indicados en el bitmap del superbloque realmente sean los unicos ocupados
//TODO Agregar logs
void superbloque_validar_integridad_bitmap()
{
	char* bloques_ocupados = files_obtener_cadena_con_bloques_ocupados(); //TODO

	superbloque_setear_bitmap_a_cero();
	superbloque_setear_bloques_en_bitmap(bloques_ocupados);
	superbloque_actualizar_bitmap_en_archivo();

	free(bloques_ocupados);
}

//FIXME Debe hacerse en base a los blocks en los archivos (creo) Devuelve una lista en formato cadena con los bloques ocupados por el total de recursos
char* files_obtener_cadena_con_bloques_ocupados()
{
	log_debug(logger, "Se ingresa a files_obtener_cadena_con_bloques_ocupados");
	char* bloques;
	int cantidad_recursos = files_cantidad_recursos();
	int posicion_ultimo_recurso = cantidad_recursos - 1;
	bloques = string_duplicate(lista_recursos[0].metadata->blocks);

	for(int i = 1; i < cantidad_recursos; i++)
	{
		cadena_sacar_ultimo_caracter(bloques);
		char* string_buffer = string_duplicate(lista_recursos[i].metadata->blocks);
		string_buffer[0] = ',';
		string_append_with_format(&bloques, "%s", string_buffer);
		free(string_buffer);
	}
	log_debug(logger, "Los bloques ocupados por el total de recursos son %s"), bloques;
	return bloques;
}

void superbloque_setear_bloques_en_bitmap(char* bloques_ocupados)
{
	log_debug(logger, "Ingreso a superbloque_setear_bloques_en_bitmap");

	char** lista_bloques = malloc(sizeof(string_get_string_as_array(bloques_ocupados)));
	lista_bloques = string_get_string_as_array(bloques_ocupados);
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

//TODO actualizar_metadata_inicial en base a SB+B? ENTIENDO QUE NO SERIA NECESARIO Y HASTA ESTARIA MAL (PREGUNTAR)

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


///////////////////////FUNCIONES DEL SUPERBLOQUE RELATIVAS A LA INICIALIZACION DEL FILE SYSTEM//////////////////////////////////////////

//////////////////////FUNCIONES DEL SUPERBLOQUE RELATIVAS AL USO POSTERIOR A LA INICIALIZACION DEL FILE SYSTEM///////////////////////////

/////////////////////FUNCION DEL BLOCKS RELATIVAS AL USO POSTERIOR A LA INICIALIZACION DEL FILE SYSTEM/////////////////////

/////////////////////FUNCIONES DE LOS RECURSOS RELATIVAS A LA REALIZACION DE LAS TAREAS//////////////////////////////////////

