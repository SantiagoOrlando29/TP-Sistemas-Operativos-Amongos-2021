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
#include <dirent.h>
#include<commons/collections/list.h>
#include<netdb.h>
#include <signal.h>
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

typedef struct {
	int num_tripulante;
	char* ruta_completa;
	t_bitacora_md* metadata;
} t_bitacora_data;

typedef struct {
	int size;
	char* blocks;
} t_bitacora_md;

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

typedef enum {
	CARGAR_BITACORA = 12,
	REALIZAR_TAREA = 13,
	INICIAR_FSCK = 14,
	SALIR = 15,
	SABOTAJE = 16

} op_code;

typedef struct
{
    uint32_t size;
    void* stream;
} t_buffer;

typedef struct
{
	op_code mensajeOperacion;
    t_buffer* buffer;
}t_paquete;

enum {
	ERROR = -1,
	OK
};

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
char* bitmap_address;
size_t bitmap_size;

char* blocks_path;
char* blocks_address; //Direccion en memoria de la copia principal de Blocks.ims
size_t blocks_size;

//pthread_mutex_t mutex_blocks;
//char* bloques_copia;

//Prototipos  21 julio
void leer_config();
void file_system_iniciar();
void file_system_generar_rutas_completas();
void files_cargar_rutas_de_recursos();
void file_system_crear_directorios_inexistentes();
void crear_directorio_si_no_existe(char* path);
int existe_en_disco(char* path);
void superbloque_validar_existencia();
void verificar_superbloque_temporal();//borrar
void superbloque_generar_estructura_desde_archivo();
int abrir_archivo_para_lectura_escritura(char* path);
void superbloque_asignar_memoria_a_bitmap();
void superbloque_setear_tamanio();
void superbloque_mapear_archivo_a_memoria();
void file_system_eliminar_archivos_previos();
void files_eliminar_carpeta_completa();
void crear_archivo(char* path);
void superbloque_generar_estructura_con_valores_tomados_por_consola();
void superbloque_setear_bitmap_a_cero();
void dar_tamanio_a_archivo(char* path, size_t length);
void superbloque_cargar_mapeo_desde_estructura();
void superbloque_actualizar_archivo();
void blocks_validar_existencia();
void blocks_validar_existencia_del_archivo();
void blocks_mapear_archivo_a_memoria();

int cadena_cantidad_elementos_en_lista(char*);
void cadena_sacar_ultimo_caracter(char*);
void cadena_eliminar_array_de_cadenas(char***, int);
void cadena_calcular_md5(const char *cadena, int, char*);
char* cadena_calcular_md5_alter(const char *cadena, int );
int files_cantidad_recursos();

int superbloque_obtener_bloque_libre();
void blocks_actualizar_archivo();
int blocks_obtener_concatenado_de_recurso(t_recurso_md* recurso_md, char** concatenado);
void recurso_generar_cantidad(recurso_code, int);
void recurso_validar_existencia_metadata_en_memoria(t_recurso_data*);
void recurso_actualizar_archivo(t_recurso_data*);
void recurso_descartar_cantidad(recurso_code, int);
void metadata_generar_cantidad(t_recurso_md*, int);
void metadata_agregar_bloque_a_lista_de_blocks(t_recurso_md*, int);
int metadata_tiene_espacio_en_ultimo_bloque(t_recurso_md*);
void metadata_cargar_en_bloque(t_recurso_md*, int*, int);
void metadata_cargar_en_bloque2(t_recurso_md*, int*, int);
int metadata_ultimo_bloque_usado(t_recurso_md*);
void metadata_levantar_de_archivo_a_memoria_valores_variables(t_recurso_md*, char*);
void metadata_setear_con_valores_default_en_memoria(t_recurso_md*);
void metadata_actualizar_md5(t_recurso_md*);

//Recientemente agregadas
int recurso_es_valido(recurso_code codigo_recurso);
int files_cantidad_recursos();
void file_system_eliminar_archivos_previos();
void blocks_eliminar_archivo();
void superbloque_actualizar_bitmap_en_archivo();
void superbloque_actualizar_blocks_en_archivo();
void superbloque_liberar_bloques_en_bitmap(char* blocks);
void iniciar_servidor(t_configuracion*);
void funcion_cliente(int);
t_list* recibir_paquete(int);
void* recibir_buffer(uint32_t*, int);
void destruir_lista_paquete(char*);
void enviar_mensaje(char*, int);
void eliminar_paquete(t_paquete*);
void sig_handler(int);
void notificar_sabotaje();
int recibir_operacion(int);
void enviar_header(op_code, int);
void iniciar_servidor2(t_configuracion*);
int funcionx(int);
char* contar_archivos(char*);
void destruir_lista(char*);
void bitacora_crear_archivo(int);


//Prototipos de funciones sujetas a cambios al linkearse con discordiador
void recurso_realizar_tarea();
void tomar_accion_recurso_y_cantidad(int* accion, int* recurso, int* cantidad);

//Funciones en desarrollo
void fsck_iniciar();
void fsck_chequeo_de_sabotajes_en_superbloque();
void superbloque_validar_integridad_cantidad_de_bloques();
void superbloque_validar_integridad_bitmap();
char* files_obtener_cadena_con_el_total_de_bloques_ocupados();
void superbloque_setear_bloques_en_bitmap(char* bloques_ocupados);
void fsck_chequeo_de_sabotajes_en_files();
void recurso_validar_size(t_recurso_data* recurso_data);
int recurso_obtener_size_real(t_recurso_data* recurso_data);
void recurso_validar_block_count(t_recurso_data* recurso_data);
int recurso_obtener_block_count_real(t_recurso_data* recurso_data);
void recurso_validar_blocks(t_recurso_data* recurso_data);
char* recurso_obtener_blocks_real(t_recurso_data* recurso_data);


char* files_obtener_cadena_con_bloques_ocupados_por_recursos();
char* files_obtener_bloques_de_archivo_metadata(char* path);
char* files_obtener_cadena_con_bloques_ocupados_por_bitacoras();
int files_cantidad_bitacoras();

//YA NO SE USAN
void superbloque_generar_estructura();

void superbloque_generar_estructura_con_valores_ingresados_por_consola();
void superbloque_tomar_valores_desde_consola();
void superbloque_validar_existencia_del_archivo();
void superbloque_cargar_archivo();
FILE* abrir_archivo_para_escritura(char*);


#endif /* CONEXIONES_H_ */
