#include "matcher.h"

void* match_pokemon_with_trainer() {
	while(1) {
		sem_wait(&sem_trainer_available);
		sem_wait(&sem_appeared_pokemon);

		pthread_mutex_lock(&mutex_pokemon_received_to_catch);
		pthread_mutex_lock(&mutex_trainers);
		t_match_pokemon_trainer* match_pokemon_trainer = match_closest_trainer();
		pthread_mutex_unlock(&mutex_trainers);
		pthread_mutex_unlock(&mutex_pokemon_received_to_catch);

		match_pokemon_trainer->closest_trainer->status = READY;

		pthread_mutex_lock(&mutex_matches);
		list_add(matches, match_pokemon_trainer);
		pthread_mutex_unlock(&mutex_matches);
		sem_post(&sem_count_matches);
	}
}

t_match_pokemon_trainer* match_closest_trainer() {
	t_match_pokemon_trainer* match_pokemon_trainer = malloc(sizeof(t_match_pokemon_trainer));
	match_pokemon_trainer->shortest_distance = -1;
	uint32_t i;
	uint32_t aux_pos;
	for (i = 0; i < list_size(pokemon_received_to_catch); i++) {
		t_appeared_pokemon* appeared_pokemon = list_get(pokemon_received_to_catch, i);

		if (check_remaining_minus_vistima(appeared_pokemon->pokemon)) {
			t_match_pokemon_trainer* match_pokemon_trainer_by_pokemon = malloc(sizeof(t_match_pokemon_trainer));
			t_trainer* closest_trainer = get_closest_trainer(appeared_pokemon);
			match_pokemon_trainer_by_pokemon->closest_pokemon = appeared_pokemon;
			match_pokemon_trainer_by_pokemon->closest_trainer = closest_trainer;
			match_pokemon_trainer_by_pokemon->shortest_distance = get_distance(closest_trainer, appeared_pokemon);
			if (match_pokemon_trainer->shortest_distance < 0
					|| match_pokemon_trainer_by_pokemon->shortest_distance
							< match_pokemon_trainer->shortest_distance) {
				match_pokemon_trainer = match_pokemon_trainer_by_pokemon;
				aux_pos = i;
			}
		}
	}


	pthread_mutex_lock(&mutex_being_caught_pokemons);
	add_to_dictionary(being_caught_pokemons, match_pokemon_trainer->closest_pokemon->pokemon);
	pthread_mutex_unlock(&mutex_being_caught_pokemons);

	list_remove(pokemon_received_to_catch, aux_pos);
	return match_pokemon_trainer;
}

uint32_t check_remaining_minus_vistima(char* pokemon) {
	pthread_mutex_lock(&mutex_remaining_pokemons);
	bool has_key_remaining = dictionary_has_key(remaining_pokemons, pokemon);
	if(has_key_remaining) {
		uint32_t* cant_remaining = dictionary_get(remaining_pokemons, pokemon);
		pthread_mutex_unlock(&mutex_remaining_pokemons);
		pthread_mutex_lock(&mutex_being_caught_pokemons);
		bool has_key = dictionary_has_key(being_caught_pokemons, pokemon);
		pthread_mutex_unlock(&mutex_being_caught_pokemons);
		if(has_key) {
			pthread_mutex_lock(&mutex_being_caught_pokemons);
			uint32_t* cant_vistima = dictionary_get(being_caught_pokemons, pokemon);
			pthread_mutex_unlock(&mutex_being_caught_pokemons);
			return (*cant_remaining) - (*cant_vistima);
		} else {
			return *cant_remaining;
		}
	} else {
		pthread_mutex_unlock(&mutex_remaining_pokemons);
		return 0;
	}
}

t_trainer* get_closest_trainer(t_appeared_pokemon* appeared_pokemon) {
	uint32_t i;
	int shortest_distance = -1;
	t_trainer* closest_trainer = malloc(sizeof(t_trainer));
	t_list* trainers_not_in_exec = list_filter(trainers, not_exec);
	for(i = 0; i < list_size(trainers_not_in_exec); i++) {
		t_trainer* trainer = list_get(trainers_not_in_exec, i);
		int distance = get_distance(trainer, appeared_pokemon);
		if(shortest_distance < 0 || distance < shortest_distance) {
			shortest_distance = distance;
			closest_trainer = trainer;
		}
	}
	return closest_trainer;
}

bool not_exec(t_trainer* trainer) {
	return trainer->status != EXEC && trainer->status != READY && trainer->pcb_trainer->status == NEW;
}

int get_distance(t_trainer* trainer, t_appeared_pokemon* pokemon) {
	return abs(trainer->pos_x - pokemon->pos_x) + abs(trainer->pos_y - pokemon->pos_y);
}
