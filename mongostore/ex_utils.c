/*
//Setea al bitmap apuntado por el campo bitmap de la estructura superbloque segun los bloques ocupados que recibe como parametro
void bitarray_setear_bits_segun_lista_en_cadena(char* bitarray_char, size_t bitarray_size, char* cadena)
{
	log_debug(logger, "Se ingresa a bitarray_setear_bits_segun_lista_en_cadena(char* bitarray_char, char* cadena)");

	char** lista_indices_bits = string_get_string_as_array(cadena);
	t_bitarray* bitarray = bitarray_create_with_mode(bitarray_char, bitarray_size, LSB_FIRST);

	int cantidad_bits_a_setear = cadena_cantidad_elementos_en_lista(cadena);

	int bit;
	for(int i = 0; i < cantidad_bits_a_setear; i++)
	{
		bit = atoi(lista_indices_bits[i]);
		bitarray_set_bit(bitarray, bit);
	}

	cadena_eliminar_array_de_cadenas(&lista_indices_bits, cantidad_bits_a_setear);
	bitarray_destroy(bitarray);

	log_debug(logger, "Bitarray seteado segun lista en cadena");
}








*/
