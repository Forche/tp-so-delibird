#include "planner.h"


void* planning_catch() {
	while(1) {
		sem_wait(&sem_count_matches);
		pthread_mutex_lock(&mutex_planning);
		pthread_mutex_lock(&mutex_matches);
		t_match_pokemon_trainer* match_pokemon_trainer = list_remove(matches, 0);
		pthread_mutex_unlock(&mutex_matches);

		match_pokemon_trainer->closest_trainer->pcb_trainer->pokemon_to_catch = match_pokemon_trainer->closest_pokemon;
		match_pokemon_trainer->closest_trainer->status = EXEC;
		log_info(logger, "Pokemon %s en x: %d y: %d", match_pokemon_trainer->closest_pokemon->pokemon,
				match_pokemon_trainer->closest_pokemon->pos_x, match_pokemon_trainer->closest_pokemon->pos_y);

		pthread_mutex_unlock(&match_pokemon_trainer->closest_trainer->sem);
	}
}

