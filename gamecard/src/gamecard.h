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


// ******* DEFINICION DE VARIABLES A UTILIZAR ******* //

t_log* logger;

t_config* config;
int TIEMPO_DE_REINTENTO_CONEXION;
int TIEMPO_DE_REINTENTO_OPERACION;
int TIEMPO_RETARDO_OPERACION;
char* PUNTO_MONTAJE_TALLGRASS;
char* IP_BROKER;
char* PUERTO_BROKER;


// ******* DEFINICION DE FUNCIONES A UTILIZAR ******* //
void read_config();
void init_loggers();

void shutdown_gamecard();

#endif /* GAMECARD_H_ */
