#include "matcher.h"

void* match_pokemon_with_trainer() {
	while(1) {
		pthread_mutex_lock(sem_existing_pokemon);
		sem_wait(sem_trainer_available);

		t_match_pokemon_trainer* match_pokemon_trainer;

		uint32_t i;
		for( i=0; i < list_size(pokemon_received_to_catch); i++) {
			t_match_pokemon_trainer* match_pokemon_trainer_by_pokemon;
			match_pokemon_trainer_by_pokemon->closest_pokemon = list_get(pokemon_received_to_catch, i);
			match_pokemon_trainer_by_pokemon->shortest_distance = NULL;
			match_pokemon_trainer_by_pokemon = list_fold(trainers, match_pokemon_trainer, distance_to_pokemon);
			if(match_pokemon_trainer == NULL ||
					match_pokemon_trainer_by_pokemon->shortest_distance < match_pokemon_trainer->shortest_distance) {
				match_pokemon_trainer = match_pokemon_trainer_by_pokemon;
			}
		}

		match_pokemon_trainer->closest_trainer->status = READY;
		list_add(matches, match_pokemon_trainer);
	}
}

t_match_pokemon_trainer* distance_to_pokemon(t_trainer* trainer, t_match_pokemon_trainer* closest) {

	uint32_t distance = get_distance(trainer, closest->closest_pokemon);
	if(closest->shortest_distance == NULL || distance < closest->shortest_distance) {
		closest->closest_trainer = trainer;
		closest->shortest_distance = distance;
	}
	return closest;
}

int get_distance(t_trainer* trainer, t_appeared_pokemon* pokemon) {
	return abs(trainer->pos_x - pokemon->pos_x) + abs(trainer->pos_y - pokemon->pos_y);
}

