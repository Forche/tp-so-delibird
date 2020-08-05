/*
 ============================================================================
 Name        : gamecard.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "gamecard.h"


int main(void) {

	read_config();
	init_loggers();
	tall_grass_metadata_info();
	open_bitmap();
	sem_t sem_gameCard_up;
	sem_init(&sem_gameCard_up, 0, 0);
	pthread_mutex_init(&mutexBitmap,NULL);
	pthread_mutex_init(&mutexDirectory,NULL);
	pthread_mutex_init(&mutexMetadataPath,NULL);

	create_listener_thread();
	create_subscription_threads();

	sem_wait(&sem_gameCard_up);
	//TODO: catch SIGKILL
	shutdown_gamecard();

	return EXIT_SUCCESS;
}

void read_config() {
	config = config_create("/home/utnso/tp-2020-1c-Operavirus/gamecard/gamecard.config");

	TIEMPO_DE_REINTENTO_CONEXION = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	TIEMPO_DE_REINTENTO_OPERACION = config_get_int_value(config, "TIEMPO_DE_REINTENTO_OPERACION");
	TIEMPO_RETARDO_OPERACION = config_get_int_value(config, "TIEMPO_RETARDO_OPERACION");
	PUNTO_MONTAJE_TALLGRASS = config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS");
	IP_BROKER = config_get_string_value(config, "IP_BROKER");
	PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");
	IP_GAMECARD = config_get_string_value(config, "IP");
	PUERTO_GAMECARD = config_get_string_value(config, "PUERTO");

}

void init_loggers() {
	logger = log_create("/home/utnso/tp-2020-1c-Operavirus/gamecard/gamecard.log", "gamecard", true, LOG_LEVEL_INFO); //true porque escribe tambien la consola
}

void shutdown_gamecard() {
	munmap(bmap, size_bitmap.st_size);
	bitarray_destroy(bitarray);
	log_destroy(logger);
}

void create_subscription_threads() {
	pthread_t thread_new = create_thread_with_param(subscribe_to, NEW_POKEMON, "subscribe NEW_POKEMON");
	sleep(2);
	pthread_t thread_catch = create_thread_with_param(subscribe_to, CATCH_POKEMON, "subscribe CATCH_POKEMON");
	sleep(2);
	pthread_t thread_get = create_thread_with_param(subscribe_to, GET_POKEMON, "subscribe GET_POKEMON");
}

void subscribe_to(event_code code) {
	t_subscription_petition* new_subscription = build_new_subscription(code, IP_GAMECARD, "Game Card", atoi(PUERTO_GAMECARD));

	t_buffer* buffer = serialize_t_new_subscriptor_message(new_subscription);

	uint32_t broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_connection == -1) {
		log_error(logger, "No se pudo conectar al broker, reintentando en %d seg", TIEMPO_DE_REINTENTO_CONEXION);
		sleep(TIEMPO_DE_REINTENTO_CONEXION);
		subscribe_to(code);
	} else {
		log_info(logger, "Conectada cola code %s al broker", event_code_to_string(code));
		send_message(broker_connection, NEW_SUBSCRIPTOR, NULL, NULL, buffer);
		int still_connected = 1;
		while(still_connected) {
			still_connected = handle_event(&broker_connection);
		}
		log_error(logger, "Se desconecto del broker");
		close(broker_connection);
	}

	new_subscription = build_new_subscription(code, IP_GAMECARD, "Game Card", PUERTO_GAMECARD);
	//subscribe_to(code);
}


void create_listener_thread() {
	create_thread(gamecard_listener, "events_listener");
}

void gamecard_listener() {
	listener(IP_GAMECARD, PUERTO_GAMECARD, handle_event);
}


int handle_event(uint32_t* client_socket) {
	event_code code;
	int bytes = recv(*client_socket, &code, sizeof(event_code), MSG_WAITALL);
	if (bytes == -1 || bytes == 0) {
			return 0;
	}
	t_message* msg = receive_message(code, *client_socket);
	uint32_t size;
	char* pokemon_with_empty;
	t_message_received* message_received = malloc(sizeof(t_message_received));

	switch (code) {
	case NEW_POKEMON: ;
		msg->buffer = deserialize_new_pokemon_message(*client_socket, &size);

		((t_new_pokemon*)msg->buffer)->pokemon_len +=1;
		pokemon_with_empty = malloc(((t_new_pokemon*)msg->buffer)->pokemon_len);
		for(int i = 0; i < (((t_new_pokemon*)msg->buffer)->pokemon_len -1); i++) {
			pokemon_with_empty[i] = (((t_new_pokemon*)msg->buffer)->pokemon)[i];
		}
		pokemon_with_empty[((t_new_pokemon*)msg->buffer)->pokemon_len -1] = '\0';
		((t_new_pokemon*)msg->buffer)->pokemon = pokemon_with_empty;
		log_info(logger, "POKEMON NEW:%s", ((t_new_pokemon*)msg->buffer)->pokemon);

		create_thread_with_param(handle_new_pokemon, msg, "handle_new_pokemon");
		message_received->message_type = NEW_POKEMON;

		break;
	case CATCH_POKEMON: ;
		msg->buffer = deserialize_catch_pokemon_message(*client_socket, &size);

		((t_catch_pokemon*)msg->buffer)->pokemon_len +=1;
		pokemon_with_empty = malloc(((t_catch_pokemon*)msg->buffer)->pokemon_len);
		for(int i = 0; i < (((t_catch_pokemon*)msg->buffer)->pokemon_len -1); i++) {
			pokemon_with_empty[i] = (((t_catch_pokemon*)msg->buffer)->pokemon)[i];
		}
		pokemon_with_empty[((t_catch_pokemon*)msg->buffer)->pokemon_len -1] = '\0';
		((t_catch_pokemon*)msg->buffer)->pokemon = pokemon_with_empty;

		create_thread_with_param(handle_catch_pokemon, msg, "handle_catch");
		message_received->message_type = CATCH_POKEMON;

		break;
	case GET_POKEMON: ;
		msg->buffer = deserialize_get_pokemon_message(*client_socket, &size);

		((t_get_pokemon*)msg->buffer)->pokemon_len +=1;
		pokemon_with_empty = malloc(((t_get_pokemon*)msg->buffer)->pokemon_len);
		for(int i = 0; i < (((t_get_pokemon*)msg->buffer)->pokemon_len -1); i++) {
			pokemon_with_empty[i] = (((t_get_pokemon*)msg->buffer)->pokemon)[i];
		}
		pokemon_with_empty[((t_get_pokemon*)msg->buffer)->pokemon_len -1] = '\0';
		((t_get_pokemon*)msg->buffer)->pokemon = pokemon_with_empty;

		create_thread_with_param(handle_get_pokemon, msg, "handle_get");
		message_received->message_type = GET_POKEMON;
		break;
	}
	message_received->received_message_id = msg->id;
	message_received->subscriptor_len = string_length("Game Card") + 1;
	message_received->subscriptor_id = "Game Card";

	send_message_received_to_broker(message_received, msg->id, msg->correlative_id);

	return 1;
	//TODO:free(msg);
}

void send_message_received_to_broker(t_message_received* message_received, uint32_t id, uint32_t correlative_id){
	t_buffer* buffer_received = serialize_t_message_received(message_received);
	uint32_t connection = connect_to(IP_BROKER,PUERTO_BROKER);
	if(connection == -1) {
			log_error(logger, "No se pudo enviar el ACK al broker. No se pudo conectar al broker");
		} else {
			log_info(logger, "Se envio el ACK al broker");
		}

	send_message(connection, MESSAGE_RECEIVED, id, correlative_id, buffer_received);

}

void handle_get_pokemon(t_message* msg){
	t_get_pokemon* get_pokemon = (t_get_pokemon*)msg->buffer;
	int exists_directory = check_pokemon_directory(get_pokemon->pokemon, msg->event_code);
	if(exists_directory){
		char* path_pokemon = string_from_format("%s/Files/%s/Metadata.bin", PUNTO_MONTAJE_TALLGRASS, get_pokemon->pokemon);
		check_if_file_is_open(path_pokemon);
		char* blocks_content = read_blocks_content(path_pokemon);
		log_info(logger, "El contenido de los bloques en %s es %s",path_pokemon, blocks_content);
		sleep(TIEMPO_RETARDO_OPERACION);
		t_list* pokemon_positions = get_positions_from_buffer(blocks_content);
		uint32_t* positions_uint32 = positions_to_uint32(pokemon_positions);
		if(pokemon_positions != NULL){
			t_localized_pokemon* localized_pokemon = create_localized_pokemon(get_pokemon->pokemon_len, get_pokemon->pokemon, list_size(pokemon_positions), positions_uint32);
			send_localized_to_broker(localized_pokemon, msg->id);
		}
		close_file_pokemon(path_pokemon);
		//list_destroy(pokemon_positions);
	}

}

t_localized_pokemon* create_localized_pokemon(uint32_t pokemon_len,	char* pokemon, uint32_t positions_count, uint32_t* positions){
	t_localized_pokemon* localized_pokemon = malloc(sizeof(t_localized_pokemon));
	localized_pokemon->pokemon_len = pokemon_len;
	localized_pokemon->pokemon = malloc(pokemon_len);
	memcpy(localized_pokemon->pokemon, pokemon, pokemon_len);
	localized_pokemon->positions_count = positions_count;
	localized_pokemon->positions = positions;
	return localized_pokemon;
}

uint32_t* positions_to_uint32(t_list* pokemon_positions){
	uint32_t* positions;
	positions = malloc((list_size(pokemon_positions))*2);
	for(int i = 0; i<(list_size(pokemon_positions));i++){
		positions[i] = ((t_position*)(list_get(pokemon_positions,i)))->pos_x;
		positions[i+1] = ((t_position*)(list_get(pokemon_positions,i)))->pos_y;
	}
	return positions;
}

void send_localized_to_broker(t_localized_pokemon* localized_pokemon, uint32_t id){
	t_buffer* buffer = serialize_t_localized_pokemon_message(localized_pokemon);
	uint32_t connection = connect_to(IP_BROKER,PUERTO_BROKER);
	if(connection == -1) {
		log_error(logger, "No se pudo enviar el localized al broker. No se pudo conectar al broker");
	} else {
		log_info(logger, "Se envio el localized al broker");
	}
	send_message(connection, LOCALIZED_POKEMON, id, 0, buffer);
	//free(buffer);
}

void handle_catch_pokemon(t_message* msg){
	t_caught_pokemon* caught_pokemon = malloc(sizeof(t_appeared_pokemon));
	t_catch_pokemon* catch_pokemon = (t_catch_pokemon*)msg->buffer;
	int exists_directory = check_pokemon_directory(catch_pokemon->pokemon, msg->event_code);
	if(exists_directory){
		char* path_pokemon = string_from_format("%s/Files/%s/Metadata.bin", PUNTO_MONTAJE_TALLGRASS, catch_pokemon->pokemon);
		check_if_file_is_open(path_pokemon);
		t_position* position = ckeck_position_exists_catch_pokemon(path_pokemon,catch_pokemon);
		if(position != NULL){
			log_info(logger, "El pokemon %s se encuentra en la posicion x:%d-y:%d",catch_pokemon->pokemon, position->pos_x, position->pos_y);
			if(position->count == 1){
				remove_position(position, path_pokemon);
			} else {
				decrease_position(position, path_pokemon);
			}
		}
		caught_pokemon->result = 1;
		close_file_pokemon(path_pokemon);
	}else {
		caught_pokemon->result = 0;
	}

	send_caught_to_broker(caught_pokemon, msg->id);
}

void send_caught_to_broker(t_caught_pokemon* caught_pokemon, uint32_t correlative_id){
	t_buffer* buffer = serialize_t_caught_pokemon_message(caught_pokemon);
	uint32_t connection = connect_to(IP_BROKER,PUERTO_BROKER);
	if(connection == -1) {
		log_error(logger, "No se pudo enviar el caught al broker. No se pudo conectar al broker");
	} else {
		log_info(logger, "Se envio el caught al broker");
	}
	send_message(connection, CAUGHT_POKEMON, 0, correlative_id, buffer);
	//free(buffer);
}

void decrease_position(t_position* position, char* path_pokemon){
	char* blocks_content = read_blocks_content(path_pokemon);
	log_info(logger, "El contenido de los bloques en %s es %s",path_pokemon, blocks_content);
	t_list* pokemon_positions = get_positions_from_buffer(blocks_content);

	int pos = position_list(pokemon_positions,position);
	t_position* position_in_list = list_get(pokemon_positions,pos);

	position_in_list->count -=1;
	log_info(logger, "Restamos 1 pokemon de la posicion x:%d y:%d quedan %d en %s", position->pos_x, position->pos_y, position_in_list->count,path_pokemon);
	list_replace(pokemon_positions,pos,position_in_list);


	write_positions_on_files(pokemon_positions, path_pokemon);
	//list_destroy(pokemon_positions);
}

void remove_position(t_position* position, char* path_pokemon){
	char* blocks_content = read_blocks_content(path_pokemon);
	log_info(logger, "El contenido de los bloques en %s es %s",path_pokemon, blocks_content);
	t_list* pokemon_positions = get_positions_from_buffer(blocks_content);
	int position_from_list = position_list(pokemon_positions,position);
	log_info(logger, "Restamos 1 pokemon de la posicion x:%d y:%d quedan 0, entonces removemos el pokemon de %s", position->pos_x, position->pos_y, path_pokemon);
	list_remove(pokemon_positions,position_from_list);

	write_positions_on_files(pokemon_positions, path_pokemon);
	//list_destroy(pokemon_positions);
}

t_position* ckeck_position_exists_catch_pokemon(char* path_pokemon, t_catch_pokemon* pokemon){
	char* blocks_content = read_blocks_content(path_pokemon);
	t_list* pokemon_positions = get_positions_from_buffer(blocks_content);

	bool _compare(t_position* position){
		return (pokemon->pos_x == position->pos_x) && (pokemon->pos_y == position->pos_y);
	}
	t_position* find_position = NULL;
	find_position = list_find(pokemon_positions, _compare);

	if(find_position == NULL){
		log_error(logger, "No existe la posicion %d-%d dentro del archivo de %s", pokemon->pos_x, pokemon->pos_y, path_pokemon);
	}
	//TODO:list_destroy(pokemon_positions);
	return find_position;
}

void handle_new_pokemon(t_message* msg){
	t_new_pokemon* new_pokemon = msg->buffer;
	check_pokemon_directory(new_pokemon->pokemon, msg->event_code);

	char* path_pokemon = string_from_format("%s/Files/%s/Metadata.bin", PUNTO_MONTAJE_TALLGRASS, new_pokemon->pokemon);

	check_if_file_is_open(path_pokemon);

	t_list* pokemon_positions = ckeck_position_exists_new_pokemon(path_pokemon,new_pokemon);
	write_positions_on_files(pokemon_positions, path_pokemon);

	close_file_pokemon(path_pokemon);

	t_appeared_pokemon* appeared_pokemon = create_appeared_pokemon(new_pokemon->pokemon_len, new_pokemon->pokemon, new_pokemon->pos_x, new_pokemon->pos_y);
	send_appeared_to_broker(appeared_pokemon, msg->id);
	free(path_pokemon);
	list_destroy(pokemon_positions);
	//TODO:free(((t_appeared_pokemon*)appeared_pokemon->pokemon));
}

t_list* ckeck_position_exists_new_pokemon(char* path_pokemon, t_new_pokemon* pokemon){
	char* blocks_content = read_blocks_content(path_pokemon);
	log_info(logger, "El contenido de los bloques en %s es %s",path_pokemon, blocks_content);
	t_list* pokemon_positions = get_positions_from_buffer(blocks_content);
	t_position* new_position = create_position(pokemon->pos_x, pokemon->pos_y, pokemon->count);

	bool _compare(t_position* position){
		return (pokemon->pos_x == position->pos_x) && (pokemon->pos_y == position->pos_y);
	}
	t_position* find_position = NULL;
	find_position = list_find(pokemon_positions, _compare);

	if(find_position != NULL){
		log_info(logger,"Sumamos %d a %d total: %d del pokemon %s en la posicion x:%d y:%d en %s", pokemon->count, find_position->count, pokemon->count + find_position->count, pokemon->pokemon, pokemon->pos_x, pokemon->pos_y, path_pokemon);
		find_position->count += pokemon->count;
		int position = position_list(pokemon_positions,find_position);
		list_replace(pokemon_positions,position,find_position);

	}else{
		log_info(logger,"Agregamos %d %s en la posicion x:%d y:%d en %s",pokemon->count, pokemon->pokemon,pokemon->pos_x, pokemon->pos_y, path_pokemon);
		list_add(pokemon_positions, new_position);
	}

	free(blocks_content);
	return pokemon_positions;
}

void send_appeared_to_broker(t_appeared_pokemon* appeared_pokemon, uint32_t id){
	t_buffer* buffer = serialize_t_appeared_pokemon_message(appeared_pokemon);
	uint32_t connection = connect_to(IP_BROKER,PUERTO_BROKER);
	if(connection == -1) {
		log_error(logger, "No se pudo enviar el appeared al broker. No se pudo conectar al broker");
	} else {
		log_info(logger, "Se envio el appeared al broker");
	}
	send_message(connection, APPEARED_POKEMON, id, 0, buffer);
	//free(buffer);
}

t_appeared_pokemon* create_appeared_pokemon(uint32_t pokemon_len, char* pokemon, uint32_t pos_x, uint32_t pos_y){
	t_appeared_pokemon* appeared_pokemon = malloc(sizeof(t_appeared_pokemon));
	appeared_pokemon->pokemon_len = pokemon_len;
	appeared_pokemon->pokemon = malloc(pokemon_len);
	memcpy(appeared_pokemon->pokemon, pokemon, pokemon_len);
	appeared_pokemon->pos_x = pos_x;
	appeared_pokemon->pos_y = pos_y;
	return appeared_pokemon;
}

void close_file_pokemon(char* path_pokemon){
	pthread_mutex_lock(&mutexMetadataPath);
	t_config* metadata = config_create(path_pokemon);
	char* is_open = config_get_string_value(metadata, "OPEN");
	if(string_equals_ignore_case(is_open, "Y") != 0){
		is_open = "N";
		config_set_value(metadata,"OPEN",is_open);
	}
	config_save(metadata);
	log_info(logger, "Cerramos el archivo %s luego de utilizarlo",path_pokemon);
	config_save(metadata);
	config_destroy(metadata);
	pthread_mutex_unlock(&mutexMetadataPath);
}

int position_list(t_list* pokemon_positions, t_position* find_position){
	int i;
	int pos = -1;
	for(i=0; i<list_size(pokemon_positions);i++){
		int pos_x = ((t_position*)(list_get(pokemon_positions,i)))->pos_x;
		int pos_y = ((t_position*)(list_get(pokemon_positions,i)))->pos_y;
		if(pos_x == find_position->pos_x &&  pos_y == find_position->pos_y){
			pos = i;
		}
	}
	return pos;
}

void write_positions_on_files(t_list* pokemon_positions, char* path_new_file){
	char* array_positions = string_new();
	while(!list_is_empty(pokemon_positions)){
		t_position* position = list_remove(pokemon_positions, 0);
		char* string_x = string_from_format("%d-", position->pos_x);
		char* string_y = string_from_format("%d=", position->pos_y);
		char* string_count = string_from_format("%d\n", position->count);
		string_append(&array_positions, string_x);
		string_append(&array_positions, string_y);
		string_append(&array_positions, string_count);

		free(string_x);
		free(string_y);
		free(string_count);
		destroy_position(position);
	}
	char* null = "\0";
	string_append(&array_positions, null);
	int size_array_positions = string_length(array_positions);
	t_list* writed_blocks = write_blocks_and_metadata(size_array_positions, array_positions, path_new_file);
	free(array_positions);
	log_info(logger, "Se escribe el archivo %s con cantidad de bloques: %d y size= %d", path_new_file, list_size(writed_blocks), size_array_positions);
	list_destroy(writed_blocks);
}

void destroy_position(t_position* position){
	free(position);
}

t_list* pokemon_blocks(int blocks_needed, char* metadata_path, int size_array_positions){
	t_config* metadata = config_create(metadata_path);
	char** actual_blocks = config_get_array_value(metadata, "BLOCKS");
	char* blocks_as_array_of_char = string_new();
	int quantity_actual_blocks = 0;
	t_list* blocks_as_char =  list_create();

	while(actual_blocks[quantity_actual_blocks]!=NULL){
		list_add(blocks_as_char, actual_blocks[quantity_actual_blocks]);
		blocks_as_array_of_char = add_block_to_array(blocks_as_array_of_char, actual_blocks[quantity_actual_blocks]);
		quantity_actual_blocks++;
	}

	int blocks_are_enough = blocks_needed - quantity_actual_blocks;
	while(blocks_are_enough > 0){
		int new_block = get_available_block();
		blocks_are_enough -= 1;
		blocks_as_array_of_char = add_block_to_array(blocks_as_array_of_char, string_from_format("%d", new_block));
		quantity_actual_blocks++;

		list_add(blocks_as_char, string_from_format("%d",new_block));
	}

	while(blocks_are_enough < 0){
		remove_block_from_bitmap(actual_blocks[quantity_actual_blocks-1]);
		blocks_are_enough++;
		actual_blocks[quantity_actual_blocks-1] = '\0';
		blocks_as_array_of_char = remove_last_block_from_array(blocks_as_array_of_char);
		list_remove(blocks_as_char,quantity_actual_blocks-1);
		quantity_actual_blocks--;
	}

	log_info(logger,"Bloques a guardar para el archivo %s:%s",metadata_path, blocks_as_array_of_char);
	dictionary_put(metadata->properties, "BLOCKS", blocks_as_array_of_char);
	char* size_array = string_new();
	string_append_with_format(&size_array,"%d", size_array_positions);
	config_set_value(metadata,"SIZE", size_array);
	//TODO:
	/*for(int i = 0;actual_blocks[i]!=NULL;i++){
		free(actual_blocks[i]);

	}
	free(actual_blocks);*/
	config_save(metadata);
	//config_destroy(metadata);
	//TODO:free(blocks_as_array_of_char);
	free(size_array);

	return blocks_as_char;
}

