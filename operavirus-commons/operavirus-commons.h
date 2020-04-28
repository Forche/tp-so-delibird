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

typedef enum
{
	NEW_POKEMON = 1,
	APPEARED_POKEMON = 2,
	CATCH_POKEMON = 3,
	CAUGHT_POKEMON = 4,
	GET_POKEMON = 5,
	LOCALIZED_POKEMON = 6
} event_code;

typedef struct
{
	uint32_t size;
	void* payload;
} t_buffer;

typedef struct
{
	event_code event_code;
	uint32_t id;
	uint32_t correlative_id;
	t_buffer* buffer;
} t_message;

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


int connect_to(char* ip, char* port);

void send_message(uint32_t client_socket, event_code event_code, uint32_t correlative_id, t_buffer* buffer);
t_buffer* serialize_buffer(event_code event_code, uint32_t arg_count, char* payload_content[]);

t_new_pokemon* serialize_new_pokemon_message(char* payload_content[]);
t_appeared_pokemon* serialize_appeared_pokemon_message(char* payload_content[]);
t_catch_pokemon* serialize_catch_pokemon_message(char* payload_content[]);
t_caught_pokemon* serialize_caught_pokemon_message(char* payload_content[]);
t_get_pokemon* serialize_get_pokemon_message(char* payload_content[]);
t_localized_pokemon* serialize_localized_pokemon_message(char* payload_content[]);

t_message* receive_message(uint32_t client_socket);
t_message* deserialize_message(event_code event_code, t_buffer* buffer);

t_new_pokemon* deserialize_new_pokemon_message(t_buffer* buffer);
t_appeared_pokemon* deserialize_appeared_pokemon_message(t_buffer* buffer);
t_catch_pokemon* deserialize_catch_pokemon_message(t_buffer* buffer);
t_caught_pokemon* deserialize_caught_pokemon_message(t_buffer* buffer);
t_get_pokemon* deserialize_get_pokemon_message(t_buffer* buffer);
t_localized_pokemon* deserialize_localized_pokemon_message(t_buffer* buffer);

void delete_message(t_message* message);
void free_connection(uint32_t client_socket);

#endif
