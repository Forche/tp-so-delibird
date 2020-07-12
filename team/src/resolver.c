#include "resolver.h"


void add_to_queue_deadlocks(t_deadlock_matcher* deadlock_matcher);
void proceed_to_finish() {
	sem_init(&sem_deadlock_directo, 0, 1);
	list_iterate(matched_deadlocks, add_to_queue_deadlocks);
	sem_wait(&sem_deadlock_directo);

	t_list* trainers_with_deadlock = get_trainers_with_deadlock();

	if(list_is_empty(trainers_with_deadlock)) {
		void print_caught(t_trainer* trainer) {
			log_info(logger, "Entrenador %s", &trainer->name);
			dictionary_iterator(trainer->caught, print_pokemons);
		}

		list_iterate(trainers,print_caught);
		//Termino el proceso, finaliza el team
		exit(0);
	} else {
		handle_deadlock(trainers_with_deadlock);

		void print_caught(t_trainer* trainer) {
			log_info(logger, "Entrenador %s", &trainer->name);
			dictionary_iterator(trainer->caught, print_pokemons);
		}

		list_iterate(trainers,print_caught);
	}
}

t_list* get_trainers_with_deadlock() {
	return list_filter(trainers, trainer_not_exit);
}

bool trainer_not_exit(t_trainer* trainer) {
	return trainer->status != EXIT;
}

void handle_deadlock(t_list* trainers_with_deadlock) {

	log_info(logger, "Inicio proceso deteccion Deadlock Indirectos");
	t_list* direct_deadlock = list_create();
	uint32_t i;

	for(i = 0; i < list_size(trainers_with_deadlock); i++) {
		t_trainer* trainer = list_get(trainers_with_deadlock, i);
		if(trainer->status == EXIT) {
			continue;
		}

		t_dictionary* caught = trainer->caught;
		t_dictionary* objectives = trainer->objective;

		t_list* leftovers = get_dictionary_difference(caught, objectives);
		t_list* remaining = get_dictionary_difference(objectives, caught);

		uint32_t aux;
		for(aux = 0; aux < list_size(trainers_with_deadlock); aux++) {
			t_trainer* trainer_to_compare = list_get(trainers_with_deadlock, aux);
			if(i == aux || trainer_to_compare->status == EXIT) {
				continue;
			}
			give_me_my_pokemons_dude(trainer, trainer_to_compare, leftovers, remaining);
		}
	}
}

void give_me_my_pokemons_dude(t_trainer* trainer, t_trainer* trainer_to_compare, t_list* leftovers, t_list* remaining) {

	t_dictionary* caught_to_compare = trainer_to_compare->caught;
	t_dictionary* objectives_to_compare = trainer_to_compare->objective;
	t_list* leftovers_to_compare = get_dictionary_difference(caught_to_compare, objectives_to_compare);
	t_list* remaining_to_compare = get_dictionary_difference(objectives_to_compare, caught_to_compare);

	char* pokemon_i_need = get_pokemon_I_need(remaining, leftovers_to_compare);
	if(pokemon_i_need != NULL) {
		char* pokemon_i_give = get_pokemon_I_need(remaining_to_compare, leftovers);
		if(pokemon_i_give == NULL) {
			pokemon_i_give = list_get(leftovers, 0);
		} else {
			list_remove_by_value(remaining_to_compare, pokemon_i_give);
		}
		t_deadlock_matcher* deadlock_match = malloc(sizeof(t_deadlock_matcher));
		deadlock_match->trainer1 = trainer;
		deadlock_match->pokemon1 = pokemon_i_give;
		deadlock_match->trainer2 = trainer_to_compare;
		deadlock_match->pokemon2 = pokemon_i_need;

		log_info(logger, "Deadlock indirecto. Entrenador %s recibe %s. Entrenador %s recibe %s, que no lo necesita",
				&trainer->name, pokemon_i_need, &trainer_to_compare->name, pokemon_i_give);

		list_remove_by_value(leftovers, pokemon_i_give);
		list_remove_by_value(remaining, pokemon_i_need);
		list_remove_by_value(leftovers_to_compare, pokemon_i_need);

		add_to_queue_deadlocks(deadlock_match);
		if(trainer->status != EXIT) {
			give_me_my_pokemons_dude(trainer, trainer_to_compare, leftovers, remaining);
		}
	}
}
//ESTA DUPLICADA
char* get_pokemon_I_need(t_list* remaining, t_list* leftovers) {
	bool i_have_one(void* pokemon_remaining) {
		bool get_by_pokemon(void* pokemon_leftover) {
			return string_equals_ignore_case(pokemon_leftover, pokemon_remaining);
		}
		return list_find(remaining, get_by_pokemon);
	}

	return list_find(leftovers, i_have_one);
}

void add_to_queue_deadlocks(t_deadlock_matcher* deadlock_matcher) {
	deadlock_matcher->trainer1->status = READY;
	deadlock_matcher->trainer2->status = SWAPPING;
	pthread_mutex_lock(&mutex_queue_deadlocks);
	list_add(queue_deadlock, deadlock_matcher);
	pthread_mutex_unlock(&mutex_queue_deadlocks);
	sem_wait(&sem_deadlock_directo);
	sem_post(&sem_count_queue_deadlocks);
}

