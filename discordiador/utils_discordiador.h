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
#include<pthread.h>
#include <semaphore.h>
#include<stdbool.h>

/*Estructuras necesarias para discordiador*/

/**
 * En este enum se agregan todos los distintos tipos de mensajes que van a enviar entre los clientes y servidores
 */

t_log* logger;

typedef enum
{
	PRUEBA,
	INICIAR_PATOTA,
	LISTAR_TRIPULANTES,
	EXPULSAR_TRIPULANTE,
	INICIAR_PLANIFICACION,
	PAUSAR_PLANIFICACION,
	OBTENER_BITACORA,
	FIN,
	PEDIR_TAREA
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
		{FIN, "FIN"},
		{PEDIR_TAREA, "PEDIR_TAREA"}
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
}config_discordiador;

//Estructuras de tripulantes inicializados
/*
typedef enum{
	N,  // NEW
	R,	// READY
	E,	// EXEC
	B	// BREAK interrumpido
}estado;
*/
typedef struct{
	uint32_t pid;  // ID PATOTA
	uint32_t tareas; // DIR. LOGICA INCIO DE TAREAS
}pcbPatota;

typedef struct{
	uint32_t tid;
	char estado;
	uint32_t posicionX;
	uint32_t posicionY;
	uint32_t prox_instruccion; // Identificador de la próxima instrucción a ejecutar
	uint32_t puntero_pcb; //Dirección lógica del PCB del tripulante
	sem_t semaforo_tripulante;
}tcbTripulante;

// fin estructuras tripulantes

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


/*FINALIZACION DE ESCTRUCTURAS PARA DISCORDIADOR*/

void leer_config();
int crear_conexion(char* ip, char* puerto);
t_paquete* crear_paquete(tipoMensaje tipo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
int menu_discordiador(int , int, t_log* );
tcbTripulante* crear_tripulante(uint32_t, char, uint32_t, uint32_t, uint32_t, uint32_t);
pcbPatota* crear_patota(uint32_t , uint32_t);
int codigoOperacion (const char*);
void enviar_header(tipoMensaje tipo, int socket_cliente);
char* imprimirTarea(tarea*);
tarea_tripulante codigoTarea(char*);
void leer_tareas(char* archTarea, char* *tareas);
tarea* crear_tarea(tarea_tripulante,int,int,int,int);
tcbTripulante* hacer_tarea(tcbTripulante*,tarea*);
void ejecutar_tarea(tarea_tripulante,int);

/*Calcular el tamaño de las diferentes estructuras o paquetes a enviar*/
size_t tamanio_tcb(tcbTripulante*);
size_t tamanio_pcb(pcbPatota*);
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
t_list* recibir_lista_tripulantes(int , int, t_log*);

/*
 * Pre: Recibo un logger
 * Post: Muestro por pantalla el error y aparte escribo en el logger el inconveniente
 * */

void mensajeError (t_log* logger);
/*FINALIZACION*/


#endif /* UTILS_DISCORDIADOR_H_ */