char* add_block_to_array(char* blocks_as_array, char* block_to_add){
	if(string_length(blocks_as_array)){
		blocks_as_array[(string_length(blocks_as_array))-1] = ',';
		string_append(&blocks_as_array,block_to_add);
		char* close_block = "]\0";
		string_append(&blocks_as_array,close_block);
	} else {
		blocks_as_array = string_from_format("[%s]\0",block_to_add);
	}
	return blocks_as_array;
}

char** metadata_blocks_to_actual_blocks(char* metadata_blocks){
	int blocks_len = string_length(metadata_blocks);
	int blocks_quantity = (blocks_len-1)/2;
	char** blocks = malloc(blocks_quantity);
	blocks[0] = string_from_format("%c", metadata_blocks[1]);
	if(blocks_quantity > 1){
		for(int i = 1; i<=(blocks_quantity*2);i+=2) {
			blocks[i] = string_from_format("%c", metadata_blocks[i+2]);
		}
	}

	return blocks;
}

char* remove_last_block_from_array(char* blocks_as_array){
	//Ej: blocks_as_array = [1,2,4] len=6
	//si queda vacio blocks_as_array = [1] len=3
	int length = string_length(blocks_as_array);
	blocks_as_array[length-1] = '\0';
	blocks_as_array[length-2] = '\0';
	blocks_as_array[length-3] = '\0';
	if(length == 3){
		int available_block = get_available_block();
		char* next_available_block = string_from_format("[%d]", available_block);
		string_append(&blocks_as_array,next_available_block);
	} else {
		char* close_block = "]\0";
		string_append(&blocks_as_array,close_block);
	}
	return blocks_as_array;
}

