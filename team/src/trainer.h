/*
 * trainer.h
 *
 *  Created on: 15 may. 2020
 *      Author: utnso
 */

#ifndef SRC_TRAINER_H_
#define SRC_TRAINER_H_

#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<commons/collections/dictionary.h>
#include<commons/string.h>
#include<operavirus-commons.h>

typedef enum
{
	NEW = 1,
	READY = 2,
	EXEC = 3,
	BLOCK = 4,
	EXIT = 5,
} status;

typedef struct {
	uint32_t pos_x;
	uint32_t pos_y;
	t_dictionary* objective;
	t_dictionary* caught;
	status status;
	pthread_t thread;
	pthread_mutex_t sem;
} t_trainer;


void handle_trainer(t_trainer* trainer);

#endif /* SRC_TRAINER_H_ */
