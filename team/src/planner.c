#include "planner.h"


void* planning_catch() {
	while(1) {
		sem_wait(&sem_count_matches);
		pthread_mutex_lock(&mutex_planning);
		pthread_mutex_lock(&mutex_matches);
		t_match_pokemon_trainer* match_pokemon_trainer = list_remove(matches, 0);
		pthread_mutex_unlock(&mutex_matches);

		t_trainer* trainer = match_pokemon_trainer->closest_trainer;
		t_appeared_pokemon* pokemon = match_pokemon_trainer->closest_pokemon;
		trainer->pcb_trainer->pokemon_to_catch = pokemon;
		trainer->status = EXEC;
		log_info(logger, "Planificando por FIFO, entrenador %s en x: %d y: %d, a atrapar pokemon %s en x: %d y: %d", &trainer->name, trainer->pos_x,
				trainer->pos_y, pokemon->pokemon, pokemon->pos_x, pokemon->pos_y);
		pthread_mutex_unlock(&match_pokemon_trainer->closest_trainer->sem);
	}
}

void* planning_deadlock() {
	while(1) {
		sem_wait(&sem_count_queue_deadlocks);
		pthread_mutex_lock(&mutex_planning_deadlock);
		pthread_mutex_lock(&mutex_queue_deadlocks);
		t_deadlock_matcher* deadlock_matcher= list_remove(queue_deadlock, 0);
		pthread_mutex_unlock(&mutex_queue_deadlocks);

		deadlock_matcher->trainer1->pcb_trainer->do_next = &swap_pokemons;
		deadlock_matcher->trainer1->pcb_trainer->params_do_next = deadlock_matcher;
		deadlock_matcher->trainer1->status = EXEC;
		pthread_mutex_unlock(&deadlock_matcher->trainer1->sem);
	}
}


