#include "resolver.h"

void swap_pokemons(t_deadlock_matcher* deadlock_matcher);

void proceed_to_finish() {
	list_iterate(matched_deadlocks, swap_pokemons);

	t_list* trainers_with_deadlock = get_trainers_with_deadlock();

	if(list_is_empty(trainers_with_deadlock)) {
		//Termino el proceso, finaliza el team
		exit(0);
	} else {
		handle_deadlock(trainers_with_deadlock);
	}
}

t_list* get_trainers_with_deadlock() {
	return list_filter(trainers, trainer_not_exit);
}

bool trainer_not_exit(t_trainer* trainer) {
	return trainer->status != EXIT;
}

void handle_deadlock(t_list* trainers_with_deadlock) {
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

		list_remove_by_value(leftovers, pokemon_i_give);
		list_remove_by_value(remaining, pokemon_i_need);
		list_remove_by_value(leftovers_to_compare, pokemon_i_need);

		swap_pokemons(deadlock_match);
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

void swap_pokemons(t_deadlock_matcher* deadlock_matcher) {
	log_info(logger, "Intercambio entre %s y %s", deadlock_matcher->pokemon1, deadlock_matcher->pokemon2);

	t_trainer* trainer_1 = deadlock_matcher->trainer1;
	t_trainer* trainer_2 = deadlock_matcher->trainer2;
	char* pokemon_1 = deadlock_matcher->pokemon1;
	char* pokemon_2 = deadlock_matcher->pokemon2;

	trainer_1->status = READY;

	//Esta parte se deberia sacar y el planificador decidir cual ejecutar
	//Ver idea de carito de asignar que funcion debe ejecutar (deadlock en este caso)

	//Se planifica para hacer el deadlock
	trainer_1->status = EXEC;
	move_to_position(trainer_1, trainer_2->pos_x, trainer_2->pos_y);
	substract_from_dictionary(trainer_1->caught, pokemon_1);
	substract_from_dictionary(trainer_2->caught, pokemon_2);

	add_to_dictionary(trainer_1->caught, pokemon_2);
	add_to_dictionary(trainer_2->caught, pokemon_1);

	t_list* leftovers_trainer_1 = get_dictionary_difference(trainer_1->caught, trainer_1->objective);

	if(list_is_empty(leftovers_trainer_1)) {
		trainer_1->status = EXIT;
	}

	t_list* leftovers_trainer_2 = get_dictionary_difference(trainer_2->caught, trainer_2->objective);

	if(list_is_empty(leftovers_trainer_2)) {
		trainer_2->status = EXIT;
	}
}

