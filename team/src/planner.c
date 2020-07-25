#include "planner.h"

t_match_pokemon_trainer* get_next_to_exec();
t_match_pokemon_trainer* planner_rr();
t_match_pokemon_trainer* planner_fifo();
t_match_pokemon_trainer* planner_sjf();
t_deadlock_matcher* planner_deadlock_rr();
t_deadlock_matcher* planner_deadlock_fifo();
t_deadlock_matcher* planner_deadlock_sjf();

void* planning_catch() {


	t_match_pokemon_trainer* (*get_next_to_exec)();

	if(string_equals_ignore_case(ALGORITMO_PLANIFICACION,"FIFO")) {
		get_next_to_exec = &planner_fifo;
	} else if (string_equals_ignore_case(ALGORITMO_PLANIFICACION,"RR")) {
		get_next_to_exec = &planner_rr;
	} else if (string_equals_ignore_case(ALGORITMO_PLANIFICACION,"SJF-CD") || string_equals_ignore_case(ALGORITMO_PLANIFICACION,"SJF-SD") ) {
		get_next_to_exec = &planner_sjf;
	}

	while(1) {
		sem_wait(&sem_count_matches);
		pthread_mutex_lock(&mutex_planning);
		pthread_mutex_lock(&mutex_matches);
		t_match_pokemon_trainer* match_pokemon_trainer = get_next_to_exec();
		pthread_mutex_unlock(&mutex_matches);
		q_cambios_contexto_totales = q_cambios_contexto_totales + 1;
		t_trainer* trainer = match_pokemon_trainer->closest_trainer;
		t_appeared_pokemon* pokemon = match_pokemon_trainer->closest_pokemon;
		trainer->pcb_trainer->pokemon_to_catch = pokemon;
		trainer->status = EXEC;
		log_info(logger, "Planificando por %s, entrenador %d en x: %d y: %d, a atrapar pokemon %s en x: %d y: %d", ALGORITMO_PLANIFICACION, trainer->name, trainer->pos_x,
				trainer->pos_y, pokemon->pokemon, pokemon->pos_x, pokemon->pos_y);
		pthread_mutex_unlock(&match_pokemon_trainer->closest_trainer->sem);
	}
}

void* planning_deadlock() {

	t_deadlock_matcher* (*get_next_to_exec)();

	if(string_equals_ignore_case(ALGORITMO_PLANIFICACION,"FIFO")) {
		get_next_to_exec = &planner_deadlock_fifo;
	} else if (string_equals_ignore_case(ALGORITMO_PLANIFICACION,"RR")) {
		get_next_to_exec = &planner_deadlock_rr;
	} else if (string_equals_ignore_case(ALGORITMO_PLANIFICACION,"SJF-CD") || string_equals_ignore_case(ALGORITMO_PLANIFICACION,"SJF-SD") ) {
		get_next_to_exec = &planner_deadlock_sjf;
	}


	while(1) {
		sem_wait(&sem_count_queue_deadlocks);
		pthread_mutex_lock(&mutex_planning_deadlock);
		pthread_mutex_lock(&mutex_queue_deadlocks);
		t_deadlock_matcher* deadlock_matcher= list_remove(queue_deadlock, 0);
		pthread_mutex_unlock(&mutex_queue_deadlocks);
		q_cambios_contexto_totales = q_cambios_contexto_totales + 1;
		deadlock_matcher->trainer1->pcb_trainer->do_next = &swap_pokemons;
		deadlock_matcher->trainer1->pcb_trainer->params_do_next = deadlock_matcher;
		deadlock_matcher->trainer1->status = EXEC;
		deadlock_matcher->trainer1->pcb_trainer->status = EXEC_DEADLOCK;
		pthread_mutex_unlock(&deadlock_matcher->trainer1->sem);
	}
}

t_match_pokemon_trainer* get_next_to_exec() {
	return NULL;
}

t_match_pokemon_trainer* planner_rr() {
	return list_remove(matches, 0);
}

t_match_pokemon_trainer* planner_fifo() {
	return list_remove(matches, 0);
}

t_match_pokemon_trainer* planner_sjf() {
	uint32_t i;
	bool is_first = true;
	double aux;
	uint32_t aux_position;
	t_match_pokemon_trainer* to_exec;
	for(i = 0; i < list_size(matches); i++) {
		t_match_pokemon_trainer* match_pokemon_trainer = list_get(matches, i);
		double estimacion = calculate_estimacion_actual_rafaga(match_pokemon_trainer->closest_trainer);
		log_trace(logger, "Estimacion actual para entrenador %d: %f", match_pokemon_trainer->closest_trainer->name, estimacion);
		if(is_first || estimacion < aux) {
			is_first = false;
			aux = estimacion;
			to_exec = match_pokemon_trainer;
			aux_position = i;
		}
	}
	list_remove(matches, aux_position);
	if(to_exec->closest_trainer->pcb_trainer->status == EXEC) {
		to_exec->closest_trainer->estimacion_anterior = to_exec->closest_trainer->estimacion_actual;
	}
	return to_exec;
}

t_deadlock_matcher* planner_deadlock_rr() {
	return list_remove(queue_deadlock, 0);
}

t_deadlock_matcher* planner_deadlock_fifo() {
	return list_remove(queue_deadlock, 0);
}

t_deadlock_matcher* planner_deadlock_sjf() {
	uint32_t i;
	bool  is_first = true;
	double aux;
	uint32_t aux_position;
	t_deadlock_matcher* to_exec;
	for(i = 0; i < list_size(queue_deadlock); i++) {
		t_deadlock_matcher* deadlock_matcher = list_get(queue_deadlock, i);
		double estimacion = calculate_estimacion_actual_rafaga(deadlock_matcher->trainer1);
		log_trace(logger, "Estimacion actual para entrenador %d: %f", deadlock_matcher->trainer1->name, estimacion);
		if(is_first || estimacion < aux) {
			is_first = false;
			aux = estimacion;
			to_exec = deadlock_matcher;
			aux_position = i;
		}
	}
	list_remove(queue_deadlock, aux_position);
	if(to_exec->trainer1->pcb_trainer->status == EXEC) {
		to_exec->trainer1->estimacion_anterior = to_exec->trainer1->estimacion_actual;
	}
	return to_exec;
}

double calculate_estimacion_actual_rafaga(t_trainer* trainer) {
	if(trainer->sjf_calculado) {
		return trainer->estimacion_actual;
	} else {
		double estimacion = ALPHA * trainer->real_anterior + (1.0 - ALPHA) * trainer->estimacion_anterior;
		trainer->estimacion_actual =  estimacion;
		trainer->sjf_calculado = true;
		return estimacion;
	}
}

