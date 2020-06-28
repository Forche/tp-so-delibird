/*
 * gamecard.h
 *
 *  Created on: 16 jun. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_H_
#define GAMECARD_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/log.h>
#include <operavirus-commons.h>
#include <semaphore.h>

// ******* DEFINICION DE VARIABLES A UTILIZAR ******* //

sem_t kkkk;

t_log* logger;

t_config* config;
int TIEMPO_DE_REINTENTO_CONEXION;
int TIEMPO_DE_REINTENTO_OPERACION;
int TIEMPO_RETARDO_OPERACION;
char* PUNTO_MONTAJE_TALLGRASS;
char* IP_BROKER;
char* PUERTO_BROKER;
char* IP_GAMECARD;
char* PUERTO_GAMECARD;


// ******* DEFINICION DE FUNCIONES A UTILIZAR ******* //
void read_config();
void init_loggers();

void subscribe_to(event_code code);
void create_listener_thread();
void gamecard_listener();
void create_subscription_threads();
void handle_event(uint32_t* socket);

void shutdown_gamecard();

#endif /* GAMECARD_H_ */
