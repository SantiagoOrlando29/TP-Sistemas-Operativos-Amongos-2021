#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "readline/readline.h"
#include "utils_discordiador.h"




int crear_conexion(char *ip, char* puerto);
void liberar_conexion(int socket_cliente);
void terminar_discordiador(int,int,t_log*);

#endif /*DISCORDIADOR_H_*/
