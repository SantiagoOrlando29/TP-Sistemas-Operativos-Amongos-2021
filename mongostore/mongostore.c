#include "mongostore.h"
config_struct configuracion;
t_log* logger;

int main(void)
{

	leer_config();


	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	logger = log_create("mongostore.log", "mongostore", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor(configuracion.ip_mongostore, configuracion.puerto_mongostore);
	log_info(logger, "Mongo listo para recibir ordenes desde Discordiador");
	int cliente_fd = esperar_cliente(server_fd);
	printf("\n");
	iniciarFS();

	superBloque sb;
	sb.block_size=8;
	sb.blocks=2;
	for(int i=0;i<sb.blocks;i++){
		sb.bitmap[i]='0';
	}
	//puts(sb.bitmap);
	printf("\n");

	iniciarSuperBloque(sb);
	iniciarBlocks(sb.block_size*sb.blocks);

	//mapeo();
 while(1)
	{
		t_paquete* paquete;
		int tipoMensaje = recibir_operacion(cliente_fd);
			switch(tipoMensaje)
			{
			case PRUEBA:
				printf("\nPRODUCCE OXIGENO\n");
				eliminar_paquete(paquete);
				break;
			case OBTENER_BITACORA:
				paquete = crear_paquete(OBTENER_BITACORA);
				char* mensaje = "bitacora";
				agregar_a_paquete(paquete, mensaje, strlen(mensaje) +1);
				enviar_paquete(paquete, cliente_fd);
				eliminar_paquete(paquete);
				break;

			case FIN:
				log_error(logger, "el discordiador finalizo el programa. Terminando servidor");
				return EXIT_FAILURE;

			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				return EXIT_FAILURE;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
			}

	}
	return EXIT_SUCCESS;
}

void leer_config(){

    t_config * archConfig = config_create("mongostore.config");

    configuracion.ip_mongostore = config_get_string_value(archConfig, "IP_I_MONGO_STORE");
    configuracion.puerto_mongostore = config_get_string_value(archConfig, "PUERTO_I_MONGO_STORE");
    configuracion.punto_monstaje = config_get_string_value(archConfig,"PUNTO_MONTAJE");
    configuracion.tiempo_sincronizacion =config_get_int_value(archConfig,"TIEMPO_SINCRONIZACION");
    configuracion.posiciones_sabotaje=config_get_array_value(archConfig,"POSICIONES_SABOTAJE");

}
void iniciarFS(){
	crearDireccion("/home/utnso/polus");
	crearDireccion("/home/utnso/polus/Files");

	crearRecursoMetadata(133,3,"[1,2,3]",'O',"0123","/home/utnso/polus/Files/Oxigeno.ims");
	crearRecursoMetadata(32,2,"[4,5]",'C',"0123","/home/utnso/polus/Files/Comida.ims");
	crearRecursoMetadata(64,1,"[6]",'B',"0132465","/home/utnso/polus/Files/Basura.ims");

	crearDireccion("/home/utnso/polus/Files/Bitacoras");
}

void iniciarSuperBloque(superBloque sb){

	/*
	superBloque sb;
	sb.blocks=count;
	sb.block_size=size;
	for(int i=0;i<sb.blocks;i++){ //tamaño del bitmap, cant de bloques
		sb.bitmap[i]=LIBRE; //estado del bloque
	}
	*/
	FILE* archivo;
	archivo=fopen("/home/utnso/polus/SuperBloque.ims","at+");
	fputs(string_from_format("BLOCK_SIZE=%d\n",sb.block_size),archivo);
	fputs(string_from_format("BLOCKS=%d\n",sb.blocks),archivo);
	/* REVEER
	fputs("BITMAP=",archivo);
	for(int i=0;i<strlen(sb.bitmap);i++){
		fputs(string_from_format("%c",sb.bitmap[i]),archivo);
	}
	*/
	//fputs("\n",archivo);
	fputs("BITMAP=000\n",archivo);
	fclose(archivo);

	//return sb;
}

void iniciarBlocks(int tamanio){
	//printf("\n -------- \n");
	//struct stat stat_file;
	char* addr;
	int fd;
	fd = open("/home/utnso/polus/Blocks.ims", O_CREAT|O_RDWR, S_IRWXU);
	if(fd == -1){
		printf("\nEl objeto no pudo ser creado\n");
	}
	if(0<=fd){
		printf("\nSe creo correctamente\n");
	}
	if(0 == ftruncate(fd,tamanio)){ // sedefine el tamanio
				printf("\n Se le pudo asignar tamaño\n");
				//exit(-1);
	}
	addr=mmap(NULL,tamanio,PROT_WRITE,MAP_SHARED,fd,0);
			//printf("\Antes de modificar: %s \n",addr);
	for(size_t i=0; i< tamanio;i++){
		addr[i]='0';
	}
	printf("\nModificado %s \n",addr);

	close(fd);
}
