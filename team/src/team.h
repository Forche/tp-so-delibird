#ifndef TEAM_H_
#define TEAM_H_

#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<operavirus-commons.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include "planner.h"
#include "matcher.h"
#include "trainer.h"
#include "util.h"
#include "connection.h"

typedef struct {
	uint32_t quantity;
	uint32_t pokemon_len;
	char* pokemon;
} t_quantity_pokemon;

t_list* pokemon_received_to_catch;
t_list* trainers;
t_list* matches;

pthread_mutex_t mutex_pokemon_received_to_catch;
pthread_mutex_t mutex_trainers;
pthread_mutex_t mutex_matches;
pthread_mutex_t mutex_planning;
pthread_mutex_t mutex_receiving_id;

sem_t sem_appeared_pokemon;
sem_t sem_trainer_available;
sem_t sem_count_matches;

t_log* logger;
uint32_t broker_connection;
t_config* config;
char* IP_TEAM;
char* PUERTO_TEAM;
char* IP_BROKER;
char* PUERTO_BROKER;

t_dictionary* global_objective;

t_dictionary* build_global_objective(char* objectives);
void create_trainers(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES);
void create_planner();
void create_matcher();
void init_sem();
void handle_event(uint32_t* socket);
void suscribe_to(event_code code);
void team_listener();
void create_thread_open_socket();
void send_get_pokemons();
void get_pokemon(char* pokemon, uint32_t* cant);

#endif /* TEAM_H_ */
