#ifndef GAMEBOY_H_
#define GAMEBOY_H_

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

pthread_t thread;
int still_connected;
t_log* logger;
char* ID;
char* BROKER_IP;
char* BROKER_PORT;
void get_payload_content(int argc, char* argv[], char* payload_content[], uint32_t is_new_subscriptor);
uint32_t validate_arg_count(uint32_t arg_count, uint32_t args);
int process_message(uint32_t* socket);
void wait_for_messages(uint32_t socket);
void received_messages(uint32_t* socket);
void send_message_received_to_broker(t_message_received* message_received, uint32_t id, uint32_t correlative_id);

#endif
