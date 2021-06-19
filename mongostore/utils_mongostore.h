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
#include<pthread.h>
//#include<messages_lib/messages_lib.h>
#include<stdbool.h>

#define IP "127.0.0.1"
#define PUERTO "5002"


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
}tipoMensaje;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	tipoMensaje mensajeOperacion;
	t_buffer* buffer;
}t_paquete;

typedef struct{
	char* ip_mongostore; // IP
	char* puerto_mongostore; // puerto en el que se iniciara el servidor
	char* punto_monstaje; // direccion donde se montara los archivos
	int tiempo_sincronizacion; //Tiempo entre bajadas de memoria a disco
	char** posiciones_sabotaje; // Lista de posiciones de los sabotajes

}config_struct;

typedef struct{
	uint32_t size;
	uint32_t block_count;
	char* blocks; // [1,2,3]
	char caracter_llenado;
	char* md5_archivo;
}config_metadata;

typedef enum{
	LIBRE,
	OCUPADO
}estadoBloque;

typedef enum{
	ARCHIVO,
	DIRECTORIO
}tipoMetadata;

typedef struct{
	uint32_t block_size; //
	uint32_t blocks;	//
	char* bitmap; //lista de estados de bloques
}superBloque;

typedef struct{
	uint32_t size;
	t_list* blocks; // [bloques utilizados]
}bitacora;
typedef enum
{
	GENERAR_OXIGENO,
	CONSUMIR_OXIGENO,
	GENERAR_COMIDA,
	CONSUMIR_COMIDA,
	GENERAR_BASURA,
	DESCARTAR_BASURA,
}tarea_tripulante;

typedef struct{
	tarea_tripulante tarea;
	int parametro;
	int pos_x;
	int pos_y;
	int tiempo;
}tarea;

struct nodoArbolDirectorio{

};

t_log* logger;

void* recibir_buffer(int*, int);
int iniciar_servidor(char* , char*);
int iniciar_servidor(char*, char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void leer_config();


/*Operaciones para enviar mensajes desde mongostore a discordiador*/
t_paquete* crear_paquete(tipoMensaje tipo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
/*FINALIZACION*/

//tareas
tarea* crearTarea(tarea_tripulante tipo,int parametro,int pos_x,int pos_y,int tiempo);

char* buscarPath(char* path,char* puntoMontaje);
int archivoExiste(char* path);
void crearDireccion(char* direccion);
void eliminarDirectorio(char* path);

// uint32_t size,uint32_t block_count,int* blocks,char caracter_llenado,char* md5,
void crearRecursoMetadata(uint32_t size,uint32_t block_count,char* blocks,char caracter_llenado,char* md5,char* path);
//void crearRecursoMetadata(char* path;
void iniciarFS();
void iniciarBlocks(int);
void iniciarSuperBloque(superBloque );

#endif /* CONEXIONES_H_ */
