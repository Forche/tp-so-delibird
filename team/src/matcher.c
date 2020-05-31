#include "matcher.h"

void* match_pokemon_with_trainer() {
	while(1) {
		sem_wait(&sem_trainer_available);
		sem_wait(&sem_appeared_pokemon);

		pthread_mutex_lock(&mutex_pokemon_received_to_catch);
		t_match_pokemon_trainer* match_pokemon_trainer = match_closest_trainer();
		pthread_mutex_unlock(&mutex_pokemon_received_to_catch);

		match_pokemon_trainer->closest_trainer->status = READY;

		pthread_mutex_lock(&mutex_matches);
		list_add(matches, match_pokemon_trainer);
		sem_post(&sem_count_matches);
		pthread_mutex_unlock(&mutex_matches);
	}
}

t_match_pokemon_trainer* match_closest_trainer() {
	t_match_pokemon_trainer* match_pokemon_trainer = malloc(sizeof(t_match_pokemon_trainer));
	match_pokemon_trainer->shortest_distance = -1;
	uint32_t i;
	for (i = 0; i < list_size(pokemon_received_to_catch); i++) {
		t_match_pokemon_trainer* match_pokemon_trainer_by_pokemon = malloc(sizeof(t_match_pokemon_trainer));
		t_appeared_pokemon* appeared_pokemon = list_get(pokemon_received_to_catch, i);
		t_trainer* closest_trainer = get_closest_trainer(appeared_pokemon);
		match_pokemon_trainer_by_pokemon->closest_pokemon = appeared_pokemon;
		match_pokemon_trainer_by_pokemon->closest_trainer = closest_trainer;
		match_pokemon_trainer_by_pokemon->shortest_distance = get_distance(closest_trainer, appeared_pokemon);
		if (match_pokemon_trainer->shortest_distance < 0
				|| match_pokemon_trainer_by_pokemon->shortest_distance
						< match_pokemon_trainer->shortest_distance) {
			match_pokemon_trainer = match_pokemon_trainer_by_pokemon;
		}
	}
	return match_pokemon_trainer;
}

t_trainer* get_closest_trainer(t_appeared_pokemon* appeared_pokemon) {
	uint32_t i;
	uint32_t shortest_distance = -1;
	t_trainer* closest_trainer = malloc(sizeof(t_trainer));
	for(i = 0; i < list_size(trainers); i++) {
		t_trainer* trainer = list_get(trainers, i);
		uint32_t distance = get_distance(trainer, appeared_pokemon);
		if(shortest_distance < 0 || distance < shortest_distance) {
			shortest_distance = distance;
			closest_trainer = trainer;
		}
	}
	return closest_trainer;
}

int get_distance(t_trainer* trainer, t_appeared_pokemon* pokemon) {
	return abs(trainer->pos_x - pokemon->pos_x) + abs(trainer->pos_y - pokemon->pos_y);
}
