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

#define IP "127.0.0.1"
#define PORT "7777"


pthread_t thread;

void server_init(void);
void wait_for_client(uint32_t);
t_buffer* receive_new_pokemon_message(uint32_t client_socket);
void process_request(uint32_t event_code, uint32_t client_fd);
void serve_client(uint32_t *socket);
void return_message(void* payload, uint32_t size, uint32_t client_socket);

#endif