void remove_block_from_bitmap(char* block){
	pthread_mutex_lock(&mutexBitmap);
	int block_to_remove = atoi(block);
	bitarray_clean_bit(bitarray, block_to_remove);
	blocks_available++;

	log_info(logger, "Cantidad de bloques disponibles: %d", blocks_available);
	log_info(logger, "Se desocupa el bloque %d", block_to_remove);
	msync(bmap, sizeof(bitarray), MS_SYNC);
	pthread_mutex_unlock(&mutexBitmap);
}

t_list* write_blocks_and_metadata(int size_array_positions, char* array_positions, char* metadata_path){
	int quantity_of_blocks = my_ceil(size_array_positions, block_size);
	log_info(logger, "Cantidad de bloques a ocupar: [%d] para %s", quantity_of_blocks, metadata_path);
	t_list* blocks = pokemon_blocks(quantity_of_blocks, metadata_path, size_array_positions);

	for(int i=0; i < quantity_of_blocks; i++){
		int first_byte = i*block_size;
		int size_positions_to_write = size_of_content_to_write(i+1, quantity_of_blocks, size_array_positions);

		char* data = string_substring(array_positions, first_byte, size_positions_to_write);
		string_append(&data,"\0");
		char* block = list_get(blocks, i);

		log_info(logger, "Se escriben [%d] bytes de registros, en el bloque: [%s] para %s", size_positions_to_write, block, metadata_path);

		write_positions_on_block(block, data);
		//log_info(logger, "Bloque escrito satisfactoriamente");
		free(data);
	}
	return blocks;
}

