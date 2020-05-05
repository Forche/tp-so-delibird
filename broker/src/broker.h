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
	receiver receivers[];
} queue_message;

typedef struct
{
	queue_message* messages[];
	uint32_t subscriptor_sockets[];
} queue;

typedef struct
{
	uint32_t id;
	answered_message* next_message;
} answered_message;


pthread_t thread;

void server_init(void);
void wait_for_client(uint32_t);
void process_request(uint32_t event_code, uint32_t client_socket);
void serve_client(uint32_t* socket);
void return_message(void* payload, uint32_t size, uint32_t client_socket);

#endif
