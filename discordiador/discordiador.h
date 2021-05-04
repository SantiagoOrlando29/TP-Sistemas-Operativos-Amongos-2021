#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utils.h"
#include "readline/readline.h"



t_config* leer_config(void);
int crear_conexion(char *ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
void liberar_conexion(int socket_cliente);
void terminar_discordiador(int,int,t_log*,t_config*);

#endif /*DISCORDIADOR_H_*/
