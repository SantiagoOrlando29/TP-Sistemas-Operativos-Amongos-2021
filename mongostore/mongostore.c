#include "mongostore.h"
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>

///////
#define ARCHIVO_LOGS "mongostore.log"
#define PROGRAMA argv[0]
#define LOGS_EN_CONSOLA 1
#define NIVEL_DE_LOGS_MINIMO LOG_LEVEL_DEBUG
#define ARCHIVO_CONFIGURACION "mongostore.config"
///////
t_log* logger;
config_struct ims_config;
int blocks_fd;
///////
void validar_argumentos(int cantidad_parametros, char* programa);
void cargar_configuracion_en_struct();
void iniciar_file_system(char* argv[]);
int existe_file_system();
int existe_path(char*);
void recuperar_file_system_existente();
void crear_file_system(char* argv[]);
void crear_directorios();
void crear_directorio_si_no_existe(char* path);

///////
pthread_mutex_t mutex_blocks;
int blocks_size=128; //acepta 8 caracteres de char
int blocks=4;

t_superbloque superbloque;
char* bloques_copia;

void levantar_blocks();

void main (int argc, char *argv[]){
	logger = log_create(ARCHIVO_LOGS, PROGRAMA, LOGS_EN_CONSOLA, NIVEL_DE_LOGS_MINIMO);
	
	if(logger == NULL)
	{
		printf("No se puede abrir el archivo de logs para iniciar el programa\n");
		exit(EXIT_FAILURE);
	}

	validar_argumentos(argc, argv);

	cargar_configuracion_en_struct();
	
	iniciar_file_system(argv[]);

	pthread_t servidor;
	int hilo_servidor = 1;
	if(pthread_create(&servidor,NULL,(void*)iniciar_servidor,&ims_config) != 0)
	{
		log_info(logger, "Falla al crearse el hilo");
		log_destroy(logger);

		exit(EXIT_FAILURE);
	}
	pthread_join(servidor,NULL);

	//mapeo();

	log_destroy(logger);
	exit(EXIT_SUCCESS);
}

//Se valida que el numero de parametros ingresados al iniciar el programa sea el correcto. De no serlo se finaliza indicando el error
void validar_argumentos(int cantidad_parametros, char* programa){
	if (cantidad_parametros != 1 && cantidad_parametros != 3)
	{
		printf("Numero de parametros incorrecto\nModos de ejecucion de \"%s\":\n"
				" 1) %s (Si no existe el File System se pedira la cantidad y el tamaño de bloques para crearlo, debe existir el archivo de configuracion)\n"
				" 2) %s <tamanio de bloques> <cantidad de bloques> (El tamanio y la cantidad de bloques serviran para crear el SuperBloque)\n",
				programa, programa, programa);
		log_info(logger, "Los parametros para la ejecucion del programa no son los correctos");
		exit(EXIT_FAILURE);
	}
}

/*Crea un t_config con el archivo existente de configuracion guardando sus valores en la estructura global ims_config y
destruyendo luego la estructura t_config*/
void cargar_configuracion_en_struct()
{
	printf("*** entro a cargar_configuracion_en_struct\n");

	//Se carga el archivo de configuracion. Si no es posible se imprime en pantalla y finaliza el programa
	t_config* config = config_create(ARCHIVO_CONFIGURACION);
	if(config == NULL)
	{
		log_info(logger, "No se puede cargar la configuracion. Se finaliza el programa");
		exit(EXIT_FAILURE);
	}
	printf("*** * lc se abrio %s\n", config->path);

	//Se carga la estructura global "ims_config"
	// *** Si no se precisa que sea global puede pasarse a local retornandolo y eliminando el destroy
	ims_config.punto_montaje = config_get_string_value(config,"PUNTO_MONTAJE");
	char* ip;
	printf("Ingrese por favor la direccion IP donde se conectara \n");
	scanf("%s", ip);
	ims_config.ip = ip;//config_get_string_value(config, "IP");// *** el archivo de configuracion no tiene IP
    ims_config.puerto = config_get_string_value(config, "PUERTO");
    ims_config.tiempo_sincronizacion = config_get_int_value(config,"TIEMPO_SINCRONIZACION");
    ims_config.posiciones_sabotaje = config_get_array_value(config,"POSICIONES_SABOTAJE");
    printf("*** * lc: se cargo en la estructura \"ims_config\" en los campos de:\n  PUNTO_MONTAJE, IP, PUERTO, "
		"TIEMPO_SINCRONIZACION y POSICIONES_SABOTAJES\n  con los valores:\n  %s, %s, %s, %d y %s\n", ims_config.punto_montaje,
		ims_config.ip, ims_config.puerto, ims_config.tiempo_sincronizacion, ims_config.posiciones_sabotaje[0]);

    config_destroy(config);

    printf("*** *** salgo de cargar_configuracion_en_struct\n\n");
}
	
