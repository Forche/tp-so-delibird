#ifndef BROKER_H_
#define BROKER_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/string.h>
#include<pthread.h>
#include<operavirus-commons.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<pthread.h>
#include<signal.h>
#include<commons/config.h>

typedef struct
{
	uint32_t receiver_socket;
	uint32_t received; // 1: received, 0: not received
} receiver;

typedef struct
{
	t_message* message;
	t_list* receivers; // Type: receiver*
} queue_message;

typedef struct
{
	t_list* messages; // Type: queue_message*
	t_list* subscriptors; // Type: t_subscriptor*
} queue;


pthread_t thread;
uint32_t message_count;
t_list* answered_messages;
t_list* threads;
queue queue_new_pokemon;
queue queue_appeared_pokemon;
queue queue_catch_pokemon;
queue queue_caught_pokemon;
queue queue_get_pokemon;
queue queue_localized_pokemon;

void server_init(void);
void queues_init();
void wait_for_client(uint32_t);
void process_request(uint32_t event_code, uint32_t socket);
void process_new_subscription(uint32_t client_socket);
void serve_client(uint32_t* socket);
void return_message(void* payload, uint32_t size, uint32_t client_socket);
void process_subscriptor(uint32_t* socket);

uint32_t get_message_id();

#endif
