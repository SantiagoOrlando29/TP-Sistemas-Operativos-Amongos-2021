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

/*Estructuras necesarias para discordiador*/

/**
 * En este enum se agregan todos los distintos tipos de mensajes que van a enviar entre los clientes y servidores
 */

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
		{PRUEBA, "PRUEBA"},
		{INICIAR_PATOTA, "INICIAR_PATOTA"},
		{LISTAR_TRIPULANTES, "LISTAR_TRIPULANTES"},
		{EXPULSAR_TRIPULANTE, "EXPULSAR_TRIPULANTE"},
		{INICIAR_PLANIFICACION, "INICIAR_PLANIFICACION"},
		{PAUSAR_PLANIFICACION, "PAUSAR_PLANIFICACION"},
		{OBTENER_BITACORA, "OBTENER_BITACORA"},
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


/*FINALIZACION DE ESCTRUCTURAS PARA DISCORDIADOR*/

void leer_config();
int crear_conexion(char* ip, char* puerto);
t_paquete* crear_paquete(tipoMensaje tipo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
int menu_discordiador(int , int, t_log* );
nuevoTripulante* crearNuevoTripulante(uint32_t id ,uint32_t posicionX, uint32_t posicionY, uint32_t numeroPatota);
int codigoOperacion (const char*);
void enviar_header(tipoMensaje tipo, int socket_cliente);


/*Calcular el tamaño de las diferentes estructuras o paquetes a enviar*/
size_t tamanioTripulante (nuevoTripulante* tripulante);
/*FINALIZACION*/


/*Operaciones para recibir un mensaje desde lo servidores usando el mismo socket*/

t_list* recibir_paquete(int);
int recibir_operacion(int);
void leer_config();
/*FINALIZACION*/

/*Operaciones para Mostrar en Discordiador*/

/*
 * Pre: Recibo el tipo de mensaje y la conexion de donde lo recibo
 * Post: Muestro por pantalla la informacion dada de la lista de los tripulantes
 * */
void recibir_lista_tripulantes(int , int, t_log*);

/*
 * Pre: Recibo un logger
 * Post: Muestro por pantalla el error y aparte escribo en el logger el inconveniente
 * */

void mensajeError (t_log* logger);
/*FINALIZACION*/


#endif /* UTILS_DISCORDIADOR_H_ */
