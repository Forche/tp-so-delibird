#include "planner.h"


void* planning_catch() {
	while(1) {
		sem_wait(&sem_count_matches);
		pthread_mutex_lock(&mutex_planning);
		pthread_mutex_lock(&mutex_matches);
		t_match_pokemon_trainer* match_pokemon_trainer = list_remove(matches, 0);
		pthread_mutex_unlock(&mutex_matches);

		pthread_mutex_unlock(&match_pokemon_trainer->closest_trainer->sem);
		match_pokemon_trainer->closest_trainer->status = EXEC;
	}
}

