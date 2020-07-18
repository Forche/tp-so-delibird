#include "resolver.h"

void add_to_queue_deadlocks_wait_status(t_deadlock_matcher* deadlock_matcher);
void create_thread_add_to_queue_deadlocks(t_deadlock_matcher* deadlock_matcher);
void add_to_queue_deadlocks(t_deadlock_matcher* deadlock_matcher);
void proceed_to_finish() {
	pthread_t thread_planner_deadlock = create_thread(planning_deadlock, "planning_deadlock");

	if(list_size(matched_deadlocks) > 0) {
		pthread_mutex_init(&mutex_direct_deadlocks, NULL);
		pthread_mutex_lock(&mutex_direct_deadlocks);

		count_sem_reverse_direct_deadlock = 0;
		pthread_mutex_lock(&mutex_matched_deadlocks);
		list_iterate(matched_deadlocks, create_thread_add_to_queue_deadlocks);
		pthread_mutex_unlock(&mutex_matched_deadlocks);

		pthread_mutex_lock(&mutex_direct_deadlocks);
	}

	t_list* trainers_with_deadlock = get_trainers_with_deadlock();

	if(list_is_empty(trainers_with_deadlock)) {

		void print_caught(t_trainer* trainer) {
			log_info(logger, "Entrenador %d. Ciclos CPU: %d", trainer->name, trainer->q_ciclos_cpu);
			dictionary_iterator(trainer->caught, print_pokemons);
		}

		list_iterate(trainers,print_caught);
		log_info(logger, "Ciclos CPU Totales: %d", q_ciclos_cpu_totales);
		log_info(logger, "Cambios de Contexto Totales: %d", q_cambios_contexto_totales);
		//Termino el proceso, finaliza el team
		exit(0);
	} else {
		handle_deadlock(trainers_with_deadlock);

		void print_caught(t_trainer* trainer) {
			log_info(logger, "Entrenador %d. Ciclos CPU: %d", trainer->name, trainer->q_ciclos_cpu);
			dictionary_iterator(trainer->caught, print_pokemons);
		}

		list_iterate(trainers,print_caught);
		log_info(logger, "Ciclos CPU Totales: %d", q_ciclos_cpu_totales);
		log_info(logger, "Cambios de Contexto Totales: %d", q_cambios_contexto_totales);
		exit(0);
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
		char* necesita_no_lo_necesita = "que no lo necesita";
		if(pokemon_i_give == NULL) {
			pokemon_i_give = list_get(leftovers, 0);
		} else {
			necesita_no_lo_necesita = "que lo necesita";
			list_remove_by_value(remaining_to_compare, pokemon_i_give);
		}
		t_deadlock_matcher* deadlock_match = malloc(sizeof(t_deadlock_matcher));
		deadlock_match->trainer1 = trainer;
		deadlock_match->pokemon1 = pokemon_i_give;
		deadlock_match->trainer2 = trainer_to_compare;
		deadlock_match->pokemon2 = pokemon_i_need;

		log_info(logger, "Deadlock indirecto. Entrenador %d recibe %s. Entrenador %d recibe %s, %s",
				trainer->name, pokemon_i_need, trainer_to_compare->name, pokemon_i_give, necesita_no_lo_necesita);

		list_remove_by_value(leftovers, pokemon_i_give);
		list_remove_by_value(remaining, pokemon_i_need);
		list_remove_by_value(leftovers_to_compare, pokemon_i_need);

		resolve_deadlock(deadlock_match);
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

void create_thread_add_to_queue_deadlocks(t_deadlock_matcher* deadlock_matcher) {
	log_info(logger, "Estado entrenador %d: %d", deadlock_matcher->trainer1->name, deadlock_matcher->trainer1->status);
	log_info(logger, "Estado entrenador %d: %d", deadlock_matcher->trainer2->name, deadlock_matcher->trainer2->status);
	if((deadlock_matcher->trainer1->status == FULL || deadlock_matcher->trainer1->status == BLOCK) &&
			(deadlock_matcher->trainer2->status == FULL || deadlock_matcher->trainer2->status == BLOCK)) {
		add_to_queue_deadlocks(deadlock_matcher);
	} else {
	create_thread_with_param(add_to_queue_deadlocks_wait_status, deadlock_matcher, "add_to_queue_deadlocks");
	}
}

void add_to_queue_deadlocks_wait_status(t_deadlock_matcher* deadlock_matcher) {
	if(deadlock_matcher->trainer1->status != FULL && deadlock_matcher->trainer1->status != BLOCK) {
		pthread_mutex_lock(&deadlock_matcher->trainer1->pcb_trainer->sem_deadlock);
	}
	if(deadlock_matcher->trainer2->status != FULL && deadlock_matcher->trainer2->status != BLOCK) {
		pthread_mutex_lock(&deadlock_matcher->trainer2->pcb_trainer->sem_deadlock);
	}
	add_to_queue_deadlocks(deadlock_matcher);
}

void add_to_queue_deadlocks(t_deadlock_matcher* deadlock_matcher) {
	log_info(logger, "Disponible a planificar deadlock entre entrenador %d y %d", deadlock_matcher->trainer1->name, deadlock_matcher->trainer2->name);
	deadlock_matcher->trainer1->status = READY;
	deadlock_matcher->trainer2->status = SWAPPING;
	pthread_mutex_lock(&mutex_queue_deadlocks);
	list_add(queue_deadlock, deadlock_matcher);
	pthread_mutex_unlock(&mutex_queue_deadlocks);
	sem_post(&sem_count_queue_deadlocks);
}

void resolve_deadlock(t_deadlock_matcher* deadlock_matcher) {
	t_trainer* trainer_1 = deadlock_matcher->trainer1;
	t_trainer* trainer_2 = deadlock_matcher->trainer2;
	char* pokemon_1 = deadlock_matcher->pokemon1;
	char* pokemon_2 = deadlock_matcher->pokemon2;
	log_info(logger, "Inicio intercambio entre %d y %d, pokemons %s y %s", trainer_1->name, trainer_2->name, pokemon_1, pokemon_2);

	move_to_position(trainer_1, trainer_2->pos_x, trainer_2->pos_y, deadlock_matcher, false);
	exchange_pokemons(deadlock_matcher, false);

	log_info(logger, "Finalizado intercambio entre %d y %d, pokemons %s y %s", trainer_1->name, trainer_2->name, pokemon_1, pokemon_2);

	validate_state_trainer(trainer_1);
	validate_state_trainer(trainer_2);
}