/*Inicia el File System verificando la existencia previa del mismo, creandolo si no existiera de antes, recuperandolo si ya
existiera y levantando posteriormente a memoria el archivo Blocks.ims*/
void iniciar_file_system(int argc, char* argv[])
{
	printf("*** entro a iniciar_file_system\n");

	if(existe_file_system())
	{
		printf("*** ** existe el file system\n");
		recuperar_file_system_existente();
	}
	else
	{
		printf("*** ** no existe el file system\n");
		crear_file_system(argc, argv[]);
	}
	levantar_superbloque();
	levantar_blocks();

	printf("*** *** salgo de iniciar_file_system\n");
}

//Devuelve true si existe el archivo Blocks.ims en la ruta dada por el punto de montaje
int existe_file_system()
{
	printf("*** * entro a -y salgo de?- existe_file_system SACAR HARCODEO\n");
	return (existe_path("/home/utnso/polus/Blocks.ims") && existe_path("/home/utnso/polus/SuperBloque.ims"));
}

//Devuelve true si el path recibido corresponde a una ruta ya existente (verifica existencia de archivos y/o carpetas)
int existe_path(char* path)
{
	printf("*** * entro a -y salgo de?- existe_path\n");
	return (access(path, F_OK) == 0);
}

//Abre el archivo Blocks.ims (finaliza el programa si no es posible)
//Suponemos en principio creados los archivos de files
void recuperar_file_system_existente()
{
	printf("*** entro a recuperar_file_system_existente\n");
	//crear_directorios_si_no_existen();
	levantar_superbloque();
	levantar_blocks();
	
	printf("*** *** salgo de recuperar_file_system_existente\n");
}

/*Crea los directorios (si no existen) para los archivos del File System, abre el archivo de Blocks.ims (si no existe lo crea)
(si no puede crearlo o puede abrirlo, finaliza el programa). Por último define el tamanio que tendra dicho archivo*/
void crear_file_system(int cantidad_parametros_programa, char* parametros_programa[])
{
	printf("*** entro a crear_file_system\n");
	crear_directorios_si_no_existen();
	if(existe_path(/home/utnso/polus/SuperBloque.ims) == 0)
	{
		crear_archivo_superbloque(cantidad_parametros_programa, char* parametros_programa[]);
	}
	levantar_superbloque();
	crear_arhivo_blocks();
	levantar_blocks();
	
	printf("*** *** salgo de crear_file_system\n\n");

/*	int tamanio_blocks;
	if(existe_path(/home/utnso/polus/SuperBloque.ims) == 0)
	{	
		if(argc == 1)
		{
			printf("Ingrese el tamanio de los bloques:\n");
			scanf("%d", &superbloque.block_size);
			printf("Ingrese la cantidad de bloques:\n");
			scanf("%d", &superbloque.blocks);
		}
		else
		{
			superbloque.block_size = atoi(argv[1]);
			superbloque.blocks = atoi(argv[2]);
		}
	}
	else
	{
		levantar_superbloque();
	}
	
	tamanio_blocks	= superbloque.block_size * superbloque.blocks;
	printf("***superbloque.block_size = %d;   superbloque.blocks = %d\n", superbloque.block_size, superbloque.blocks);
	
	char* blocks_path;
	blocks_path = "/home/utnso/polus/Blocks.ims"; //*** sacar hardcode
	blocks_fd = open(blocks_path, O_CREAT|O_RDWR, S_IRWXU);
	if(blocks_fd == -1)
	{
		log_info(logger, "No pudo ser creado el archivo %s. Se finaliza el programa\n", blocks_path);
		exit(EXIT_FAILURE);
	}
	printf("*** ** se creo y abrio correctamente el archivo %s para lectura y escritura\n", blocks_path);

	//Se cambia el tamanio del archivo fisico
	if(ftruncate(blocks_fd, tamanio) == -1)
	{
		log_info(logger, "No se le pudo asignar el tamanio al archivo %s. Se finaliza el programa\n", blocks_path);
		exit(EXIT_FAILURE);
	}

	printf("*** ** se le pudo asignar tamanio al archivo %s\n", blocks_path);

*/
}
//crear_directorios_si_no_existen();crear_archivo_superbloque();levantar_superbloque();crear_arhivo_blocks();levantar_blocks();

