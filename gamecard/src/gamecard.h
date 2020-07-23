/*
 * gamecard.h
 *
 *  Created on: 16 jun. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_H_
#define GAMECARD_H_

#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <fcntl.h>
#include <operavirus-commons.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread.h>


// ******* DEFINICION DE ESTRUCTURAS A UTILIZAR ******* //

typedef struct {
	int pos_x;
	int pos_y;
	int count;
} t_position;

// ******* DEFINICION DE VARIABLES A UTILIZAR ******* //

t_log* logger;

char* bmap;
struct stat size_bitmap;
t_bitarray* bitarray;

t_config* config;
int TIEMPO_DE_REINTENTO_CONEXION;
int TIEMPO_DE_REINTENTO_OPERACION;
int TIEMPO_RETARDO_OPERACION;
char* PUNTO_MONTAJE_TALLGRASS;
char* IP_BROKER;
char* PUERTO_BROKER;
char* IP_GAMECARD;
char* PUERTO_GAMECARD;

int block_size, blocks, blocks_available;

pthread_mutex_t mutexBitmap;
pthread_mutex_t mutexMetadata;

// ******* DEFINICION DE FUNCIONES A UTILIZAR ******* //
void read_config();
void init_loggers();

void subscribe_to(event_code code);
void create_listener_thread();
void gamecard_listener();
void create_subscription_threads();

void handle_event(uint32_t* socket);
void handle_new_pokemon(t_message* msg);
void handle_catch_pokemon(t_message* msg);
void handle_get_pokemon(t_message* msg);

void add_new_pokemon(char* path_pokemon, t_new_pokemon* pokemon);
t_position* ckeck_position_exists(char* path_pokemon, t_catch_pokemon* pokemon);
void remove_position(t_position* position, char* path_pokemon);
void decrease_position(t_position* position, char* path_pokemon);

void remove_block_from_bitmap(char* block);
char* remove_last_block_from_array(char* blocks_as_array);

int check_pokemon_directory(char* pokemon, event_code code);
void check_if_file_is_open(char* path);

int get_available_block();
char* read_blocks_content(char* path_pokemon);
t_list* get_positions_from_buffer(char* buffer);
t_position* create_position(int x, int y, int pokemon_quantity);
void destroy_position(t_position* position);
t_list* write_blocks_and_metadata(int size_array_positions, char* array_positions, char* metadata_path);
char** metadata_blocks_to_actual_blocks(char* metadata_blocks);
char* add_block_to_array(char* blocks_as_array, char* block_to_add);
int my_ceil(int a, int b);
int size_of_content_to_write(int index_next_block, int quantity_total_blocks, int total_content_size);
void write_positions_on_block(char* block, char* data);
t_list* pokemon_blocks(int blocks_needed, char* metadata_path, int size_array_positions);
int position_list(t_list* pokemon_positions, t_position* find_position);
uint32_t* positions_to_uint32(t_list* pokemon_positions);
void close_file_pokemon(char* path_pokemon);

t_appeared_pokemon* create_appeared_pokemon(uint32_t pokemon_len, char* pokemon, uint32_t pos_x, uint32_t pos_y);
t_localized_pokemon* create_localized_pokemon(uint32_t pokemon_len,	char* pokemon, uint32_t positions_count, uint32_t* positions);

void send_appeared_to_broker(t_appeared_pokemon* appeared_pokemon, uint32_t id);
void send_caught_to_broker(t_caught_pokemon* caught_pokemon, uint32_t id);
void send_localized_to_broker(t_localized_pokemon* localized_pokemon, uint32_t id);

void tall_grass_metadata_info();
void open_bitmap();
void write_positions_on_files(t_list* pokemon_positions, char* path_new_file);

void shutdown_gamecard();

#endif /* GAMECARD_H_ */
