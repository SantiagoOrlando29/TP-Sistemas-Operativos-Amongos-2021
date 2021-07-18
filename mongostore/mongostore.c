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

	int operacion;
	printf("Elegi un tipo de operacion (0 para una tarea, 1 para cargar una bitacora, 2 para iniciar el fsck, 3 para salir)\n");
	scanf("%d", &operacion);

	while(operacion != SALIR)
	{
		switch(operacion)
		{
			case REALIZAR_TAREA:
				recurso_realizar_tarea();
				break;
			case CARGAR_BITACORA:
				printf("Not yet\n");
				break;
			case INICIAR_FSCK:
				printf("Not yet pero ya casi (EN DESARROLLO\n");//fsck_iniciar();
				break;
			case SALIR:
				printf("Gracias, vuelva prontos\n");
				break;
			default:
				printf("A esa operacion no la tengo, proba con otra\n");
		}
		printf("Elegi un tipo de operacion (0 para una tarea, 1 para cargar una bitacora, 2 para iniciar el fsck, 3 para salir)\n");
		scanf("%d", &operacion);
	}
	log_info(logger, "Finaliza main");

	return EXIT_SUCCESS;
}

//TODO BITACORA, SIGNAL (LINKEADO A mandar_primer_ubicacion_sabotaje)

//TODO EN UTILS.C EN DESARROLLO FSCK Y CONSUMIR_RECURSO (NO TERMINADO 2)

/*//RESTO DEL CODIGO ORIGINAL DEL MAIN
	pthread_t servidor;
	int hilo_servidor = 1;
	if((pthread_create(&servidor,NULL,(void*)iniciar_servidor,&configuracion))!=0){
		log_info(logger, "Falla al crearse el hilo");
	}
	pthread_join(servidor,NULL);
	log_info(logger, "Main: Se genero un hilo");
	//mapeo();
	return EXIT_SUCCESS;
}*/