int my_ceil(int a, int b){
	int resto = a % b;
	int retorno = a/b;

	return resto==0 ? retorno : (retorno+1);
}


int size_of_content_to_write(int index_next_block, int quantity_total_blocks, int total_content_size){
	int size_of_content = block_size;

	if(index_next_block == quantity_total_blocks){
		size_of_content = total_content_size - ((quantity_total_blocks-1) * block_size);
	}
	return size_of_content;
}

void write_positions_on_block(char* block, char* data){

	char* path_block = string_from_format("%s/Blocks/%s.bin", PUNTO_MONTAJE_TALLGRASS, block);
	FILE* file = fopen(path_block, "wb+");

	if(file == NULL){
		log_error(logger,"error al abrir el archivo %s",path_block);
	} else {
		size_t data_length = string_length(data);
		size_t bit_to_write = sizeof(*data);

		fwrite(data, bit_to_write, data_length, file);
		log_info(logger, "Se escribe el bloque %s en el File System", block);
	}
	sleep(TIEMPO_RETARDO_OPERACION);
	fclose(file);
	free(path_block);
}

char* read_blocks_content(char* path_pokemon){
	t_config* metadata = config_create(path_pokemon);
	int content_size = config_get_int_value(metadata, "SIZE");
	char** blocks = config_get_array_value(metadata, "BLOCKS");

	int rest_to_read = content_size;
	int size_to_read = 0;
	int ind_blocks=0;
	char* buffer = string_new();
	char* blocks_content = string_new();

	if (content_size!=0){
		while(blocks[ind_blocks]!=NULL){
			if (rest_to_read-block_size > 0){
				size_to_read = block_size;
				rest_to_read = rest_to_read-block_size;
			}else{
				size_to_read = rest_to_read;
			}
			int block_num=atoi(blocks[ind_blocks]);

			char* dir_block = string_from_format("%s/Blocks/%i.bin", PUNTO_MONTAJE_TALLGRASS, block_num);
			FILE* file = fopen(dir_block, "rb+");
			buffer = malloc(size_to_read+1);

			fread(buffer, sizeof(char), size_to_read, file);
			buffer[size_to_read] = '\0';
			string_append(&blocks_content, buffer);
			fclose(file);
			free(dir_block);
			free(blocks[ind_blocks]);

			ind_blocks += 1;
		}
	}
	free(blocks);
	free(buffer);
	//log_info(logger, "El contenido de los bloques en %s es %s",path_pokemon, blocks_content);
	config_destroy(metadata);
	return blocks_content;
}

