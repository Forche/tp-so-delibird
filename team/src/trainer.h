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
#include<commons/string.h>
#include<operavirus-commons.h>
#include"team.h"

void handle_trainer(t_trainer* trainer);
void change_status_to(t_trainer* trainer, status status);
void move_to_position(t_trainer* trainer, uint32_t pos_x, uint32_t pos_y);
void catch_pokemon(t_trainer* trainer, t_appeared_pokemon* appeared_pokemon);

#endif /* SRC_TRAINER_H_ */
