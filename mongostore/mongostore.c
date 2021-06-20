#include "mongostore.h"
#include<openssl/md5.h>
//#include<pthread.h>

config_struct configuracion;
t_log* logger;

pthread_mutex_t mutex_blocks;

int blocks_size=8; //acepta 8 caracteres de char
int blocks=2;

superBloque* superbloque;
char* bloques_copia;

void escribirArchivoSuperBloque(char* bitmap){

	FILE* archivo;

	archivo=fopen("/home/utnso/polus/SuperBloque.ims","wt");
	fputs(string_from_format("BLOCK_SIZE=%d\n",blocks_size),archivo);
	fputs(string_from_format("BLOCKS=%d\n",blocks),archivo);
	fputs(string_from_format("BITMAP=%s\n",bitmap),archivo);

	fclose(archivo);
}

void crearSuperBloque(){
	t_config* archivo=config_create("SuperBloque.ims");

	superbloque->block_size=config_get_int_value(archivo,"BLOCK_SIZE");
	superbloque->blocks=config_get_int_value(archivo,"BLOCKS");
	superbloque->bitmap=config_get_string_value(archivo,"BITMAP");
}

void generar_oxigeno(int cant,char* bloques){
	// size de archivo en bytes
	// char = 1 bytes
	bloques_recurso* oxigeno=asignarBloques(cant,superbloque,'O',bloques);
	crearRecursoMetadata(oxigeno,"0xMD5oxigeno","/home/utnso/polus/Files/Oxigeno.ims");
}
void generar_comida(int cant,char* bloques){
	// size de archivo en bytes
	// char = 1 bytes
	bloques_recurso* comida=asignarBloques(cant,superbloque,'C',bloques);
	crearRecursoMetadata(comida,"0xMD5oxigeno","/home/utnso/polus/Files/Comida.ims");
}
void generar_basura(int cant,char* bloques){
	// size de archivo en bytes
	// char = 1 bytes
	bloques_recurso* basura=asignarBloques(cant,superbloque,'B',bloques);
	crearRecursoMetadata(basura,"0xMD5oxigeno","/home/utnso/polus/Files/Basura.ims");
}

bloques_recurso* recuperarValores(char* path){
	bloques_recurso* recurso;
	t_config* archivo=config_create(path);
	recurso->size=config_get_int_value(archivo,"SIZE");
	recurso->block_count=config_get_int_value(archivo,"BLOCK_COUNT");
	recurso->blocks=config_get_string_value(archivo,"BLOCKS");
	recurso->caracter_llenado=(char *)config_get_string_value(archivo,"CARACTER_LLENADO");

	return recurso;
}

/*falta contemplar el caso donde se sobrepasa la cantidad de bloques*/
/* Tambien se escribe en bloques  los recursos y bitmap es modificado */
bloques_recurso* asignarBloques(int cant,superBloque* sb,char caracter,char* bloques){
	bloques_recurso* recurso;
	char* bloques_asignados="[";
	int i=0;
	int inicio=0;
	int aux_cant=cant;
	recurso->block_count=0;

	pthread_mutex_lock(&mutex_blocks); //sector critica poruqe se usa superBloque  y bloques

	if(sb->bitmap[i]=='0'){
		string_append_with_format(&bloques_asignados,"%s",i);
		for(size_t cont=0;cont<sb->block_size;cont++){
			if(cont < cant ){
				bloques[inicio+cont]=caracter;
				aux_cant= aux_cant - 1;
			}
		}
		sb->bitmap[i]='1';
		recurso->block_count++;
		cant= cant-aux_cant; // vemos si basta con solo un bloque
	}
	else{
		/*
		 * Vimos que esta ocupado
		 * Vemos si esta ocupado con los mismos recursos que generaremos
		 * Solo vemos si el primer byte es del mismo caracter recurso
		 */
		if(sb->bitmap[i]=='1' && bloques[inicio]==caracter){
			string_append_with_format(&bloques_asignados,"%s",i);
		}
	}


	if(cant > 0){ //Si basta con  el primer bloeque vacio o necesitamos mas bloques o el primer bloque esta ocupado
		for(i=1;i<sb->blocks;i++){ //recorremos el bitmap
			inicio=i*sb->block_size; //definimos el inicio del bloque
			if(sb->bitmap[i]=='0'){ //vemos si esta libre
				string_append_with_format(&bloques_asignados,",%s",i);
				for(size_t cont=0;cont<sb->block_size;cont++){
					if(cont < cant){
						bloques[inicio + cont]=caracter;
						aux_cant= aux_cant - 1;
					}
				}
				sb->bitmap[i]='1';
				recurso->block_count++;
			}
			else{
				if(sb->bitmap[i]=='1' && bloques[inicio]==caracter){
					string_append_with_format(&bloques_asignados,",%s",i);
				}
			}
		}
	}

	/*poner en una funcion sincronizacion
	 * escribe en el archivo el cambio del bitmap
	 */
	//escribirArchivoSuperBloque(sb.bitmap);

	pthread_mutex_unlock(&mutex_blocks);
	string_append(&bloques_asignados,"]");

	recurso->blocks=bloques_asignados;
	recurso->caracter_llenado=caracter;
	recurso->size=cant; // char = 1 byte
	return recurso; // un string de donde se han guardado
}

