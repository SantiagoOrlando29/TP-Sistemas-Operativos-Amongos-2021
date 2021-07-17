#include "utils_mongostore.h"

//Prototipos de funciones en desarrollo


int main(void)
{
	logger = log_create(ARCHIVO_LOGS, PROGRAMA, LOGS_EN_CONSOLA, NIVEL_DE_LOGS_MINIMO);

	if(logger == NULL)
	{
		printf("No se puede abrir el archivo de logs para iniciar el programa\n");
		exit(EXIT_FAILURE);
	}

	log_debug(logger, "LOGGER OK");

	leer_config();
	log_debug(logger, "Testeo config, la IP es: %s", configuracion.ip);

	file_system_iniciar();

	recurso_generar_cantidad(OXIGENO, 2);
/*	int fd = abrir_archivo_para_lectura_escritura(blocks_path);
	char* deblocks =malloc(41);
	read(fd,deblocks, 40);
	printf("aaa %c\n", deblocks[15]);
	free(deblocks);close(fd);*/
	/*
	recurso_generar_cantidad(COMIDA, 3);
	recurso_generar_cantidad(BASURA, 4);
	recurso_generar_cantidad(BASURA, 5);

	log_info(logger, "Finaliza main");

	return EXIT_SUCCESS;*/
	return 8;
}

//TODO actualizar_metadata_inicial en base a SB+B? ENTIENDO QUE NO SERIA NECESARIO Y HASTA ESTARIA MAL (PREGUNTAR)
//TODO generar md5
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
///**Elimina la cantidad cantidad de caracteres de llenado del archivo y de la metadata correspondiente al recurso dado
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
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*void mandar_primer_ubicacion_sabotaje()
{
}*/

//Inicia la secuencia de chequeos y reparaciones del File System Catastrophe Killer
/*void fsck_iniciar()
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

//Valida que la cantidad de bloques en el superbloque se corresponda con la cantidad real en el recurso fisico
void superbloque_validar_integridad_cantidad_de_bloques()
{
	superbloque.blocks = blocks_size / superbloque.block_size; //TODO usar el tamanio de bloque real del archivo Blocs.ims el cual debe ser equivalente
	memcpy(superbloque_address, &superbloque.blocks, sizeof(uint32_t));
	msync(superbloque_address, sizeof(uint32_t), MS_SYNC);
}

//Valida que los bloques indicados en el bitmap del superbloque realmente sean los unicos ocupados
void superbloque_validar_integridad_bitmap()
{
	char* bloques_ocupados = files_obtener_lista_de_bloques_ocupados(); //TODO

	superbloque_setear_bitmap_a_cero();
	superbloque_setear_bloques_en_bitmap(bloques_ocupados);
	superbloque_actualizar_archivo();

	free(bloques_ocupados);
}

//Devuelve una lista en formato cadena con los bloques ocupados por el total de recursos
char* files_obtener_bloques_ocupados()
{
	log_debug(logger, "Se ingresa a files_obtener_bloques_ocupados");
	char* bloques;
	int cantidad_recursos = sizeof(lista_recursos) / sizeof(t_recurso_data);
	int posicion_ultimo_recurso = cantidad_recursos - 1;
	bloques = string_duplicate(lista_recursos[0].metadata->blocks);
	cadena_sacar_ultimo_caracter(bloques);
	for(int i = 1; i < cantidad_recursos; i++)
	{
		char* string_buffer = string_duplicate(lista_recursos[i].metadata->blocks);
		string_buffer[0] = ',';
		string_append_with_format(bloques, "%s", string_buffer); //ERROR
		if(i != posicion_ultimo_recurso)
		{
			cadena_sacar_ultimo_caracter(bloques);
		}
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

	int cantidad_bloques = cadena_cantidad_elementos_en_lista(bloques_ocupados);

	int ultimo_bloque = atoi(lista_bloques[cantidad_bloques - 1]);         //Probar strtol(data, NULL, 10);

	//for();//TODO iterando el total de bloques hasta terminar la lista

	//puede hacer falta liberar uno por uno todos los bloques con un for(int i = 0; i < cantidad_bloques; i++)
	free(lista_bloques);

	log_debug(logger, "Bitmap del SuperBloque actualizado en memoria");


}

//Realiza los chequeos propios de los recursos
void fsck_chequeo_de_sabotajes_en_files()
{
	files_validar_tamanio_de_archivos();
	files_validar_block_counts();
	files_validar_blocks();
}

void files_validar_tamanio_de_archivos()
{
}

char* files_obtener_lista_de_bloques_ocupados(){
	//TODO
	return "";
};





 //FSCK, bitacora, signal,
char** prueba = string_split("La casa,dos_diez"," ,_");
	log_debug(logger, "Se genero el recurso solicitado. Se probara iniciar el servidor a traves de un hilo %s %s %s %s %p", prueba[0], prueba[1], prueba[2], prueba[3], prueba[4], prueba[5], prueba[6]);
	pthread_t servidor;
	int hilo_servidor = 1;
	if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion))!=0){
		log_info(logger, "Falla al crearse el hilo");
	}
	pthread_join(servidor,NULL);
	log_info(logger, "Main: Se genero un hilo");
	//mapeo();
	return EXIT_SUCCESS;
}*/
