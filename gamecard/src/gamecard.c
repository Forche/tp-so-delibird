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

	sem_t sem_gameCard_up;
	sem_init(&sem_gameCard_up, 0, 0);
	pthread_mutex_init(&mutexBitmap,NULL);

	create_listener_thread();
	create_subscription_threads();

	sem_wait(&sem_gameCard_up);

	shutdown_gamecard();

	return EXIT_SUCCESS;
}

void read_config() {
	config = config_create("/home/utnso/workspace/tp-2020-1c-Operavirus/gamecard/gamecard.config");

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
	logger = log_create("/home/utnso/workspace/tp-2020-1c-Operavirus/gamecard/gamecard.log", "gamecard", true, LOG_LEVEL_INFO); //true porque escribe tambien la consola
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
	t_subscription_petition* new_subscription = build_new_subscription(code, IP_GAMECARD, "Game Card", PUERTO_GAMECARD);
	make_subscription_to(new_subscription, IP_BROKER, PUERTO_BROKER, TIEMPO_DE_REINTENTO_CONEXION, logger, handle_event);
}


void create_listener_thread() {
	create_thread(gamecard_listener, "events_listener");
}

void gamecard_listener() {
	listener(IP_GAMECARD, PUERTO_GAMECARD, handle_event);
}


void handle_event(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;
	t_message* msg = receive_message(code, *socket);
	uint32_t size;
	switch (code) {
	case NEW_POKEMON:
		msg->buffer = deserialize_new_pokemon_message(*socket, &size);
		create_thread_with_param(handle_new_pokemon, msg, "handle_new_pokemon");

		break;
	case CATCH_POKEMON:
		msg->buffer = deserialize_catch_pokemon_message(*socket, &size);
		//create_thread_with_param(handle_caught, msg, "handle_caught");

		break;
	case GET_POKEMON:
		msg->buffer = deserialize_get_pokemon_message(*socket, &size);
		//create_thread_with_param(handle_appeared, msg, "handle_appeared");
		break;
	}

}

void handle_new_pokemon(t_message* msg){
	t_new_pokemon* new_pokemon = (t_new_pokemon*) msg->buffer;

	check_pokemon_directory(new_pokemon->pokemon);

	char* path_pokemon = string_from_format("%s/Files/%s/Metadata.bin", PUNTO_MONTAJE_TALLGRASS, new_pokemon->pokemon);

	check_if_file_is_open(path_pokemon);
	add_new_pokemon(path_pokemon, new_pokemon);

}

void add_new_pokemon(char* path_pokemon, t_new_pokemon* pokemon){
	char* blocks_content = read_blocks_content(path_pokemon);
	t_list* pokemon_positions = get_positions_from_buffer(blocks_content);

	bool _compare(t_position* position){
		return pokemon->pos_x == position->pos_x && pokemon->pos_y == position->pos_y;
	}
	t_position* find_position = list_find(pokemon_positions, _compare);

	if( find_position != NULL){
		find_position->count += pokemon->count;
	}else{
		t_position* new_position = create_position(pokemon->pos_x, pokemon->pos_y, pokemon->count);
		list_add(pokemon_positions, new_position);
	}


}

char* read_blocks_content(char* path_pokemon){
	t_config* metadata = config_create(path_pokemon);
	int content_size = config_get_int_value(metadata, "SIZE");
	char** blocks = config_get_array_value(metadata, "BLOCKS");

	char* n = "N";
	dictionary_put(metadata->properties, "OPEN", n);
	config_save(metadata);

	int rest_to_read = content_size;
	int size_to_read;
	int ind_blocks=0;
	char* buffer;
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

			char* dir_block = string_from_format("%s/Bloques/%i.bin", PUNTO_MONTAJE_TALLGRASS, block_num);
			FILE* file = fopen(dir_block, "rb+");
			buffer = malloc(size_to_read +1);

			size_t read_count = fread(buffer, sizeof(char), size_to_read, file);
			buffer[read_count]='\0';
			string_append(&blocks_content, buffer);
			fclose(file);
			free(buffer);
			free(dir_block);
			free(blocks[ind_blocks]);

			ind_blocks += 1;
		}
	}
	free(blocks);
	config_destroy(metadata);
	return blocks_content;
}

t_list* get_positions_from_buffer(char* buffer){
	t_list* positions = list_create();
	char **array_buffer_positions = string_split(buffer, "\n");
	int index_position=0;
	while (array_buffer_positions[index_position]!=NULL){
		char** string_position = string_split(array_buffer_positions[index_position], "=");
		char** only_positions = string_split(string_position[0], "-");
		int quantity = atoi(string_position[1]);
		t_position* new_position = create_position(atoi(only_positions[0]), atoi(only_positions[1]), quantity);

		list_add(positions, new_position);
		free(only_positions[0]);
		free(only_positions[1]);
		free(string_position[0]);
		free(string_position[1]);
		free(array_buffer_positions[index_position]);
		index_position += 1;


	}
	free(array_buffer_positions);
	free(buffer);
	return positions;
}

t_position* create_position(int x, int y, int pokemon_quantity){
	t_position* new_position = malloc( sizeof(t_position) );
	new_position->pos_x = x;
	new_position->pos_y = y;
	new_position->count = pokemon_quantity;
	return new_position;
}


void check_if_file_is_open(char* path){
	t_config* metadata = config_create(path);
	char* is_open = (char*) config_get_string_value(metadata, "OPEN");

	if(string_equals_ignore_case(is_open, "Y")){
		sleep(TIEMPO_DE_REINTENTO_OPERACION);
		check_if_file_is_open(path);
	}
	config_destroy(metadata);
}

void check_pokemon_directory(char* pokemon){
	char* path_pokemon = string_from_format("%s/Files/%s", PUNTO_MONTAJE_TALLGRASS, pokemon);
	struct stat stats;
	stat(path_pokemon, &stats);

	//Si no existe el directorio en ese path, lo crea
	if(! S_ISDIR(stats.st_mode)){
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

		free(path_pokemon);
	}
}

int get_available_block(){
	pthread_mutex_lock(&mutexBitmap);
	bool is_not_available=true;
	int block=0;
	while(is_not_available){
		is_not_available = bitarray_test_bit(bitarray, block);
		block += 1;
	}
	block -= 1;
	bitarray_set_bit(bitarray, block);
	blocks_available -= 1;
	//log_info(logger, "Cantidad de bloques disponibles: %d", blocks_available);
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
	for(int i=0;i< bitarray_get_max_bit(bitarray);i++){
		 if(bitarray_test_bit(bitarray, i) == 0){
			 blocks_available = blocks_available+1;
		 }
	}
	log_info(logger, "Cantidad de bloques disponibles: %d", blocks_available);
	free(nombre_bitmap);
}
