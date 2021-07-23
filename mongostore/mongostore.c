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

	//file_system_iniciar();
	char* archivitos = contar_archivos("/home/utnso/tp-2021-1c-INTratables/mongostore/file_system/Files/Bitacoras");
	//log_debug(logger, "ARCHIVOS PA %s", archivitos);



	int piddd = getpid();
	log_info(logger, "pid %d  \n", piddd);
	signal(SIGUSR1, sig_handler);


    pthread_t servidor;
    if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion))!=0){
        log_info(logger, "Falla al crearse el hilo");
    }

    pthread_t servidor_sabotaje;
    if((pthread_create(&servidor_sabotaje,NULL,(void*)iniciar_servidor2,&configuracion))!=0){
        log_info(logger, "Falla al crearse el hilo 2");
    }

    pthread_join(servidor,NULL);
    pthread_join(servidor_sabotaje,NULL);



	log_info(logger, "Finaliza main");

	return EXIT_SUCCESS;
}

//TODO BITACORA, SIGNAL (LINKEADO A mandar_primer_ubicacion_sabotaje)

//TODO EN UTILS.C EN DESARROLLO FSCK Y CONSUMIR_RECURSO (NO TERMINADO 2)



