#include"discordiador.h"

config_discordiador configuracion;
//config_struct configuracion;
int main(int argc, char* argv[]) {
	//config_struct configuracion;
	t_log* logger;


	//Reinicio el anterior y arranco uno nuevo
	FILE* archivo = fopen("discordiador.log","w");
	fclose(archivo);
	logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);



	leer_config();
	//leer numeros random
	//leer_tareas("tareas.txt");
	int conexionMiRam = crear_conexion(configuracion.ip_miram,configuracion.puerto_miram);
	int conexionMongoStore = crear_conexion(configuracion.ip_mongostore, configuracion.puerto_mongostore);

	menu_discordiador(conexionMiRam, conexionMongoStore,logger);

	terminar_discordiador(conexionMiRam, conexionMongoStore, logger);


	return 0;


}

int menu_discordiador(int conexionMiRam, int conexionMongoStore,  t_log* logger) {
	int tipoMensaje = -1;
	int tid=1;
	int posx = 0;
	int posy = 0;
	while(1){
		tcbTripulante* tripulante = crear_tripulante(tid,'N',5,6,1,1);
		t_paquete* paquete;
		char* leido = readline("");
		switch (codigoOperacion(leido)){
			case PRUEBA:
				paquete = crear_paquete(PRUEBA);
				//            TAREA_TRIPULANTE PARAMETRO POSX POSY TIEMPO
				tarea* t_tarea=crearTarea(GENERAR_OXIGENO,5,1,1,5);
				agregar_a_paquete(paquete,t_tarea, sizeof(tarea));
				printf("\n Tarea GENERAR OXIGENO \n");
				enviar_paquete(paquete,conexionMongoStore);
				break;

			case INICIAR_PATOTA:
				paquete = crear_paquete(INICIAR_PATOTA);
				char** parametros = string_split(leido," ");
				int cantidad_tripulantes  = atoi(parametros[1]);
				char *tareas = leer_tareas(parametros[2]);
				printf("Las tareas recibidas por parametro son: %s\n", tareas);
				agregar_a_paquete(paquete, tareas, ((sizeof(char)*5) + (sizeof(int)*5))*7); //Revisar tamanio paquete
				//agregar_a_paquete(paquete, "Hola Mundo", 11);
/*
INICIAR_PATOTA 5 tareas.txt 1|2 3|4
*/
				int j = 0;
				for(int i = 0; i < cantidad_tripulantes ; i++){
					if (j == 0){
						if (parametros[i+3] == NULL){
							j=1;
						}else {
							posx = parametros[i+3][0] - 48;
							posy = parametros[i+3][2] - 48;
						}
					}

					printf("El tripulante %d tiene posx %d y posy %d\n", i+1, posx,posy);
					tripulante = crear_tripulante(tid,'N',posx,posy,1,1);
					posx=0;
					posy=0;
					agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));
					tid++;

				}

				enviar_paquete(paquete, conexionMiRam);

				//agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));
				//tcbTripulante* tripulante = crear_tripulante(1,'N',5,6,1,1);
				//agregar_a_paquete(paquete, tripulante, tamanio_tcb(tripulante));

				eliminar_paquete(paquete);
				break;

			case LISTAR_TRIPULANTES:
				enviar_header(LISTAR_TRIPULANTES, conexionMiRam);
				tipoMensaje = recibir_operacion(conexionMiRam);
				recibir_lista_tripulantes(tipoMensaje, conexionMiRam, logger);
				break;

			case OBTENER_BITACORA:
				enviar_header(OBTENER_BITACORA, conexionMongoStore);
				tipoMensaje = recibir_operacion(conexionMongoStore);
				t_list* lista = recibir_paquete(conexionMongoStore);
				char* mensaje = (char*)list_get(lista, 0);
				log_info(logger, mensaje);
				list_destroy(lista);
				break;

			case FIN:
				paquete = crear_paquete(FIN);
				enviar_paquete(paquete, conexionMiRam);
				enviar_paquete(paquete, conexionMongoStore);
				eliminar_paquete(paquete);
				return EXIT_FAILURE;

			default:
				mensajeError(logger);
				break;
		}
		free(leido);
		free(tripulante);
	}
}

void leer_config(void){
	t_config * archConfig = config_create("discordiador.config");

	    configuracion.ip_miram = config_get_string_value(archConfig, "IP_MI_RAM_HQ");
	    configuracion.puerto_miram = config_get_string_value(archConfig, "PUERTO_MI_RAM_HQ");
	    configuracion.ip_mongostore = config_get_string_value(archConfig, "IP_I_MONGO_STORE");
	    configuracion.puerto_mongostore = config_get_string_value(archConfig, "PUERTO_I_MONGO_STORE");
	    configuracion.grado_multitarea= config_get_int_value(archConfig, "GRADO_MULTITAREA");
	    configuracion.algoritmo= config_get_string_value(archConfig, "ALGORITMO");
	    configuracion.quantum = config_get_int_value(archConfig, "QUANTUM");
	    configuracion.duracion_sabotaje = config_get_int_value(archConfig, "DURACION_SABOTAJE");
	    configuracion.retardo_cpu = config_get_int_value(archConfig, "RETARDO_CICLO_CPU");
}

void terminar_discordiador (int conexionMiRam, int conexionMongoStore, t_log* logger){

	log_destroy(logger);
	liberar_conexion(conexionMiRam);
	liberar_conexion(conexionMongoStore);
}


