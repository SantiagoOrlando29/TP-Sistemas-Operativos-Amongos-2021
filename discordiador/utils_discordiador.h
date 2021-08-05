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

#define tamanio_PCB  8
#define tamanio_TCB  21

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
	PEDIR_TAREA,
	CAMBIAR_DE_ESTADO,
	INFORMAR_MOVIMIENTO,
	NO_HAY_NADA_PARA_LISTAR,
	INFORMAR_BITACORA,
	TAREA_MONGO,
	FSCK,
	SALIR_MONGO,
	FIN_HILO_TRIPULANTE=17
}tipoMensaje;

typedef enum
{
	INICIO_TAREA,
	FIN_TAREA,
	CORRER_A_SABOTAJE,
	RESOLVER_SABOTAJE
}mensaje_bitacora;

typedef enum
{
	OXIGENO,
	COMIDA,
	BASURA
} recurso_code;

typedef enum
{
	GENERAR,
	CONSUMIR,
	DESCARTAR
} accion_code;

typedef struct
{
	uint32_t size;//tendria q ser uint32
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
	char* tarea;
	uint32_t parametro;
	int pos_x;
	int pos_y;
	int tiempo;
}tarea;

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
	int socket_miram;
	int socket_mongo;
	tarea* tarea_posta;
	bool fui_expulsado;
	int cant_tripus_patota;
}tcbTripulante;

// fin estructuras tripulantes


/*FINALIZACION DE ESCTRUCTURAS PARA DISCORDIADOR*/

void leer_config();
int crear_conexion(char* ip, char* puerto);
t_paquete* crear_paquete(tipoMensaje tipo);
//void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_a_paquete(t_paquete* paquete, void* valor, uint32_t tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
char* recibir_mensaje(int socket_cliente);

void menu_discordiador();
void funcion_sabotaje();

tcbTripulante* crear_tripulante(uint32_t tid, char estado, uint32_t posicionX, uint32_t posicionY, uint32_t puntero_pcb, int cantidad_tripulantes);
//pcbPatota* crear_patota(uint32_t , uint32_t);
int codigoOperacion (const char*);
void enviar_header(tipoMensaje tipo, int socket_cliente);

void remover_tripulante_de_lista(tcbTripulante* tripulante, t_list* lista);

//TAREAS
//char* imprimirTarea(tarea*);
tarea_tripulante codigoTarea(char*);
//void leer_tareas(char* archTarea, char* *tareas);
//tarea* crear_tarea(tarea_tripulante,int,int,int,int);
//tarea* crear_tarea(char*,int,int,int,int);
tcbTripulante* hacer_tarea(tcbTripulante*,tarea*);
//void ejecutar_tarea(tarea_tripulante,int);
tarea* pedir_tarea(int conexion_miram, tcbTripulante* tripulante);

char* leer_tareas_archivo(char* archTarea);
tarea* transformar_char_tarea(char* char_tarea);

void termina_quantum(int* quantums_ejecutados, tcbTripulante* tripulante);

void cambiar_estado(int conexion_miram, tcbTripulante* tripulante, char nuevo_estado);
void informar_movimiento(int conexion_miram, tcbTripulante* tripulante);
void informar_inicio_tarea(tcbTripulante* tripulante);
void informar_fin_tarea(tcbTripulante* tripulante);
void informar_movimiento_mongo_X (tcbTripulante* tripulante, int x_viejo);
void informar_movimiento_mongo_Y (tcbTripulante* tripulante, int y_viejo);
void informar_atencion_sabotaje(tcbTripulante* tripulante, char* posicion_char);
void informar_sabotaje_resuelto(tcbTripulante* tripulante);
void mongo_tarea(tcbTripulante* tripu);

void planificacion_pausada_o_no();
void planificacion_pausada_o_no_exec(tcbTripulante* tripulante, int* quantums_ejecutados);

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
//t_list* recibir_lista_tripulantes(int , int, t_log*);

/*
 * Pre: Recibo un logger
 * Post: Muestro por pantalla el error y aparte escribo en el logger el inconveniente
 * */

void mensajeError (t_log* logger);

void limpiar_array(char** array);

void liberar_memoria_tripu(tcbTripulante* tripu);
/*FINALIZACION*/


#endif /* UTILS_DISCORDIADOR_H_ */