//Crea el archivo Superbloque.ims usando los parametros recibidos al iniciar el programa o solicitandolos
void crear_archivo_superbloque(int cantidad_parametros_programa, char* parametros_programa[])
{
	printf("*** entro a crear_archivo_superbloque - sacar hardcodeo");
	FILE* superbloque_file;
	if(superbloque_file = fopen("/home/utnso/polus/SuperBloque.ims","w"))
	{
		log_info(logger, "No puede ser creado el archivo %s\n. Se finaliza el programa", "/home/utnso/polus/SuperBloque.ims");
		exit(EXIT_FAILURE);
	}
	
	int tamanio_bloques, cantidad_bloques;
	char* bitmap;
	if (cantidad_parametros_programa == 1)
	{
		printf("Ingrese el tamanio de los bloques:\n");
		scanf("%d", tamanio_bloques);
		getchar();//para eliminar del buffer el fin de linea
		printf("Ingrese la cantidad de bloques:\n");
		scanf("%d", cantidad_bloques);
		getchar();//para eliminar del buffer el fin de linea
	}
	else
	{
		tamanio_bloques = atoi(parametros_programa[1]);
		cantidad_bloques = atoi(parametros_programa[2]);
	}
	bitmap = (char*) malloc(cantidad_bloques * sizeof(char)); //usar un bitmap real con las commons
	//setear a 0
	
	fprintf(superbloque_file, "BLOCK_SIZE=%d\n", tamanio_bloques);
	fprintf(superbloque_file, "BLOCKS=%d\n", cantidad_bloques);
	fprintf(superbloque_file, "BITMAP=%s\n", bitmap);

	free(bitmap);
	fclose(superbloque_file);
	printf("*** *** salgo de crear_archivo_superbloque");
}

void levantar_superbloque()
{
	FILE* superbloque_file;
	if(superbloque_file = fopen("/home/utnso/polus/SuperBloque.ims","w"))
	
}
	}
	else
	{
		levantar_superbloque();
	}
	
	t_config* archivo=config_create("SuperBloque.ims");
		
	superbloque.block_size = config_get_int_value(archivo,"BLOCK_SIZE");
	superbloque.blocks = config_get_int_value(archivo,"BLOCKS");
	superbloque.bitmap = (char *)malloc(superbloque.blocks * sizeof(char));
		
	(char *)malloc(superbloque.blocks * sizeof(char));
	
	int tamanio_blocks;
	tamanio_blocks	= superbloque.block_size * superbloque.blocks;
	printf("***superbloque.block_size = %d;   superbloque.blocks = %d\n", superbloque.block_size, superbloque.blocks);
	
	char* blocks_path;
	blocks_path = "/home/utnso/polus/Blocks.ims"; //*** sacar hardcode
	blocks_fd = open(blocks_path, O_CREAT|O_RDWR, S_IRWXU);
	if(blocks_fd == -1)
	{
		log_info(logger, "No pudo ser creado el archivo %s. Se finaliza el programa\n", blocks_path);
		exit(EXIT_FAILURE);
	}
	printf("*** ** se creo y abrio correctamente el archivo %s para lectura y escritura\n", blocks_path);

	//Se cambia el tamanio del archivo fisico
	if(ftruncate(blocks_fd, tamanio) == -1)
	{
		log_info(logger, "No se le pudo asignar el tamanio al archivo %s. Se finaliza el programa\n", blocks_path);
		exit(EXIT_FAILURE);
	}

	printf("*** ** se le pudo asignar tamanio al archivo %s\n", blocks_path);

	
