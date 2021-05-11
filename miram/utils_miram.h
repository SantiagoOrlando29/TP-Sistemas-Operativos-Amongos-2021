

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
#include<string.h>



typedef enum
{
	INICIAR_PATOTA,
	LISTAR_TRIPULANTES,
	EXPULSAR_TRIPULANTE,
	INICIAR_PLANIFICACION,
	PAUSAR_PLANIFICACION,
	OBTENER_BITACORA,
} tipoMensaje;

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


typedef struct {
	uint32_t id;
	uint32_t posicionX;
	uint32_t posicionY;
	uint32_t numeroPatota;

} nuevoTripulante;

t_log* logger;


typedef struct{
	char* ip_miram;
	char* puerto_miram;
	char* ip_mongostore;
	char* puerto_mongostore;
	//int grado_multitarea;
	//char* algoritmo;
	//int quantum;
	//int duracion_sabotaje;
	//int retardo_cpu;
}config_struct;

void* recibir_buffer(int*, int);
int iniciar_servidor(char*, char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void leer_config();

/*Operaciones para enviar mensajes desde miram a discordiador*/
t_paquete* crear_paquete(tipoMensaje tipo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
/*FINALIZACION*/


/*Calcular el tamaño de las diferentes estructuras o paquetes a enviar*/
size_t tamanioTripulante (nuevoTripulante* tripulante);
/*FINALIZACION*/

/*crear la estructura de un nuevo tripulante*/
nuevoTripulante* crearNuevoTripulante(uint32_t ,uint32_t , uint32_t , uint32_t);


#endif /* CONEXIONES_H_ */

