/*
 * create_tall_grass.h
 *
 *  Created on: 28 jun. 2020
 *      Author: utnso
 */

#ifndef CREATE_TALL_GRASS_H_
#define CREATE_TALL_GRASS_H_

#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>


// ******* DEFINICION DE VARIABLES A UTILIZAR ******* //

t_config* config;
char* PUNTO_MONTAJE;

struct stat mystat;


// ******* DEFINICION DE FUNCIONES A UTILIZAR ******* //

void read_config();
void create_bitmap(char* path_metadata, int blocks_quantity, int blocks_size);

#endif /* CREATE_TALL_GRASS_H_ */