//Si no existen, crea los directorios para el File System
void crear_directorios()
{
	printf("*** entro a crear_directorios - sacar 3 hardcodes\n");

	crear_directorio_si_no_existe("/home/utnso/polus");
	crear_directorio_si_no_existe("/home/utnso/polus/Files");
	crear_directorio_si_no_existe("/home/utnso/polus/Files/Bitacoras");

	printf("*** *** salgo de crear_directorios\n\n");
	return;
}

/*Crea el directorio para la ruta especificada. En caso de no poderse crear, finaliza el programa*/
void crear_directorio_si_no_existe(char* path)
{
	//Se verifica si la ruta ya existe y -en caso contrario- si puede crearse exitosamente
	if (existe_path(path) == 0 && mkdir(path, 0777))
	{
		log_info(logger, "No existia el directorio %s desde antes ni puede crearse. No pueden crearse el total de directorios del file system", path);
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("*** ** el directorio %s ya existia desde antes o fue creado con exito\n", path);
		return;
	}
}

/*Levanta a memoria con mmap el archivo de Blocks.ims ya abierto el cual luego de esto es cerrado
(puede que se precise dejarlo abierto para la sincronizacion de los datos)*/
void levantar_blocks()
{
	printf("*** entro a levantar_blocks - sacar 1 hardcodes\n");

	char* blocks_path;
	blocks_path = "/home/utnso/polus/Blocks.ims";
	blocks_fd = open(blocks_path, O_RDWR);
	if(blocks_fd == -1)
	{
		log_info(logger, "No pudo ser abierto el archivo %s para lectura y escritura", blocks_path);
		exit(EXIT_FAILURE);
	}
	printf("*** ** 1 - se abrio correctamente el archivo %s ya existente para lectura y escritura\n", blocks_path);
	// *** struct stat stat_file;
	int tamanio = superbloque.block_size * superbloque.blocks;

	void* address = mmap(NULL, tamanio, PROT_WRITE|PROT_READ, MAP_SHARED, blocks_fd, 0);

	printf("*** 2 - se mapeo correctamente blocks a la memoria - direccion de mapeo %p\n", address);

	close(blocks_fd);//puede que se precise dejarlo abierto para la sincronizacion de los datos ***
	printf("*** *** salgo de levantar_blocks\n");
}


////De aca para abajo esta sin revisar


/*
void algo()
{
	escribir_archivo_superbloque(string_repeat('0',blocks));
	printf("1\n");
	bloques_copia=string_repeat('0',blocks);
	printf("2\n");;
	crear_superbloque();
	printf("3\n");
}

//crear_recurso_metadata(133,3,"[1,2,3]",'O',"0123","/home/utnso/polus/Files/Oxigeno.ims");
//crear_recurso_metadata(32,2,"[4,5]",'C',"0123","/home/utnso/polus/Files/Comida.ims");
//crear_recurso_metadata(64,1,"[6]",'B',"0132465","/home/utnso/polus/Files/Basura.ims");
*/

/*Sube a memoria a traves del struct global superbloque los valores que se tienen en el archivo SuperBloque.ims
(ex void crear_superbloque())*/
void levantar_superbloque()
{
	printf("*** entro a crear_superbloque - sacar 1 hardcodes\n");
	t_config* superbloque_config = config_create("/home/utnso/polus/SuperBloque.ims"); //*** sacar hardcode

	printf("*** **1b\n");
	superbloque.block_size = (uint32_t)config_get_int_value(superbloque_config,"BLOCK_SIZE");
	printf("*** **2b\n");
	superbloque.blocks = (uint32_t)config_get_int_value(superbloque_config,"BLOCKS");
	printf("*** **3b\n");
	superbloque.bitmap = config_get_string_value(superbloque_config,"BITMAP");

	config_destroy(superbloque_config);

	printf("*** ** salgo de crear_superbloque\n");
}

