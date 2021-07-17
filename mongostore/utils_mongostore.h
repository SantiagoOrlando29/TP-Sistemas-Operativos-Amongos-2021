//utils_mongostore.h 15072021 03.16
//valgrind --tool=memcheck --leak-check=full ./mongostore
#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <commons/log.h>
#include <commons/config.h>
#include<commons/string.h>
#include <string.h>
//#include<commons/collections/list.h>
//#include<commons/collections/queue.h>
#include<commons/bitarray.h>
//#include<sys/types.h>
#include<sys/stat.h>
//#include<sys/socket.h>
#include<sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

//#include <unistd.h>
//#include <sys/types.h>
#include <openssl/md5.h>
/*
#include<netdb.h>
#include<math.h>
#include<errno.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>
#include <openssl/md5.h>
#include <stdlib.h>
#define TAMANIO_MD5 33
#define MAXIMO_LISTA_BLOCKS ((superbloque.blocks + 1) * 2)
//#include<messages_lib/messages_lib.h>*/

#define ARCHIVO_LOGS "mongostore.log"
#define PROGRAMA "mongostore"
#define LOGS_EN_CONSOLA 1
#define NIVEL_DE_LOGS_MINIMO LOG_LEVEL_DEBUG
#define ARCHIVO_CONFIGURACION "mongostore.config"

//TODO VERIFICAR TIPO DE DATO PARA posiciones_sabotaje
typedef struct {
	char* ip;
	char* puerto;
	char* punto_montaje;
	int tiempo_sincronizacion;
	char** posiciones_sabotaje;
} t_configuracion;

typedef struct {
	int size;
	int block_count;
	char* blocks;
	char* caracter_llenado;
	char md5_archivo[33];
} t_recurso_md;

typedef struct {
	char* nombre;
	char caracter_llenado;
	char* nombre_archivo;
	char* ruta_completa;
	t_recurso_md* metadata;
} t_recurso_data;

typedef enum {
	OXIGENO,
	COMIDA,
	BASURA
} recurso_code;

typedef enum {
	GENERAR,
	CONSUMIR,
	DESCARTAR
} accion_code;

struct {
	uint32_t block_size;
	uint32_t blocks;
	char* bitmap;
} superbloque;

t_log* logger;
t_configuracion configuracion;

char* files_path;
char* bitacoras_path;

char* superbloque_path;
char* superbloque_address;
size_t superbloque_size;
size_t bitmap_size;

char* blocks_path;
char* blocks_address; //Direccion en memoria de la copia principal de Blocks.ims
size_t blocks_size;

int superbloque_es_nuevo; //Flag para decidir crear un nuevo Blocks por mas que ya existiera desde antes


//pthread_mutex_t mutex_blocks;
//char* bloques_copia;



//Prototipos  16 julio
void crear_directorio_si_no_existe(char*);
int existe_en_disco(char*);
int abrir_archivo_para_lectura_escritura(char*);
void crear_archivo(char*);
FILE* abrir_archivo_para_escritura(char*);
void dar_tamanio_a_archivo(char*, size_t);
int cadena_cantidad_elementos_en_lista(char*);
void cadena_sacar_ultimo_caracter(char*);
void cadena_eliminar_array_de_cadenas(char***, int);
void cadena_calcular_md5(const char *cadena, int, char*);
char* cadena_calcular_md5_alter(const char *cadena, int );
void leer_config();
void file_system_iniciar();
void file_system_generar_rutas_completas();
void files_cargar_rutas_de_recursos();
void file_system_crear_directorios_inexistentes();
void superbloque_validar_existencia();
void superbloque_generar_estructura();
void superbloque_generar_estructura_desde_archivo();
void superbloque_asignar_memoria_a_bitmap();
void superbloque_generar_estructura_con_valores_ingresados_por_consola();
void superbloque_tomar_valores_desde_consola();
void superbloque_setear_bitmap_a_cero();
void superbloque_validar_existencia_del_archivo();
void superbloque_cargar_archivo();
void superbloque_mapear_archivo_a_memoria();
void superbloque_actualizar_archivo();
int superbloque_obtener_bloque_libre();
void blocks_validar_existencia();
void blocks_validar_existencia_del_archivo();
void blocks_mapear_archivo_a_memoria();
void blocks_actualizar_archivo();
int blocks_obtener_concatenado_segun_lista(t_recurso_md*, char**);
void recurso_generar_cantidad(recurso_code, int);
void recurso_validar_existencia_metadata_en_memoria(t_recurso_data*);
void recurso_actualizar_archivo(t_recurso_data*);
void recurso_descartar_cantidad(recurso_code, int);
void metadata_generar_cantidad(t_recurso_md*, int);
void metadata_agregar_bloque_a_lista_de_blocks(t_recurso_md*, int);
int metadata_tiene_espacio_en_ultimo_bloque(t_recurso_md*);
void metadata_cargar_en_bloque(t_recurso_md*, int*, int);
void metadata_cargar_en_bloque2(t_recurso_md*, int*, int);
void metadata_cargar_en_ultimo_bloque(t_recurso_md*, int*, int);
void metadata_cargar_en_ultimo_bloque2(t_recurso_md*, int*, int);
int metadata_ultimo_bloque_usado(t_recurso_md*);
void metadata_levantar_de_archivo_a_memoria_valores_variables(t_recurso_md*, char*);
void metadata_setear_con_valores_default_en_memoria(t_recurso_md*);
void metadata_actualizar_md5(t_recurso_md*);
void metadata_actualizar_md5_alter(t_recurso_md*);



#endif /* CONEXIONES_H_ */
