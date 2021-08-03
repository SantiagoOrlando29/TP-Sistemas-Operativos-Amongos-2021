#include "mongostore.h"

int main(void)
{
	FILE* archivo = fopen(ARCHIVO_LOGS,"w");
	fclose(archivo);
	//logger = log_create("discordiador.log","discordiador",1,LOG_LEVEL_INFO);
	logger = log_create(ARCHIVO_LOGS, PROGRAMA, LOGS_EN_CONSOLA, NIVEL_DE_LOGS_MINIMO);

	if(logger == NULL)
	{
		printf("No se puede abrir el archivo de logs para iniciar el programa\n");
		exit(EXIT_FAILURE);
	}

	log_debug(logger, "LOGGER OK");

	leer_config();
	log_debug(logger, "Testeo config, la IP es: %s", configuracion.ip);

	semaforos_inicializar();

	file_system_iniciar();
	blocks_abrir_hilo_sincronizacion();

	//utils_esperar_a_usuario();

	//Prueba de cargar bitacora con cantidad de tripulantes y mensaje original

	//prueba_de_tareas_random();
	//prueba_de_generar_recursos();

	//prueba_de_consumir_recursos();
	//prueba_de_descartar_basura();
	//prueba_de_cargar_n_bitacoras_con_mensaje(1, "un, dos, tres, probando mas nro tripulante");
	//prueba_de_cargar_bitacora_de_tripulante_n_con_m_mensajes(2, 3, "CACA");

	//fsck_chequeo_de_sabotajes_en_superbloque();
	//fsck_chequeo_de_sabotajes_en_files();

	int piddd = getpid();
	log_info(logger, "pid %d  \n", piddd);
	signal(SIGUSR1, sig_handler);

    pthread_t servidor;
    if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,NULL))!=0){
        log_info(logger, "Falla al crearse el hilo");
    }

    pthread_t servidor_sabotaje;
    if((pthread_create(&servidor_sabotaje,NULL,(void*)iniciar_servidor2,NULL))!=0){
        log_info(logger, "Falla al crearse el hilo 2");
    }

    pthread_join(servidor,NULL);
    pthread_join(servidor_sabotaje,NULL);



	log_info(logger, "Finaliza main");

	return EXIT_SUCCESS;
}