/*Baja a disco al archivo SuperBloque.ims los valores contenidos en el struct global superbloque*/
void persistir_superbloque(char* bitmap)
//(ex) void escribir_archivo_superbloque(char* bitmap) ***
{
	printf("*** entro a persistir_superbloque (ver si se cargan bien los valores en el archivo) - sacar 1 hardcodes\n");
	FILE* archivo_superbloque;
	archivo_superbloque = fopen("/home/utnso/polus/SuperBloque.ims", "w"); //*** sacar hardcode

	fputs(string_from_format("BLOCK_SIZE=%d", blocks_size), archivo_superbloque);
	fputs(string_from_format("BLOCKS=%d", blocks), archivo_superbloque);
	fputs(string_from_format("BITMAP=%s", bitmap), archivo_superbloque);

	fclose(archivo_superbloque);
	printf("*** ** salgo de persistir_superbloque\n\n");
}

/*
void generar_oxigeno(int cant,char* bloques)
{
	printf("*** entro a generar_oxigeno - sacar 1/3 hardcodes\n");
	// size de archivo en bytes
	// char = 1 bytes
	bloques_recurso* oxigeno=asignar_bloques(cant,superbloque,'O',bloques);
	crear_recurso_metadata(oxigeno,"0xMD5oxigeno","/home/utnso/polus/Files/Oxigeno.ims");
	printf("*** ** salgo de generar_oxigeno\n\n");
}
void generar_comida(int cant,char* bloques)
{
	printf("*** entro a generar_comida - sacar 1/3 hardcodes\n");
	// size de archivo en bytes
	// char = 1 bytes
	bloques_recurso* comida=asignar_bloques(cant,superbloque,'C',bloques);
	crear_recurso_metadata(comida,"0xMD5oxigeno","/home/utnso/polus/Files/Comida.ims");
	printf("*** ** salgo de generar_comida\n\n");
}

void generar_basura(int cant,char* bloques)
{
	printf("*** entro a generar_basura - sacar 1/3 hardcodes\n");
	// size de archivo en bytes
	// char = 1 bytes
	bloques_recurso* basura=asignar_bloques(cant,superbloque,'B',bloques);
	crear_recurso_metadata(basura,"0xMD5oxigeno","/home/utnso/polus/Files/Basura.ims");
	printf("*** ** salgo de generar_basura\n\n");
}

bloques_recurso* recuperar_valores(char* path)
{
	printf("*** entro a recuperar_valores en %s\n", path);
	bloques_recurso* recurso;
	t_config* archivo=config_create(path);
	recurso->size=config_get_int_value(archivo,"SIZE");
	recurso->block_count=config_get_int_value(archivo,"BLOCK_COUNT");
	recurso->blocks=config_get_string_value(archivo,"BLOCKS");
	//recurso->caracter_llenado=config_get_string_value(archivo,"CARACTER_LLENADO"); TIRA ERROR
	recurso->caracter_llenado='0';

	printf("**** salgo de recuperar_valores en %s\n\n", path);
	return recurso;
}
*/
/*falta contemplar el caso donde se sobrepasa la cantidad de bloques*/
/* Tambien se escribe en bloques  los recursos y bitmap es modificado */
bloques_recurso* asignar_bloques(int cant,t_superbloque* sb,char caracter,char* bloques)
{

	printf("*** entro a asignar_bloques\n");
	bloques_recurso* recurso;
	char* bloques_asignados="[";
	int i=0;
	int inicio=0;
	int aux_cant=cant;
	recurso->block_count=0;

	pthread_mutex_lock(&mutex_blocks); //sector critica poruqe se usa superBloque  y bloques

	if(sb->bitmap[i]=='0')
	{
		string_append_with_format(&bloques_asignados,"%s",i);
		for(size_t cont=0;cont<sb->block_size;cont++)
		{
			if(cont < cant )
			{
				bloques[inicio+cont]=caracter;
				aux_cant= aux_cant - 1;
			}
		}
		sb->bitmap[i]='1';
		recurso->block_count++;
		cant= cant-aux_cant; // vemos si basta con solo un bloque
	}
	else
	{
		/*
		 * Vimos que esta ocupado
		 * Vemos si esta ocupado con los mismos recursos que generaremos
		 * Solo vemos si el primer byte es del mismo caracter recurso
		 */
		if(sb->bitmap[i]=='1' && bloques[inicio]==caracter)
		{
			string_append_with_format(&bloques_asignados,"%s",i);
		}
	}


	if(cant > 0)
	{ //Si basta con  el primer bloeque vacio o necesitamos mas bloques o el primer bloque esta ocupado
		for(i=1;i<sb->blocks;i++)
		{ //recorremos el bitmap
			inicio=i*sb->block_size; //definimos el inicio del bloque
			if(sb->bitmap[i]=='0')
			{ //vemos si esta libre
				string_append_with_format(&bloques_asignados,",%s",i);
				for(size_t cont=0;cont<sb->block_size;cont++)
				{
					if(cont < cant)
					{
						bloques[inicio + cont]=caracter;
						aux_cant= aux_cant - 1;
					}
				}
				sb->bitmap[i]='1';
				recurso->block_count++;
			}
			else
			{
				if(sb->bitmap[i]=='1' && bloques[inicio]==caracter)
				{
					string_append_with_format(&bloques_asignados,",%s",i);
				}
			}
		}
	}
	/*poner en una funcion sincronizacion escribe en el archivo el cambio del bitmap*/
	//escribir_archivo_superbloque(sb.bitmap);

	pthread_mutex_unlock(&mutex_blocks);
	string_append(&bloques_asignados,"]");

	recurso->blocks=bloques_asignados;
	recurso->caracter_llenado=caracter;
	recurso->size=cant; // char = 1 byte

	printf("*** ** salgo de asignar_bloques\n\n");
	return recurso; // un string de donde se han guardado
}