t_list* get_positions_from_buffer(char* buffer){
	t_list* positions = list_create();

	if(string_length(buffer) != 0){
		char **array_buffer_positions = string_split(buffer, "\n");
		int index_position=0;
		while (array_buffer_positions[index_position]!=NULL){
			char** string_position = string_split(array_buffer_positions[index_position], "=");
			char** only_positions = string_split(string_position[0], "-");
			t_position* new_position = create_position(atoi(only_positions[0]), atoi(only_positions[1]), atoi(string_position[1]));

			list_add(positions, new_position);
			free(only_positions[0]);
			free(only_positions[1]);
			free(only_positions);
			free(string_position[0]);
			free(string_position[1]);
			free(string_position);
			free(array_buffer_positions[index_position]);
			index_position += 1;
		}

		free(array_buffer_positions);
		//free(buffer);
	}
	return positions;
}

t_position* create_position(int x, int y, int pokemon_quantity){
	t_position* new_position = malloc(sizeof(t_position));
	new_position->pos_x = x;
	new_position->pos_y = y;
	new_position->count = pokemon_quantity;
	return new_position;
}


void check_if_file_is_open(char* path){
	//ver si esta cerrado
	//si esta cerrado abrirlo y salir
	//si esta abierto esperar y preguntar devuelta hasta que este cerrado, ahí abrirlo y salir

	pthread_mutex_lock(&mutexMetadataPath);
	t_config* metadata = config_create(path);
	char* is_open = config_get_string_value(metadata, "OPEN");

	if(string_equals_ignore_case(is_open, "N")){
		log_info(logger, "El archivo %s esta cerrado",path);
		open_file_to_use(path);
		config_destroy(metadata);
		pthread_mutex_unlock(&mutexMetadataPath);
		return;
	}
	pthread_mutex_unlock(&mutexMetadataPath);

	while(string_equals_ignore_case(is_open, "Y")) {
		log_info(logger,"El archivo %s esta abierto reintentando",path);
		sleep(TIEMPO_DE_REINTENTO_OPERACION);
		pthread_mutex_lock(&mutexMetadataPath);
		metadata = config_create(path);
		is_open = config_get_string_value(metadata, "OPEN");
		if(string_equals_ignore_case(is_open, "N")){
			log_info(logger, "El archivo %s esta cerrado",path);
			open_file_to_use(path);
			config_destroy(metadata);
			pthread_mutex_unlock(&mutexMetadataPath);
			return;
		}
		pthread_mutex_unlock(&mutexMetadataPath);
	}
}

