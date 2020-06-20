#include "thread.h";

pthread_t create_thread(void* function_thread, char* name) {
    pthread_t thread;
    if (pthread_create(&thread, 0, function_thread, NULL) !=0) {
        log_error(logger, strcat("Error al crear el hilo:", name));
    }
    if (pthread_detach(thread) != 0) {
        log_error(logger, strcat("Error al detachar el hilo:", name));
    }
    return thread;
}
pthread_t create_thread_with_param(void* function_thread, void* param, char* name) {
    pthread_t thread;
    if (pthread_create(&thread, 0, function_thread, param) !=0) {
        log_error(logger, strcat("Error al crear el hilo:", name));
    }
    if (pthread_detach(thread) != 0) {
        log_error(logger, strcat("Error al detachar el hilo:", name));
    }
    return thread;
}

void set_logger_thread(t_log* log) {
	logger = log;
}