void crearRecursoMetadata(bloques_recurso* recurso,char* md5,char* path){
	FILE* archivo;


	archivo=fopen(path,"wt");

	fputs(string_from_format("SIZE=%d\n",recurso->size),archivo);
	fputs(string_from_format("BLOCK_COUNT=%d\n",recurso->block_count),archivo);
	fputs(string_from_format("BLOCKS=%s\n",recurso->blocks),archivo);
	fputs(string_from_format("CARACTER_LLENADO=%c\n",recurso->caracter_llenado),archivo);
	fputs(string_from_format("MD5_ARCHIVO=%s\n",md5),archivo);

	fclose(archivo);
}

void iniciarFS(){
	crearDireccion("/home/utnso/polus");
	crearDireccion("/home/utnso/polus/Files");
	crearDireccion("/home/utnso/polus/Files/Bitacoras");
/*verificar si ya existe superbloque.ims*/
	if(archivoExiste("/home/utnso/polus/SuperBloque.ims")==0){
		escribirArchivoSuperBloque(string_repeat('0',blocks));
		bloques_copia=string_repeat('0',blocks);
		crearSuperBloque();
		if(archivoExiste("/home/utnso/polus/Blocks.ims")==0){ //tampoco existe el bloques.ims
			iniciarBlocks(superbloque->block_size * superbloque->blocks);
		}
	}
	else{ //existe superbloque
		if(archivoExiste("/home/utnso/polus/Blocks.ims")==0){ //preguntar existe el bloques.ims
			iniciarBlocks(superbloque->block_size * superbloque->blocks);
		}
	}

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
	if(0 == ftruncate(fd,tamanio)){ // se define el tamanio
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
/*
crearRecursoMetadata(133,3,"[1,2,3]",'O',"0123","/home/utnso/polus/Files/Oxigeno.ims");
crearRecursoMetadata(32,2,"[4,5]",'C',"0123","/home/utnso/polus/Files/Comida.ims");
crearRecursoMetadata(64,1,"[6]",'B',"0132465","/home/utnso/polus/Files/Basura.ims");
*/

int main(void)
{

	leer_config();


	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	logger = log_create("mongostore.log", "mongostore", 1, LOG_LEVEL_DEBUG);

	int server_fd = iniciar_servidor(configuracion.ip_mongostore, configuracion.puerto_mongostore);

	iniciarFS();

/*
	superBloque sb=iniciarSuperBloque(sb);
	sb.block_size=blocks_size;
	sb.blocks=blocks;
	sb.bitmap=string_repeat('0', sb.blocks); //inicia el bitmap libre

*/	//iniciarBlocks(sb.block_size*sb.blocks);

	log_info(logger, "Mongo listo para recibir ordenes desde Discordiador");
	int cliente_fd = esperar_cliente(server_fd);
	printf("\n");


	//mapeo();
/*
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

	}*/
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

