

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
#include <commons/config.h>
#include <commons/memory.h>
#include<string.h>
#include<pthread.h>
#include<stdbool.h>
#include <semaphore.h>
#include <signal.h>
#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

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

sem_t MUTEX_PEDIR_TAREA;
sem_t MUTEX_CAMBIAR_ESTADO;
sem_t MUTEX_CAMBIAR_POSICION;
sem_t MUTEX_TABLA_MEMORIA;
sem_t MUTEX_LISTA_TABLAS_SEGMENTOS;

sem_t MUTEX_TABLA_MEMORIA_PAGINACION;
sem_t MUTEX_PETICION_MARCOS;
sem_t MUTEX_LISTA_TABLAS_PAGINAS;
sem_t MUTEX_MEM_PRINCIPAL;
sem_t MUTEX_MEM_SEC;
sem_t MUTEX_MEMORIA;
sem_t MUTEX_SWAP;
sem_t MUTEX_CASE;
sem_t MUTEX_FILE;
sem_t MUTEX_CLOCK;
sem_t MUTEX_LRU;
sem_t SEM_MAPA;

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
	TAREA_MONGO,
	FSCK
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
	uint32_t size;
	void* stream;
} t_buffer;

typedef struct
{
	tipoMensaje mensajeOperacion;
	t_buffer* buffer;
}t_paquete;

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
	t_list* conversion_marcos;
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
	t_list* lista_marcos;
	int cant_tripulantes;
	int long_tareas;

}tabla_paginacion;