void crear_recurso_metadata(bloques_recurso* recurso, char* md5, char* path)
{
	printf("*** entro a crear_recurso_metadata en %s\n", path);
	FILE* archivo;

	archivo = fopen(path,"w");

	fputs(string_from_format("SIZE=%d\n", recurso->size), archivo);
	fputs(string_from_format("BLOCK_COUNT=%d\n", recurso->block_count), archivo);
	fputs(string_from_format("BLOCKS=%s\n", recurso->blocks), archivo);
	fputs(string_from_format("CARACTER_LLENADO=%c\n", recurso->caracter_llenado), archivo);
	fputs(string_from_format("MD5_ARCHIVO=%s", md5), archivo);

	fclose(archivo);
	printf("**** salgo de crear_recurso_metadata en %s\n\n", path);
}

//verificar modos de apertura de archivos necesarios
/*
              ┌─────────────┬───────────────────────────────┐
              │fopen() mode │ open() flags                  │
              ├─────────────┼───────────────────────────────┤
              │     r       │ O_RDONLY                      │
              ├─────────────┼───────────────────────────────┤
              │     w       │ O_WRONLY | O_CREAT | O_TRUNC  │
              ├─────────────┼───────────────────────────────┤
              │     a       │ O_WRONLY | O_CREAT | O_APPEND │
              ├─────────────┼───────────────────────────────┤
              │     r+      │ O_RDWR                        │
              ├─────────────┼───────────────────────────────┤
              │     w+      │ O_RDWR | O_CREAT | O_TRUNC    │
              ├─────────────┼───────────────────────────────┤
              │     a+      │ O_RDWR | O_CREAT | O_APPEND   │
              └─────────────┴───────────────────────────────┘
    
	@NAME: string_from_format
	* @DESC: Crea un nuevo string a partir de un formato especificado
	*
	* Ejemplo:
	* char* saludo = string_from_format("Hola %s", "mundo");
	*
	* => saludo = "Hola mundo"
	*
	char*   string_from_format(const char* format, ...);
	para generar la ruta de los archivos en tiempo de ejecución por lo que no pueden ser variables globales, no? */
