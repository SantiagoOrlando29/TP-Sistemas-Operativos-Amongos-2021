#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include <commons/config.h>

#include <fcntl.h>
#include<commons/bitarray.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<math.h>
#include<errno.h>
#include<string.h>
#include<assert.h>
#include<sys/mman.h>
#include<commons/collections/queue.h>
#include<commons/string.h>
#include<pthread.h>
//#include<messages_lib/messages_lib.h>
#include<stdbool.h>


//Los siguientes defines estan comentados ya que los valores se toman del config
//#define IP "127.0.0.1"
//#define PUERTO "5002"

//#define pathFiles "/home/utnso/polus/Files/"
//#define pathBlocks "/home/utnso/polus/"

//char* puntoMontaje

typedef enum
{
	PRUEBA,
	INICIAR_PATOTA,
	LISTAR_TRIPULANTES,
	EXPULSAR_TRIPULANTE,
	INICIAR_PLANIFICACION,
	PAUSAR_PLANIFICACION,
	OBTENER_BITACORA,
	FIN
} mensaje_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	mensaje_code mensajeOperacion;
	t_buffer* buffer;
} t_paquete;

typedef struct{
	char* ip; // IP
	char* puerto; // puerto en el que se iniciara el servidor
	char* punto_montaje; // direccion donde se montara los archivos
	int tiempo_sincronizacion; //Tiempo entre bajadas de memoria a disco
	char** posiciones_sabotaje; // Lista de posiciones de los sabotajes

} config_struct;

typedef struct{
	uint32_t size;
	uint32_t block_count;
	t_list* blocks; // [1,2,3]
	char caracter_llenado;
	char* md5_archivo;
} config_metadata;

typedef enum{
	LIBRE,
	OCUPADO
} bloque_state;

typedef enum{
	ARCHIVO,
	DIRECTORIO
} tipoMetadata;

typedef struct{
	int parte_entera;
	int bytes_usados;
} bloque_usado;

typedef struct{
	uint32_t size;
	uint32_t block_count;
	char* blocks; // [1,2,3]
	char caracter_llenado;
} bloques_recurso; //al momento de generar recurso

typedef struct{
	uint32_t block_size; //
	uint32_t blocks;	//
	char* bitmap; //lista de estados de bloques
} t_superbloque;

typedef struct{
	uint32_t size;
	t_list* blocks; // [bloques utilizados]
} bitacora;

typedef enum
{
	GENERAR_OXIGENO,
	CONSUMIR_OXIGENO,
	GENERAR_COMIDA,
	CONSUMIR_COMIDA,
	GENERAR_BASURA,
	DESCARTAR_BASURA,
} tarea_tripulante_code;

typedef struct{
	tarea_tripulante_code tarea;
	int parametro;
	int pos_x;
	int pos_y;
	int tiempo;
} tarea;

t_log* logger;
config_struct ims_config;

void* recibir_buffer(int*, int);
//int iniciar_servidor(char* , char*);
//int iniciar_servidor(char*, char*);
void iniciar_servidor(config_struct*);
int funcion_cliente(int);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);



/*Operaciones para enviar mensajes desde mongostore a discordiador*/
t_paquete* crear_paquete(mensaje_code tipo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
/*FINALIZACION*/

//tareas
tarea* crear_tarea(tarea_tripulante_code tipo,int parametro,int pos_x,int pos_y,int tiempo);

char* buscar_path(char* path,char* puntoMontaje);
int archivo_existe(char* path);
void crear_direccion(char* direccion);
void eliminar_directorio(char* path);


void escribir_archivo_superbloque(char*);
void crear_superbloque();
void generar_oxigeno(int ,char*);
void generar_comida(int,char*);
void generar_basura(int,char*);
bloques_recurso* recuperar_valores(char*);
bloques_recurso* asignar_bloques(int,t_superbloque*,char,char*);
void crear_recurso_metadata(bloques_recurso*,char*,char*);
void iniciar_blocks(int);



#endif /* CONEXIONES_H_ */
