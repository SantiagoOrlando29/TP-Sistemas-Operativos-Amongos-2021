/*
 * conexiones.h
 *
 *  Created on: 2 mar. 2019
 *      Author: utnso
 */

#ifndef UTILS_DISCORDIADOR_H_
#define UTILS_DISCORDIADOR_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include <commons/config.h>
#include<string.h>

/**
 * En este enum se agregan todos los distintos tipos de mensajes que van a enviar entre los clientes y servidores
 */

typedef enum
{
	// Se prueba con dos mensajes
	//
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

typedef struct {
	uint32_t id;
	uint32_t posicionX;
	uint32_t posicionY;
	uint32_t numeroPatota;

} nuevoTripulante;

const static struct {
	uint32_t valor;
	const char *string;
}conversionProceso [] = {
		{INICIAR_PATOTA, "INICIAR_PATOTA"},
		{LISTAR_TRIPULANTES, "LISTAR_TRIPULANTES"},
		{FIN, "FIN"}
};

typedef struct{
	char* ip_miram;
	char* puerto_miram;
	char* ip_mongostore;
	char* puerto_mongostore;
	int grado_multitarea;
	char* algoritmo;
	int quantum;
	int duracion_sabotaje;
	int retardo_cpu;
}config_struct;

void leer_config();
int crear_conexion(char* ip, char* puerto);
t_paquete* crear_paquete(tipoMensaje tipo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void armar_paquete(int conexMiRam, int conexMongoStore);
size_t tamanioTripulante (nuevoTripulante* tripulante);
nuevoTripulante* crearNuevoTripulante(uint32_t id ,uint32_t posicionX, uint32_t posicionY, uint32_t numeroPatota);

// recibir

int iniciar_servidor(char*, char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void leer_config();


#endif /* UTILS_DISCORDIADOR_H_ */
