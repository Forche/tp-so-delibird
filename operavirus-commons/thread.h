/*
 * thread.h
 *
 *  Created on: 19 jun. 2020
 *      Author: utnso
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <stdio.h>
#include<commons/log.h>
#include<pthread.h>

t_log* logger;

pthread_t create_thread(void* function_thread, char* name);
pthread_t create_thread_with_param(void* function_thread, void* param, char* name);
void set_logger_thread(t_log* log);

#endif /* THREAD_H_ */
