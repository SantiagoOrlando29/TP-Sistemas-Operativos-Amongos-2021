

#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include <commons/string.h>
#include <commons/memory.h>
#include <commons/config.h>
#include<string.h>
#include<pthread.h>
#include<stdbool.h>
#include <semaphore.h>
#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>




#define tamanio_PCB  8
#define tamanio_tarea 10
#define tamanio_TCB  21

#define ASSERT_CREATE(nivel, id, err)                                                   \
    if(err) {                                                                           \
        nivel_destruir(nivel);                                                          \
        nivel_gui_terminar();                                                           \
        fprintf(stderr, "Error al crear '%c': %s\n", id, nivel_gui_string_error(err));  \
        return EXIT_FAILURE;                                                            \
    }

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
	uint32_t puntero_pcb;
	sem_t semaforo_tripulante;//Dirección lógica del PCB del tripulante
}tcbTripulante;

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
	int tamanio_memoria;
	char* squema_memoria;
	int tamanio_pag;
	int tamanio_swap;
	char* path_swap;
	char* algoritmo_reemplazo;
	void* posicion_inicial;
	int cant_marcos;
	int **marcos;
	char* criterio_seleccion;
	t_list* marcos_libres;
	int cant_lugares_swap;
	t_list* swap_libre;

}config_struct;


typedef enum{
	MEM_PRINCIPAL,
	MEM_SECUNDARIA,
}presencia;

typedef struct{
	int id_patota;
	t_list* marco_inicial;
	int cant_tripulantes;
	int long_tareas;

}tabla_paginacion;

typedef struct{
	int id_marco;
	time_t ultimo_uso;
	presencia ubicacion;
	int bit_uso;
	int libre;
}marco;


typedef struct{
	int base;
	int tamanio;
}segmento;

typedef struct{
	int id_patota;
	t_list* segmento_inicial;

}tabla_segmentacion;

typedef struct{
    int base;
    int tam;
    bool libre;
}espacio_de_memoria;

t_list* tabla_espacios_de_memoria;

void* recibir_buffer(int*, int);
void iniciar_servidor(config_struct*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void leer_config();
tarea* crear_tarea(tarea_tripulante,int,int,int,int);


//Funciones memoria
void iniciar_miram(config_struct* config);
void agregar_memoria_aux(t_list* tabla_aux, config_struct* config);
void imprimir_memoria(t_list* tabla_aux);
void imprimir_seg(t_list* tabla_aux);
void agregar_segmentos(t_list* lista_aux_seg);
int posicion_marco(config_struct*);
marco* siguiente_marco(int id_patota, int id_marco,tabla_paginacion* tabla_aux);
void* leer_atributo(int offset, int nro_marco, config_struct* config_s);
void* leer_atributo_char(int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_char(tcbTripulante* tripulante, int offset, int nro_marco, config_struct* config_s);
int alcanza_espacio(int* offset,int tamanio_marco, int tipo_dato);
marco* incrementar_marco(int* indice,int* offset, int tamanio_marco, int tipo_dato, tabla_paginacion* auxiliar, config_struct* config_servidor);
void imprimir_ocupacion_marcos(config_struct* configuracion);
int posicion_patota(int id_buscado,t_list* tabla_aux);
void finalizar_miram(config_struct* config_servidor);
int marco_tarea(int posicion_patota, t_list* tabla_aux, int nro_marco);
void agregar_tripulante_marco(tcbTripulante* tripulante, int id_patota, t_list* tabla_aux, config_struct* configuracion);
int cuantos_marcos(int cuantos_tripulantes, int longitud_tarea,config_struct* config_servidor);
void mostrar_tripulante(tcbTripulante* tripulante,pcbPatota* patota);
int cuantos_marcos_libres(config_struct* config_servidor);
void almacenar_informacion(config_struct* config_servidor, tabla_paginacion* una_tabla, t_list* lista);
void reservar_marco(int cantidad_marcos, config_struct* configuracion, t_list* tabla_aux, int pid );
void eliminar_estructura_memoria(t_list* tabla_aux);
void leer_informacion(config_struct* config_servidor, tabla_paginacion* una_tabla, t_list* lista);
int escribir_atributo(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_char_tarea(char caracter, int offset, int nro_marco, config_struct* config_s);
void dump_memoria();
void buscar_marco(int id_marco,int * estado,int* proceso, int *pagina);
int lugar_swap_libre();
void actualizar_lru(marco* un_marco);
int reemplazo_clock();
void actualizar_tripulante(tcbTripulante* tripulante, int id_patota);


//swap

void swap_pagina(void* contenidoAEscribir,int numDeBloque);
void borrarBloque(int numeroDeBloque, int tamanioDeBloque);
char* completarBloque(char* bloqueACompletar);
void* recuperar_pag_swap(int numDeBloque);
char* vaciar_bloque(char* bloqueAVaciar);

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
espacio_de_memoria* crear_espacio_de_memoria(int base, int tam, bool libre);
void imprimir_tabla_espacios_de_memoria();
void eliminar_espacio_de_memoria(int base);
espacio_de_memoria* buscar_espacio_de_memoria_libre(int tam);
espacio_de_memoria* busqueda_first_fit(int tam);
espacio_de_memoria* busqueda_best_fit(int tam);
espacio_de_memoria* asignar_espacio_de_memoria(size_t tam);


pcbPatota* crear_pcb(uint32_t numero_patota);


/*FINALIZACION*/

//inicializar PCB desde lo mandado
tcbTripulante* crear_tripulante(uint32_t, char, uint32_t, uint32_t, uint32_t, uint32_t);
pcbPatota* crear_patota(uint32_t , uint32_t);
//solo para comprobar que se formaron bien
void mostrarTripulante(tcbTripulante*,pcbPatota*);

/*Calcular el tamaño de las diferentes estructuras o paquetes a enviar*/
size_t tamanio_tcb(tcbTripulante*);
size_t tamanio_pcb(pcbPatota*);
/*FINALIZACION*/

/*crear la estructura de un nuevo tripulante*/
tcbTripulante* crear_tripulante(uint32_t, char, uint32_t, uint32_t, uint32_t, uint32_t);


char* string_nombre_tarea(int);
char* tarea_a_string(tarea* t);


//Mapa

int iniciar_mapa(int * cols, int * rows);
void* crear_mapa();

#endif /* CONEXIONES_H_ */

