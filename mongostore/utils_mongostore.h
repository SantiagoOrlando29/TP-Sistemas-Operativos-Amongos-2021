//utils_mongostore.h 15072021 03.16
//valgrind --tool=memcheck --leak-check=full ./mongostore
#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <commons/log.h>
#include <commons/config.h>
#include<commons/string.h>
#include <string.h>
#include<commons/bitarray.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <dirent.h>
#include<commons/collections/list.h>
#include<netdb.h>
#include <signal.h>
#include<pthread.h>
#include<semaphore.h>
/*
//#include<commons/collections/list.h>
//#include<commons/collections/queue.h>
//#include<sys/types.h>
//#include<sys/socket.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include<math.h>
//#include<errno.h>
//#include<assert.h>
//#include <stdlib.h>
//#include<messages_lib/messages_lib.h>*/

#define ARCHIVO_LOGS "mongostore.log"
#define PROGRAMA "mongostore"
#define LOGS_EN_CONSOLA 1
#define NIVEL_DE_LOGS_MINIMO LOG_LEVEL_DEBUG
#define ARCHIVO_CONFIGURACION "mongostore.config"

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
	int size;
	char* blocks;
} t_bitacora_md;

typedef struct {
	int tid;
	char* ruta_completa;
	t_bitacora_md* metadata;
} t_bitacora_data;

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

typedef struct {
	char* nombre;
	accion_code codigo_accion;
	recurso_code codigo_recurso;
} t_tarea;

typedef enum {
	OBTENER_BITACORA = 6,
	FIN = 7,
	CARGAR_BITACORA = 12,
	REALIZAR_TAREA = 13,
	INICIAR_FSCK = 14,
	SALIR = 15,
	SABOTAJE = 16,
	FIN_HILO_TRIPULANTE = 17
} op_code;

typedef struct {
    uint32_t size;
    void* stream;
} t_buffer;

typedef struct {
	op_code mensajeOperacion;
    t_buffer* buffer;
} t_paquete;

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
char* blocks_path;
char md5_bitmap_real_inicial[33];
char md5_bitmap_actual[33];

size_t bitmap_size;
size_t blocks_size;
char* blocks_address; //Direccion en memoria de la copia principal de Blocks.ims
pthread_t thread_sincronizacion;
sem_t MUTEX_SUPERBLOQUE_BITMAP;
sem_t MUTEX_SUPERBLOQUE_PATH;
sem_t MUTEX_BLOCKS;
sem_t MUTEX_OXIGENO_MD;
sem_t MUTEX_COMIDA_MD;
sem_t MUTEX_BASURA_MD;
sem_t MUTEX_OXIGENO_IMS;
sem_t MUTEX_COMIDA_IMS;
sem_t MUTEX_BASURA_IMS;
sem_t MUTEX_BITACORA;//solo para no joder con el obtener_bitacora


//pthread_mutex_t mutex_blocks;
//char* bloques_copia;

///////////////////////FUNCIONES AGRUPADAS Y ORDENADAS "CRONOLOGICAMENTE"//////////////////////////////////////////////////////////////

void leer_config();
void blocks_abrir_hilo_sincronizacion();
int blocks_sincronizar_segun_tiempo_sincronizacion();
void semaforos_inicializar();

///////////////////////COMIENZO DE FUNCIONES NECESARIAS PARA INICIAR EL FILE SYSTEM///////////////////////////////

///////////////////////COMIENZO DE FUNCIONES NECESARIAS PARA INICIAR EL FILE SYSTEM///////////////////////////////

void file_system_iniciar();
void file_system_generar_rutas_completas();
void recursos_cargar_rutas();
int recursos_cantidad();
void file_system_verificar_existencia_previa();
void superbloque_levantar_estructura_desde_archivo();
void superbloque_asignar_memoria_a_bitmap();
void file_system_consultar_para_formatear();
void file_system_iniciar_limpio();
void file_system_eliminar_archivos_previos();
//void files_eliminar_carpeta_completa();
void superbloque_generar_estructura_con_valores_tomados_por_consola();
void superbloque_setear_bitmap_a_cero();
void superbloque_cargar_archivo();
void blocks_validar_existencia();
void blocks_validar_existencia_del_archivo();
void blocks_mapear_archivo_a_memoria();
void recursos_validar_existencia_metadatas();
void recurso_validar_existencia_metadata_en_memoria(t_recurso_data* recurso_data);
void recurso_levantar_de_archivo_a_memoria_valores_variables(t_recurso_data* recurso_data);
void metadata_setear_con_valores_default_en_memoria(t_recurso_md* recurso_md);
void files_crear_directorios_inexistentes();