void open_file_to_use(char* path){
	t_config* metadata = config_create(path);
	char* y = "Y";
	config_set_value(metadata, "OPEN", y);
	log_info(logger, "Abrimos el archivo %s para utilizarlo",path);
	config_save(metadata);
	config_destroy(metadata);
}

int check_pokemon_directory(char* pokemon, event_code code){
	char* path_pokemon = string_from_format("%s/Files/%s", PUNTO_MONTAJE_TALLGRASS, pokemon);
	struct stat stats;
	pthread_mutex_lock(&mutexDirectory);
	stat(path_pokemon, &stats);
	int exist = (stat(path_pokemon, &stats) == 0);
	//Si no existe el directorio en ese path, si es NEW_POKEMON lo crea, si es CATCH_POKEMON o GET_POKEMON lo informa
	if(exist == 0){
		if(code == NEW_POKEMON){
			log_info(logger, "Se crea el nuevo directorio con su metadata para %s", pokemon);
			mkdir(path_pokemon, 0777);
			string_append(&path_pokemon, "/");
			string_append(&path_pokemon, "Metadata.bin");

			int available_block = get_available_block();

			FILE* file = fopen(path_pokemon, "wb+");
			fprintf(file, "DIRECTORY=N\n");
			fprintf(file, "SIZE=0\n");
			fprintf(file, "BLOCKS=[%d]\n", available_block);
			fprintf(file, "OPEN=N\n");
			fclose(file);
		} else if(code == CATCH_POKEMON || code == GET_POKEMON){
			log_error(logger, "No existe el directorio para %s", pokemon);
		}
	}
	pthread_mutex_unlock(&mutexDirectory);
	free(path_pokemon);
	return exist;
}

