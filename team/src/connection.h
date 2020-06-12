#ifndef SRC_CONNECTION_H_
#define SRC_CONNECTION_H_

#include<stdlib.h>
#include<stdio.h>
#include<operavirus-commons.h>
#include<commons/config.h>

void listener(char* ip, char* port, void* handle_event);
void wait_for_message(uint32_t sv_socket, void* handle_appeared_pokemon);
void serve_client(uint32_t* socket);

int* server_socket;

#endif /* SRC_CONNECTION_H_ */