void utils_eliminar_carpeta_completa_si_existe(char* path);
void bitacoras_eliminar_si_existen();

///////////////FIN DE FUNCIONES NECESARIAS PARA INICIAR EL FILE SYSTEM//////////////////////////////////////////////////////

////////////////////////////FUNCIONES NECESARIAS PARA ATENDER LA SEÑAL SIGUSR1////////////////////

void sig_handler(int signum);
void calcular_md5_bitmaps_actuales();
void notificar_sabotaje();
char* superbloque_obtener_bitmap_de_archivo();

////////////////////////////FIN DE FUNCIONES NECESARIAS PARA ATENDER LA SEÑAL SIGUSR1////////////////////

////////////////////////////FUNCIONES NECESARIAS PARA LA COMUNICACION CON DISCORDIADOR///////////////////

void iniciar_servidor();
void iniciar_servidor2();
int funcion_cliente(int socket_cliente);
int recibir_operacion(int socket_cliente);
t_list* recibir_paquete(int socket_cliente);
void* recibir_buffer(uint32_t* size, int socket_cliente);
void enviar_ok(int _socket);
void enviar_mensaje(char* mensaje, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void destruir_lista_paquete(char* contenido);

////////////////////////////FIN DE FUNCIONES NECESARIAS PARA LA COMUNICACION CON DISCORDIADOR///////////////////

////////////////////////FUNCIONES NECESARIAS PARA REALIZAR TAREAS///////////////////////

int recurso_realizar_tarea(char* nombre_tarea, char* cantidad_str);
t_tarea* tarea_buscar_accion_y_recurso(char* tarea_str);
int tarea_cantidad_disponibles();
void recurso_actualizar_archivo(t_recurso_data* recurso_data);
void superbloque_actualizar_bitmap_en_archivo();
void blocks_actualizar_archivo();
void blocks_actualizar_archivo_con_delay(int _delay);

////////////COMIENZO GENERAR

int recurso_generar_cantidad(t_recurso_data* recurso_data, int cantidad);
void recurso_asignar_caracter_de_llenado_a_archivo(t_recurso_data* recurso_data);
void metadata_generar_cantidad(t_recurso_md* recurso_md, int cantidad);
int metadata_tiene_espacio_en_ultimo_bloque(t_recurso_md* recurso_md);
int metadata_ultimo_bloque_usado(t_recurso_md* recurso_md);
void metadata_cargar_parcialmente_bloque(t_recurso_md* recurso_md, int* cantidad, int bloque);
int superbloque_obtener_bloque_libre();
void metadata_agregar_bloque_a_lista_de_blocks(t_recurso_md* recurso_md, int bloque);
void metadata_cargar_bloque_completo(t_recurso_md* recurso_md, int bloque);
void metadata_actualizar_md5(t_recurso_md* recurso_md);
char* blocks_obtener_concatenado_de_recurso(t_recurso_md* recurso_md);

////////////FIN GENERAR

/////////////COMIENZO CONSUMIR

int recurso_consumir_cantidad(t_recurso_data* recurso_data, int cantidad);
void metadata_consumir_cantidad(t_recurso_md* recurso_md, int cantidad);
void cadena_eliminar_ultimos_n_elementos_de_elementos_separados_por_comas(char* cadena, int n);
void metadata_consumir_en_ultimo_bloque(t_recurso_md* recurso_md, int cantidad);

/////////////FIN CONSUMIR

/////////////COMIENZO DESCARTAR

int recurso_descartar(t_recurso_data* recurso_data, int cantidad);
int metadata_tiene_caracteres_en_blocks(t_recurso_md* recurso_md);
void metadata_descartar_caracteres_existentes(t_recurso_md* recurso_md);
void metadata_liberar_bloques_en_bitmap_y_en_blocks(char* blocks);
void blocks_eliminar_bloque(int bloque);
void recurso_descartar_archivo(t_recurso_data* recurso_data);

////////////FIN DESCARTAR

////////////////////////FIN DE FUNCIONES NECESARIAS PARA REALIZAR TAREAS///////////////////////

////////////////////////FUNCIONES NECESARIAS PARA CARGAR BITACORAS/////////////////////////

int cargar_bitacora(char* tid_str, char* mensaje);
t_bitacora_data* bitacora_cargar_estructura_completa(int tid);
void bitacora_levantar_metadata_de_archivo(t_bitacora_data* bitacora_data);
void bitacora_cargar_metadata_default(t_bitacora_data* bitacora_data);
int bitacora_guardar_log(t_bitacora_data* bitacora_data, char* mensaje);
int bitacora_tiene_espacio_en_ultimo_bloque(t_bitacora_md* bitacora_md);
void bitacora_escribir_en_bloque(t_bitacora_md* bitacora_md, char** mensaje, int bloque);
void bitacora_actualizar_archivo(t_bitacora_data* bitacora_data);
void bitacora_borrar_estructura_completa(t_bitacora_data* bitacora_data);

//char* bitacora_obtener_mensajes_de_tripulante(char* tid_str);
char* bitacora_obtener_mensajes_de_tripulante(int tid);
char* blocks_obtener_concatenado_de_bloques_segun_tamanio(char* bloques, size_t tamanio);

////////////FIN DE FUNCIONES NECESARIAS PARA CARGAR BITACORAS//////////////////////////

/////////////////////////////////////FUNCIONES NECESARIAS PARA FSCK/////////////////////////////////////////////////////////

int fsck_iniciar();
void fsck_chequeo_de_sabotajes_en_superbloque();
void superbloque_validar_integridad_cantidad_de_bloques();
void superbloque_validar_integridad_bitmap();
char* superbloque_obtener_bitmap_correcto_segun_bloques_ocupados();
char* files_obtener_cadena_con_el_total_de_bloques_ocupados();
char* recursos_obtener_cadena_con_el_total_de_bloques_ocupados();
char* metadata_obtener_bloques_desde_archivo(char* path);
char* bitacoras_obtener_cadena_con_el_total_de_bloques_ocupados();
t_list* bitacoras_obtener_lista_con_rutas_completas();
void fsck_chequeo_de_sabotajes_en_files();
void recurso_validar_size(t_recurso_data* recurso_data);
int recurso_obtener_size_real(t_recurso_data* recurso_data);
int blocks_esta_lleno_bloque(int bloque);
int metadata_cantidad_del_caracter_en_bloque(char caracter, int bloque);
void recurso_validar_block_count(t_recurso_data* recurso_data);
int recurso_obtener_block_count_real(t_recurso_data* recurso_data);
void recurso_validar_blocks_bloque_mayor_a_fs(t_recurso_data* recurso_data);
void recurso_validar_blocks_bloque_ocupado(t_recurso_data* recurso_data);
void recurso_validar_blocks_orden_bloques(t_recurso_data* recurso_data);
void metadata_restaurar_en_blocks(t_recurso_md* recurso_md);

/////////////////////////////////////FIN DE FUNCIONES NECESARIAS PARA FSCK/////////////////////////////////////////////////////////

////////////////////////////FUNCIONES UTILS (SOBRETODO PARA MANEJO DE ARCHIVOS)/////////////////////////

void utils_crear_directorio_si_no_existe(char* path);
int utils_existe_en_disco(char* path);
int utils_abrir_archivo_para_lectura_escritura(char* path);
void utils_crear_archivo(char* path);
void utils_dar_tamanio_a_archivo(char* path, size_t length);
void utils_esperar_a_usuario();

////////////////////////////FIN DE FUNCIONES UTILS (SOBRETODO PARA MANEJO DE ARCHIVOS)/////////////////////////

////////////////////////////FUNCIONES ADICIONALES PARA MANEJO DE CADENAS DE TEXTO/////////////////////////

int cadena_cantidad_elementos_en_lista(char* cadena);
void cadena_eliminar_array_de_cadenas(char*** puntero_a_array_de_punteros, int cantidad_cadenas);
int cadena_ultimo_entero_en_lista_de_enteros(char* lista_de_enteros);
void cadena_agregar_entero_a_lista_de_enteros(char** lista_de_enteros, int entero);
void cadena_sacar_ultimo_caracter(char* cadena);
void cadena_calcular_md5(const char *cadena, int length, char* md5_33str);
char* cadena_obtener_n_ultimos_enteros(char* cadena, int n);
void cadena_agregar_a_lista_existente_valores_de_otra_lista(char** lista_original, char* lista_a_adicionar);

////////////////////////////FUNCIONES ADICIONALES PARA MANEJO DE CADENAS DE TEXTO/////////////////////////



/////////////////////////FUNCIONES A BORRAR (O NO)/////////////////////

void enviar_header(op_code tipo, int socket_cliente);
void verificar_superbloque_temporal();
void prueba_de_tareas_random();
void prueba_de_generar_recursos();
void prueba_de_consumir_recursos();
void prueba_de_descartar_basura();
void prueba_de_cargar_n_bitacoras_con_mensaje(int n, char* mensaje);
void prueba_de_cargar_bitacora_de_tripulante_n_con_m_mensajes(int n, int m, char* mensaje_de_prueba);


#endif /* CONEXIONES_H_ */
