#include "resolver.h"

void proceed_to_finish() {

	void swap_pokemons(t_deadlock_matcher* deadlock_matcher) {
		log_info(logger, "Intercambio entre %s y %s", deadlock_matcher->pokemon1, deadlock_matcher->pokemon2);
	}

	list_iterate(matched_deadlocks, swap_pokemons);


//	t_list* trainers_with_deadlock = get_trainers_with_deadlock();
//
//	if(list_size(trainers_with_deadlock) == 0) {
//		//Termino el proceso, finaliza el team
//		exit(0);
//	} else {
//		handle_deadlock(trainers_with_deadlock);
//	}
}




//
//t_list* get_trainers_with_deadlock() {
//	return list_filter(trainers, trainer_not_exit);
//}
//
//bool trainer_not_exit(t_trainer* trainer) {
//	return trainer->status != EXIT;
//}
//
//void handle_deadlock(t_list* trainers_with_deadlock) {
//	t_list* direct_deadlock = list_create();
//	uint32_t i;
//
//	for(i = 0; i < list_size(trainers_with_deadlock); i++) {
//		t_trainer* trainer = list_get(trainers_with_deadlock, i);
//		uint32_t aux;
//
//		t_dictionary* caught = trainer->caught;
//		t_dictionary* objectives = trainer->objective;
//
//		t_dictionary* leftovers = get_dictionary_difference(caught, objectives);
//		t_dictionary* remaining = get_dictionary_difference(objectives, caught);
//
//		for(aux = 0; aux < list_size(trainers_with_deadlock); aux++) {
//			t_trainer* trainer_to_compare = list_get(trainers_with_deadlock);
//
//			t_dictionary* caught_to_compare = trainer_to_compare->caught;
//			t_dictionary* objectives_to_compare = trainer_to_compare->objective;
//			t_dictionary* leftovers_to_compare = get_dictionary_difference(caught_to_compare, objectives_to_compare);
//			t_dictionary* remaining_to_compare = get_dictionary_difference(objectives_to_compare, caught_to_compare);
//
//		}
//	}
//}

t_list* get_dictionary_difference(t_dictionary* a, t_dictionary* b) {

	t_list* difference = list_create();

	void get_difference(char* key, uint32_t* value) {
		int q_diff;
		if(dictionary_has_key(b, key)) {
			uint32_t* q_b = dictionary_get(b, key);
			q_diff =  (*value) - (*q_b);
		} else {
			q_diff =  (*value);
		}

		int i;
		for(i = 0; i < q_diff; i++) {
			list_add(difference, key);
		}
	}

	dictionary_iterator(a, get_difference);
	return difference;
}