typedef struct{
	int id_marco;
	int ultimo_uso;
	presencia ubicacion;
	int bit_uso;
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

//void* recibir_buffer(int*, int);
void* recibir_buffer(uint32_t* size, int socket_cliente);
void iniciar_servidor(config_struct*);
void iniciar_servidor2(config_struct* config_servidor);
int funcionx(int socket_cliente_sabotaje);

int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void leer_config();
tarea* crear_tarea(tarea_tripulante,int,int,int,int);
void enviar_mensaje(char* mensaje, int socket_cliente);
int enviar_mensaje_malloqueado(char* mensaje, int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int bytes);


//Funciones memoria
void iniciar_miram(config_struct* config);
void agregar_memoria_aux(t_list* tabla_aux, config_struct* config);
void imprimir_memoria(t_list* tabla_aux);
void imprimir_seg(t_list* tabla_aux);
int posicion_marco();
void imprimir_ocupacion_marcos(config_struct* configuracion);
int posicion_patota(int id_buscado,t_list* tabla_aux);
void finalizar_miram(config_struct* config_servidor);
int marco_tarea(int posicion_patota, t_list* tabla_aux, int nro_marco);
void agregar_tripulante_marco(tcbTripulante* tripulante, int id_patota, t_list* tabla_aux, config_struct* configuracion);

void escribir_tripulante(tcbTripulante* tripulante, void* posicion_inicial);


/*Operaciones para enviar mensajes desde miram a discordiador*/
t_paquete* crear_paquete(tipoMensaje tipo);
//void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_a_paquete(t_paquete* paquete, void* valor, uint32_t tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void enviar_header(tipoMensaje , int );
int funcion_cliente_segmentacion(int);
int funcion_cliente_paginacion(int);


//PAGINACION
//PAGINACION
int marco_tarea(int posicion_patota, t_list* tabla_aux, int nro_marco);
void agregar_tripulante_marco(tcbTripulante* tripulante, int id_patota, t_list* tabla_aux, config_struct* configuracion);
int cuantos_marcos(int cuantos_tripulantes, int longitud_tarea);
void mostrar_tripulante(tcbTripulante* tripulante,pcbPatota* patota);
int cuantos_marcos_libres(config_struct* config_servidor);
bool almacenar_informacion(t_list* lista, config_struct* config_servidor);
void cargar_patota(int patota_id);
void reservar_marco(int cantidad_marcos, config_struct* configuracion, t_list* tabla_aux, int pid );
void eliminar_estructura_memoria(t_list* tabla_aux);
void leer_informacion(config_struct* config_servidor, t_list* lista, int patota_id);
void* leer_atributo_char(int offset, int nro_marco, config_struct* config_s);
void* leer_atributo(int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_char(tcbTripulante* tripulante, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
void imprimir_marcos_ms();
void imprimir_marcos_mp();

int escribir_atributo_cero_swap(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_uno_swap(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_dos_swap(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_tres_swap(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_char_swap(tcbTripulante* tripulante, int offset, int nro_marco, config_struct* config_s);
int escribir_char_tarea_swap(char caracter, int offset, int nro_marco, config_struct* config_s);


int posicion_vector(int tripulante_id);
int escribir_atributo_cero(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_uno(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_dos(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
int escribir_atributo_tres(uint32_t dato, int offset, int nro_marco, config_struct* config_s);
void leer_atributo_cero(void* dato,int offset, int nro_marco, config_struct* config_s);
void leer_atributo_uno(void* dato,int offset, int nro_marco, config_struct* config_s);
void leer_atributo_dos(void* dato,int offset, int nro_marco, config_struct* config_s);
void leer_atributo_tres(void* dato,int offset, int nro_marco, config_struct* config_s);
void swap_pagina_iniciar();


int escribir_char_tarea(char caracter, int offset, int nro_marco, config_struct* config_s);
void buscar_marco_ms(int id_marco,int * estado,int* proceso, int *pagina);
void buscar_marco(int id_marco,int * estado,int* proceso, int *pagina);
int lugar_swap_libre();
int alcanza_espacio(int* offset,int tamanio_marco, int tipo_dato);
void actualizar_lru(marco* un_marco);
int reemplazo_clock();
marco* buscar_pagina(int num_pag);
void actualizar_tripulante(tcbTripulante* tripulante, int id_patota, config_struct* config_servidor);
int espacios_swap_libres(config_struct* config_servidor);
void imprimir_tabla_paginacion();
void swap_pagina(void* contenidoAEscribir,int numDeBloque);
int swap_a_memoria(int numBloque);
void* recuperar_pag_swap(int numDeBloque);
void limpiar_bloque_swap(int numDeBloque);
void dump_memoria_paginacion();
int reemplazo_lru();
char* obtener_tarea(int id_patota, tcbTripulante* tripulante);
tcbTripulante* obtener_tripulante(int patota_id,int tripulante_id, config_struct* config_servidor);
bool enviar_tarea_paginacion(int socket_cliente, int numero_patota, tcbTripulante* tripulante);




int escribir_char_tarea(char caracter, int offset, int nro_marco, config_struct* config_s);
void buscar_marco_ms(int id_marco,int * estado,int* proceso, int *pagina);
void buscar_marco(int id_marco,int * estado,int* proceso, int *pagina);
int lugar_swap_libre();
int alcanza_espacio(int* offset,int tamanio_marco, int tipo_dato);
void actualizar_lru(marco* un_marco);
int reemplazo_clock();
marco* buscar_pagina(int num_pag);
void actualizar_tripulante(tcbTripulante* tripulante, int id_patota, config_struct* config_servidor);
int espacios_swap_libres(config_struct* config_servidor);
void imprimir_tabla_paginacion();
void swap_pagina(void* contenidoAEscribir,int numDeBloque);
int swap_a_memoria(int numBloque);
void* recuperar_pag_swap(int numDeBloque);
void limpiar_bloque_swap(int numDeBloque);
void dump_memoria_paginacion();
int reemplazo_lru();
char* obtener_tarea(int id_patota, tcbTripulante* tripulante);
tcbTripulante* obtener_tripulante(int patota_id,int tripulante_id, config_struct* config_servidor);
bool enviar_tarea_paginacion(int socket_cliente, int numero_patota, tcbTripulante* tripulante);


//SEGMENTACION
espacio_de_memoria* crear_espacio_de_memoria(int, int, bool);
void imprimir_tabla_espacios_de_memoria();
void eliminar_espacio_de_memoria(int);
void eliminar_espacio_de_memoria_tarea(int base);
espacio_de_memoria* buscar_espacio_de_memoria_libre(int);
espacio_de_memoria* busqueda_first_fit(int);
espacio_de_memoria* busqueda_best_fit(int);
espacio_de_memoria* asignar_espacio_de_memoria(size_t);
void imprimir_tabla_segmentos_patota(tabla_segmentacion*);
void eliminar_segmento(int, tabla_segmentacion*);
int ordenar_memoria();
int unir_espacios_contiguos_libres();
bool patota_segmentacion(int, uint32_t, char*, t_list*);
bool funcion_expulsar_tripulante(int tripulante_id);
int compactar_memoria();
bool enviar_tarea_segmentacion(int socket_cliente, int numero_patota, int id_tripulante);
espacio_de_memoria* buscar_espacio(segmento* segmento);
char* buscar_tarea(espacio_de_memoria* espacio, int prox_instruccion);
bool cambiar_estado(int numero_patota, char nuevo_estado, int tid);
tabla_segmentacion* buscar_tabla_segmentos(int numero_patota);
bool cambiar_posicion(int tid, int posx, int posy, int pid);
void dump_memoria_segmentacion();
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

//Mapa

int iniciar_mapa(int * cols, int * rows);
void* crear_mapa();

#endif /* CONEXIONES_H_ */

