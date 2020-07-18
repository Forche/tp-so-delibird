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
#include "resolver.h"

typedef struct {
	uint32_t quantity;
	uint32_t pokemon_len;
	char* pokemon;
} t_quantity_pokemon;

typedef struct {
	t_trainer* trainer1;
	char* pokemon1; //Pokemon que da el trainer1 y recibe el trainer2
	t_trainer* trainer2;
	char* pokemon2; //Pokemon que da el trainer2 y recibe el trainer1
} t_deadlock_matcher;

t_list* matched_deadlocks;

t_list* pokemon_received_to_catch;
t_list* trainers;
t_list* matches;
t_list* queue_deadlock;

pthread_mutex_t mutex_pokemon_received_to_catch;
pthread_mutex_t mutex_trainers;
pthread_mutex_t mutex_matches;
pthread_mutex_t mutex_planning;
pthread_mutex_t mutex_remaining_pokemons;
pthread_mutex_t mutex_caught_pokemons;
pthread_mutex_t mutex_being_caught_pokemons;
pthread_mutex_t mutex_queue_deadlocks;
pthread_mutex_t mutex_planning_deadlock;
pthread_mutex_t mutex_direct_deadlocks;
pthread_mutex_t mutex_matched_deadlocks;
pthread_mutex_t mutex_q_ciclos_cpu_totales;

sem_t sem_appeared_pokemon;
sem_t sem_trainer_available;
sem_t sem_count_matches;
sem_t sem_all_pokemons_caught;
sem_t sem_count_queue_deadlocks;
uint32_t count_sem_reverse_direct_deadlock;

t_log* logger;
t_config* config;
char* IP_TEAM;
char* PUERTO_TEAM;
char* IP_BROKER;
char* PUERTO_BROKER;
uint32_t TIEMPO_RECONEXION;
uint32_t RETARDO_CICLO_CPU;
double ALPHA;
double ESTIMACION_INICIAL;
char* ALGORITMO_PLANIFICACION;
uint32_t QUANTUM;
uint32_t q_ciclos_cpu_totales;
uint32_t q_cambios_contexto_totales;

t_dictionary* global_objective;
t_dictionary* caught_pokemons;
t_dictionary* being_caught_pokemons;
t_dictionary* remaining_pokemons;

t_dictionary* build_global_objective(char* objectives);
void create_trainers(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES);
pthread_t create_planner();
pthread_t create_matcher();
void init_sem();
void handle_event(uint32_t* socket);
void subscribe_to(event_code code);
void team_listener();
pthread_t create_thread_open_socket();
void send_get_pokemons();
void get_pokemon(char* pokemon, uint32_t* cant);
void handle_localized(t_message* msg);
void handle_caught(t_message* msg);
void handle_appeared(t_message* msg);
void send_get(char* pokemon);
void* build_remaining_pokemons();
void* add_remaining(char* pokemon, uint32_t* cant_global);
void print_pokemons(char* key, int* value);
void swap_pokemons(t_deadlock_matcher* deadlock_matcher);
void validate_state_trainer(t_trainer* trainer);
void increment_q_ciclos_cpu(t_trainer* trainer);

#endif /* TEAM_H_ */
