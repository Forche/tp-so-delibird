#ifndef OPERAVIRUS_COMMONS_H_
#define OPERAVIRUS_COMMONS_H_

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/string.h>
#include<commons/log.h>
#include<commons/collections/dictionary.h>

typedef enum {
	NEW_POKEMON = 1,
	APPEARED_POKEMON = 2,
	CATCH_POKEMON = 3,
	CAUGHT_POKEMON = 4,
	GET_POKEMON = 5,
	LOCALIZED_POKEMON = 6,
	NEW_SUBSCRIPTOR = 7,
	MESSAGE_RECEIVED = 8
} event_code;

typedef struct {
	uint32_t size;
	void* payload;
} t_buffer;

typedef struct {
	event_code event_code;
	uint32_t id;
	uint32_t correlative_id;
	t_buffer* buffer;
} t_message;

typedef struct {
	event_code event_code;
	uint32_t id;
	uint32_t correlative_id;
	uint32_t memory_partition_id;
} t_memory_message;

typedef struct {
	uint32_t pokemon_len;
	char* pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t count;
} t_new_pokemon;

typedef struct {
	uint32_t pokemon_len;
	char* pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
} t_appeared_pokemon;

typedef struct {
	uint32_t pokemon_len;
	char* pokemon;
	uint32_t pos_x;
	uint32_t pos_y;
} t_catch_pokemon;

typedef struct {
	uint32_t result;
} t_caught_pokemon;

typedef struct {
	uint32_t pokemon_len;
	char* pokemon;
} t_get_pokemon;

typedef struct {
	uint32_t pokemon_len;
	char* pokemon;
	uint32_t positions_count;
	uint32_t* positions;
} t_localized_pokemon;

typedef struct {
	uint32_t subscriptor_len;
	char* subscriptor_id;
	event_code queue;
	uint32_t ip_len;
	char* ip;
	uint32_t port;
	uint32_t duration; // If duration is -1, it's for an undefined amount of time. Otherwise, 'duration' seconds.
} t_subscription_petition;

typedef struct {
	uint32_t subscriptor_len;
	char* subscriptor_id;
	event_code message_type;
	uint32_t received_message_id;
} t_message_received;

typedef struct {
	t_subscription_petition* subscriptor_info;
	uint32_t socket;
} t_subscriptor;

typedef struct {
	pthread_t thread;
	t_subscriptor subscriptor;
} t_thread;

typedef enum
{
	NEW = 1,
	READY = 2,
	EXEC = 3,
	BLOCK = 4,
	EXIT = 5,
	FULL = 6,
	SWAPPING = 7,
	EXEC_DEADLOCK = 8,
	CATCHING = 9,
} status;

typedef struct {
    t_appeared_pokemon* pokemon_to_catch;
    uint32_t result_catch;
    uint32_t id_message;
    pthread_mutex_t sem_caught;
    pthread_mutex_t sem_deadlock;
    status status;
    uint32_t msg_connection;
    void (*do_next)(void*);
    void* params_do_next;
    uint32_t quantum;
} t_pcb_trainer;

typedef struct {
       int name;
       uint32_t pos_x;
       uint32_t pos_y;
       t_dictionary* objective;
       t_dictionary* caught;
       status status;
       pthread_t thread;
       pthread_mutex_t sem;
       t_pcb_trainer* pcb_trainer;
       bool sjf_calculado;
       double estimacion_actual;
       double estimacion_anterior;
       double real_anterior;
       uint32_t q_ciclos_cpu;
} t_trainer;

uint32_t connect_to(char* ip, char* port);

t_subscription_petition* build_new_subscription(event_code code, char* my_ip, char* id, uint32_t my_port);
void make_subscription_to(t_subscription_petition* suscription, char* broker_ip,
		char* broker_port, uint32_t reconnect_time, t_log* logger, void handle_event(uint32_t*));

void send_message(uint32_t client_socket, event_code event_code, uint32_t id,
		uint32_t correlative_id, t_buffer* buffer);
t_buffer* serialize_buffer(event_code event_code, uint32_t arg_count,
		char* payload_content[], char* sender_id, char* sender_ip,
		uint32_t sender_port);
void* serialize_message(t_message* message, uint32_t* bytes_to_send);

t_buffer* serialize_t_new_pokemon_message(t_new_pokemon* new_pokemon);
t_buffer* serialize_new_pokemon_message(char* payload_content[]);

t_buffer* serialize_t_appeared_pokemon_message(t_appeared_pokemon* appeared_pokemon);
t_buffer* serialize_appeared_pokemon_message(char* payload_content[]);

t_buffer* serialize_t_catch_pokemon_message(t_catch_pokemon* catch_pokemon);
t_buffer* serialize_catch_pokemon_message(char* payload_content[]);

t_buffer* serialize_t_caught_pokemon_message(t_caught_pokemon* caught_pokemon);
t_buffer* serialize_caught_pokemon_message(char* payload_content[]);

t_buffer* serialize_t_get_pokemon_message(t_get_pokemon* get_pokemon);
t_buffer* serialize_get_pokemon_message(char* payload_content[]);

t_buffer* serialize_t_localized_pokemon_message(t_localized_pokemon* get_pokemon);
t_buffer* serialize_localized_pokemon_message(char* payload_content[]);

t_buffer* serialize_t_new_subscriptor_message(t_subscription_petition* subscription_petition);
t_buffer* serialize_new_subscriptor_message(char* payload_content[], char* sender_id, char* sender_ip, uint32_t sender_port);

t_buffer* serialize_t_message_received_message(t_message_received* message_received);
t_buffer* serialize_message_received_message(char* payload_content[], char* sender_id, char* sender_ip, uint32_t received_message_id);

t_buffer* serialize_t_message_received(t_message_received* message_received);

t_message* receive_message(uint32_t event_code, uint32_t socket);
t_message* deserialize_message(event_code event_code, t_buffer* buffer);
void return_message_id(uint32_t client_socket, uint32_t id);

t_new_pokemon* deserialize_new_pokemon_message(uint32_t socket, uint32_t* size);
t_appeared_pokemon* deserialize_appeared_pokemon_message(uint32_t socket,
		uint32_t* size);
t_catch_pokemon* deserialize_catch_pokemon_message(uint32_t socket,
		uint32_t* size);
t_caught_pokemon* deserialize_caught_pokemon_message(uint32_t socket,
		uint32_t* size);
t_get_pokemon* deserialize_get_pokemon_message(uint32_t socket, uint32_t* size);
t_localized_pokemon* deserialize_localized_pokemon_message(uint32_t socket,
		uint32_t* size);

void delete_message(t_message* message);
void free_connection(uint32_t client_socket);

event_code string_to_event_code(char* code);
t_log* logger_init();

#endif