int get_available_block(){
	pthread_mutex_lock(&mutexBitmap);
	bool is_not_available=true;
	int block=0;
	while(is_not_available){
		is_not_available = bitarray_test_bit(bitarray, block);
		block ++;
	}
	block -= 1;
	bitarray_set_bit(bitarray, block);
	blocks_available -= 1;
	log_info(logger, "Cantidad de bloques disponibles: %d", blocks_available);
	log_info(logger, "Se ocupa bloque %d", block);
	msync(bmap, sizeof(bitarray), MS_SYNC);
	pthread_mutex_unlock(&mutexBitmap);
	return block;
}

void tall_grass_metadata_info(){

	char* path_metadata = string_from_format("%s/Metadata/Metadata", PUNTO_MONTAJE_TALLGRASS);
	t_config* metadata  = config_create(path_metadata);
	block_size = config_get_int_value(metadata, "BLOCK_SIZE");
	blocks = config_get_int_value(metadata, "BLOCKS");
	log_info(logger, "Existe el File System en %s con %d bloques de tamaño %d", PUNTO_MONTAJE_TALLGRASS, blocks, block_size);
	config_destroy(metadata);
	free(path_metadata);
}

void open_bitmap(){

	char* nombre_bitmap = string_from_format("%s/Metadata/Bitmap.bin", PUNTO_MONTAJE_TALLGRASS);

	int fd = open(nombre_bitmap, O_RDWR);
    if (fstat(fd, &size_bitmap) < 0) {
        printf("Error al establecer fstat\n");
        close(fd);
    }

    blocks_available=0;
    bmap = mmap(NULL, size_bitmap.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,	fd, 0);
	bitarray = bitarray_create_with_mode(bmap, blocks/8, MSB_FIRST);
	size_t len_bitarray = bitarray_get_max_bit(bitarray);
	for(int i=0 ; i < len_bitarray;i++){
		 if(bitarray_test_bit(bitarray, i) == 0){
			 blocks_available++;
		 }
	}
	log_info(logger, "Cantidad de bloques disponibles: %d", blocks_available);
	free(nombre_bitmap);
}
