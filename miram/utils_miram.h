

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
#include<pthread.h>
#include<stdbool.h>
#include <semaphore.h>
#include <signal.h>

#define tamanio_PCB  8
#define tamanio_tarea 10
#define tamanio_TCB  21

sem_t MUTEX_PEDIR_TAREA;
sem_t MUTEX_CAMBIAR_ESTADO;
sem_t MUTEX_CAMBIAR_POSICION;
sem_t MUTEX_TABLA_MEMORIA;
sem_t MUTEX_LISTA_TABLAS_SEGMENTOS;

typedef struct{
	int patota_id;
	int cant_tareas;
	int cant_tripulantes;
}patota_aux;

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
	INFORMAR_BITACORA, //PARA PROBAR LO DE MONGO
	INFORMAR_BITACORA_MOVIMIENTO //PARA PROBAR LO DE MONGO
}tipoMensaje;

typedef enum //PARA PROBAR LO DE MONGO
{
	INICIO_TAREA,
	FIN_TAREA,
	CORRER_A_SABOTAJE,
	RESOLVER_SABOTAJE
}mensaje_bitacora;

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
}tcbTripulante;

typedef struct {
	uint32_t id;
	uint32_t posicionX;
	uint32_t posicionY;
	uint32_t numeroPatota;

} nuevoTripulante;


typedef struct{
	char* ip_miram;
	char* puerto_miram;
	char* tamanio_memoria;
	char* squema_memoria;
	char* tamanio_pag;
	char* tamanio_swap;
	char* path_swap;
	char* algoritmo_reemplazo;
	void* posicion_inicial;
	int cant_marcos;
	uint8_t **marcos;
	char* criterio_seleccion;
}config_struct;

t_config * archConfig;
config_struct configuracion;
t_log* logger;

typedef enum{
	MEM_PRINCIPAL,
	MEM_SECUNDARIA,
}presencia;

typedef struct{
	int id_patota;
	presencia ubicacion;
	t_list* marco_inicial;

}tabla_paginacion;

typedef struct{
	int id_marco;
	int control_lru;
	int clock;
	int libre;
}marco;


typedef struct{
	int numero_segmento;
	int base;
	int tamanio;
}segmento;

typedef struct{
	int id_patota;
	int primer_tripulante;
	int ultimo_tripulante;
	t_list* lista_segmentos;//segmento_inicial
}tabla_segmentacion;

typedef struct{
    int base;
    int tam;
    bool libre;
    void* contenido;
}espacio_de_memoria;

t_list* tabla_espacios_de_memoria;

t_list* lista_tablas_segmentos;

void* recibir_buffer(int*, int);
void iniciar_servidor(config_struct*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void leer_config();
tarea* crear_tarea(tarea_tripulante,int,int,int,int);
void enviar_mensaje(char* mensaje, int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int bytes);


//Funciones memoria
void iniciar_miram(config_struct* config);
void agregar_memoria_aux(t_list* tabla_aux, config_struct* config);
void imprimir_memoria(t_list* tabla_aux);
void imprimir_seg(t_list* tabla_aux);
int posicion_marco(config_struct*);
void imprimir_ocupacion_marcos(config_struct configuracion);
int posicion_patota(int id_buscado,t_list* tabla_aux);
void finalizar_miram(config_struct* config_servidor);
int marco_tarea(int posicion_patota, t_list* tabla_aux, int nro_marco);
void agregar_tripulante_marco(tcbTripulante* tripulante, int id_patota, t_list* tabla_aux, config_struct* configuracion);
int cuantos_marcos(int cuantos_tripulantes, int longitud_tarea,config_struct* config_servidor);

void escribir_tripulante(tcbTripulante* tripulante, void* posicion_inicial);

tcbTripulante* obtener_tripulante(void* inicio_tripulantes);

/*Operaciones para enviar mensajes desde miram a discordiador*/
t_paquete* crear_paquete(tipoMensaje tipo);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void enviar_header(tipoMensaje , int );
int funcion_cliente(int);

//SEGMENTACION
espacio_de_memoria* crear_espacio_de_memoria(int, int, bool);
void imprimir_tabla_espacios_de_memoria();
void eliminar_espacio_de_memoria(int);
espacio_de_memoria* buscar_espacio_de_memoria_libre(int);
espacio_de_memoria* busqueda_first_fit(int);
espacio_de_memoria* busqueda_best_fit(int);
espacio_de_memoria* asignar_espacio_de_memoria(size_t);
void imprimir_tabla_segmentos_patota(tabla_segmentacion*);
void eliminar_segmento(int, tabla_segmentacion*);
void ordenar_memoria();
void unir_espacios_contiguos_libres();
bool patota_segmentacion(int, uint32_t, char*, t_list*);
bool funcion_expulsar_tripulante(int tripulante_id);
void compactar_memoria();
bool enviar_tarea_segmentacion(int socket_cliente, int numero_patota, int id_tripulante);
espacio_de_memoria* buscar_espacio(segmento* segmento);
char* buscar_tarea(espacio_de_memoria* espacio, int prox_instruccion);
bool cambiar_estado(int numero_patota, char nuevo_estado, int tid);
tabla_segmentacion* buscar_tabla_segmentos(int numero_patota);
bool cambiar_posicion(int tid, int posx, int posy, int pid);

pcbPatota* crear_pcb(uint32_t numero_patota);


/*FINALIZACION*/

//inicializar PCB desde lo mandado
tcbTripulante* crear_tripulante(uint32_t, char, uint32_t, uint32_t, uint32_t, uint32_t);
pcbPatota* crear_patota(uint32_t , uint32_t);
//solo para comprobar que se formaron bien
void mostrar_tripulante(tcbTripulante* tripulante, pcbPatota* patota);

/*Calcular el tamaño de las diferentes estructuras o paquetes a enviar*/
size_t tamanio_tcb(tcbTripulante*);
size_t tamanio_pcb(pcbPatota*);
/*FINALIZACION*/

/*crear la estructura de un nuevo tripulante*/
tcbTripulante* crear_tripulante(uint32_t, char, uint32_t, uint32_t, uint32_t, uint32_t);


char* string_nombre_tarea(int);
char* tarea_a_string(tarea* t);

void limpiar_array(char** array);

void destruir_lista_paquete(char* contenido);
void destruir_segmentos(segmento* seg);
void destruir_espacio_memoria(espacio_de_memoria* espacio);
void destruir_tabla_segmentacion(tabla_segmentacion* tabla);

void sig_handler(int signum);
void dump_memoria();

#endif /* CONEXIONES_H_ */

