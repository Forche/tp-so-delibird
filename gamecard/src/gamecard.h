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

// ******* DEFINICION DE FUNCIONES A UTILIZAR ******* //
void read_config();
void init_loggers();

void subscribe_to(event_code code);
void create_listener_thread();
void gamecard_listener();
void create_subscription_threads();
void handle_event(uint32_t* socket);
void handle_new_pokemon(t_message* msg);

void check_pokemon_directory(char* pokemon);
void check_if_file_is_open(char* path);
int get_available_block();
char* read_blocks_content(char* path_pokemon);
t_list* get_positions_from_buffer(char* buffer);
t_position* create_position(int x, int y, int pokemon_quantity);

void shutdown_gamecard();

#endif /* GAMECARD_H_ */
