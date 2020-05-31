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

void get_payload_content(int argc, char* argv[], char* payload_content[], uint32_t is_new_subscriptor);
void process_message(uint32_t* socket);
void wait_for_messages(uint32_t socket);

#endif
