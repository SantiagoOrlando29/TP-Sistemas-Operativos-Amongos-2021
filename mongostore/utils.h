

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

#define IP "127.0.0.1"
#define PUERTO "5002"

typedef enum
{
	// Se prueba con dos mensajes
	MensajeUno,
	MensajeDos,
	//
	INICIAR_PATOTA,
	LISTAR_TRIPULANTES,
	EXPULSAR_TRIPULANTE,
	INICIAR_PLANIFICACION,
	PAUSAR_PLANIFICACION,
	OBTENER_BITACORA,
}tipoMensaje;

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

t_log* logger;

void* recibir_buffer(int*, int);
int iniciar_servidor(char* , char*);
int iniciar_servidor(char*, char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void leer_config();

#endif /* CONEXIONES_H_ */

