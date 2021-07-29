#include "mongostore.h"

int main(void)
{
	logger = log_create(ARCHIVO_LOGS, PROGRAMA, LOGS_EN_CONSOLA, NIVEL_DE_LOGS_MINIMO);

	if(logger == NULL)
	{
		printf("No se puede abrir el archivo de logs para iniciar el programa\n");
		exit(EXIT_FAILURE);
	}

	log_debug(logger, "LOGGER OK");

	leer_config();
	log_debug(logger, "Testeo config, la IP es: %s", configuracion.ip);

	file_system_iniciar();
	//char* archivitos = contar_archivos(bitacoras_path);  //Chequear caso que no hay archivos


	/*for(int i= 0; i<3;i++){
		t_bitacora_data* bitacora_data = malloc(sizeof(t_bitacora_data));

		bitacora_cargar_data(bitacora_data, i);
		printf("El tid es %d y la ruta completa es %s \n", bitacora_data->tid, bitacora_data->ruta_completa);
		bitacora_guardar_log(bitacora_data, "Edu aprobame el tp");

		free(bitacora_data);

		log_debug(logger, "Se supone ya cargue una bitacora y estoy por enviar el ok %d", i);
	}*/ //Prueba de cargar bitacora

	/*int variable;
	printf("variable: ");
	scanf("%d", &variable);
	superbloque_validar_integridad_cantidad_de_bloques();
	*/ //Prueba de validad integridad cantidad de bloques

	//int variable;
	//printf("variable: ");
	//scanf("%d", &variable);
	//superbloque_validar_integridad_bitmap();
		 //Prueba de validad integridad cantidad de bloques



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
