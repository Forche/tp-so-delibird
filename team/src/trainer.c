#include "trainer.h"

void move_to_position(t_trainer* trainer, uint32_t pos_x, uint32_t pos_y, t_deadlock_matcher* deadlock_matcher, bool with_validate_desalojo);
void move_one_step(uint32_t* pos1, uint32_t pos2);
void trainer_must_go_on(t_trainer* trainer, t_deadlock_matcher* deadlock_matcher);
void desalojar(t_trainer* trainer, bool refresh_estimation, t_deadlock_matcher* deadlock_matcher, bool lock_sem_caught);

bool all_trainers_full() {
	bool all_full(t_trainer* trainer) {
		return trainer->status == FULL || trainer->status == EXIT;
	}

	return list_all_satisfy(trainers, all_full);
}

void handle_trainer(t_trainer* trainer) {
	trainer->thread = pthread_self();
	trainer->pcb_trainer->status = NEW;
	trainer->pcb_trainer->do_next = &trainer_catch_pokemon;
	trainer->pcb_trainer->params_do_next = trainer;
	trainer->pcb_trainer->quantum = 0;
	pthread_mutex_init(&trainer->sem, NULL);
	pthread_mutex_lock(&trainer->sem); // Para que quede en 0.
	pthread_mutex_init(&trainer->pcb_trainer->sem_caught, NULL);
	pthread_mutex_lock(&trainer->pcb_trainer->sem_caught);
	pthread_mutex_init(&trainer->pcb_trainer->sem_deadlock, NULL);
	pthread_mutex_lock(&trainer->pcb_trainer->sem_deadlock);

	if(is_trainer_full(trainer)) {
		dictionary_iterator(trainer->caught, print_pokemons);
		change_status_to(trainer, FULL);
		validate_deadlock(trainer);
		log_trace(logger, "Entrenador %d completo", trainer->name);

		pthread_mutex_lock(&mutex_trainers);
		bool trainers_full = all_trainers_full();
		pthread_mutex_unlock(&mutex_trainers);
		if(dictionary_is_empty(remaining_pokemons) && trainers_full) {
			log_trace(logger, "Todos los entrenadores completos");
			sem_post(&sem_all_pokemons_caught);
		}
	}
	while(1) {
		pthread_mutex_lock(&trainer->sem);
		trainer->pcb_trainer->do_next(trainer->pcb_trainer->params_do_next);
	}


}

void trainer_catch_pokemon(t_trainer* trainer) {
	t_pcb_trainer* pcb_trainer = trainer->pcb_trainer;
	pcb_trainer->status = EXEC;

	move_to_position(trainer, pcb_trainer->pokemon_to_catch->pos_x, pcb_trainer->pokemon_to_catch->pos_y, NULL, true);
	catch_pokemon(trainer, pcb_trainer->pokemon_to_catch);
}

void handle_catch(t_trainer* trainer) {
	t_pcb_trainer* pcb_trainer = trainer->pcb_trainer;
	if (pcb_trainer->result_catch == 1) {
		add_to_dictionary(trainer->caught, pcb_trainer->pokemon_to_catch->pokemon);

		pthread_mutex_lock(&mutex_caught_pokemons);
		add_to_dictionary(caught_pokemons, pcb_trainer->pokemon_to_catch->pokemon);
		pthread_mutex_unlock(&mutex_caught_pokemons);

		pthread_mutex_lock(&mutex_remaining_pokemons);
		substract_from_dictionary(remaining_pokemons, pcb_trainer->pokemon_to_catch->pokemon);
		pthread_mutex_unlock(&mutex_remaining_pokemons);

		must_remove_from_backup(pcb_trainer);
		log_info(logger, "Atrapado pokemon %s, entrenador %d", pcb_trainer->pokemon_to_catch->pokemon, trainer->name);
	} else {
		log_info(logger, "Error atrapar pokemon %s, entrenador %d", pcb_trainer->pokemon_to_catch->pokemon, trainer->name);
		search_in_backup(pcb_trainer);
	}

	pthread_mutex_lock(&mutex_being_caught_pokemons);
	substract_from_dictionary(being_caught_pokemons, pcb_trainer->pokemon_to_catch->pokemon);
	pthread_mutex_unlock(&mutex_being_caught_pokemons);

	if(is_trainer_full(trainer)) {
		dictionary_iterator(trainer->caught, print_pokemons);
		change_status_to(trainer, FULL);
		validate_deadlock(trainer);

		log_info(logger, "Entrenador %d completo", trainer->name);

		pthread_mutex_lock(&mutex_trainers);
		bool trainers_full = all_trainers_full();
		pthread_mutex_unlock(&mutex_trainers);

		if(dictionary_is_empty(remaining_pokemons) && trainers_full) {
			log_trace(logger, "Todos los entrenadores completos");
			sem_post(&sem_all_pokemons_caught);
		}
	} else {
		trainer->pcb_trainer->status = NEW;
		change_status_to(trainer, BLOCK);
		sem_post(&sem_trainer_available);
	}

	trainer->real_anterior = trainer->estimacion_anterior - trainer->estimacion_actual;
	trainer->sjf_calculado = false;
	trainer->pcb_trainer->quantum = 0;
}

void search_in_backup(t_pcb_trainer* pcb_trainer) {
	pthread_mutex_lock(&mutex_appeared_backup);
	uint32_t i;
	for (i = 0; i < list_size(appeared_backup); i++) {
		t_appeared_pokemon* pokemon_backup = list_get(appeared_backup, i);
		if (string_equals_ignore_case(pcb_trainer->pokemon_to_catch->pokemon,
				pokemon_backup->pokemon)) {
			list_remove(appeared_backup, i);
			pthread_mutex_lock(&mutex_pokemon_received_to_catch);
			list_add(pokemon_received_to_catch, pokemon_backup);
			pthread_mutex_unlock(&mutex_pokemon_received_to_catch);
			sem_post(&sem_appeared_pokemon);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_appeared_backup);
}

void must_remove_from_backup(t_pcb_trainer* pcb_trainer) {
	pthread_mutex_lock(&mutex_remaining_pokemons);
	uint32_t* q_catch = dictionary_get(remaining_pokemons, pcb_trainer->pokemon_to_catch->pokemon);
	pthread_mutex_unlock(&mutex_remaining_pokemons);
	if(q_catch != NULL && *q_catch) {

	} else {
		pthread_mutex_lock(&mutex_appeared_backup);
		int i;
		for (i = list_size(appeared_backup) - 1; i >= 0; i--) {
			t_appeared_pokemon* pokemon_backup = list_get(appeared_backup, i);
			if (string_equals_ignore_case(pcb_trainer->pokemon_to_catch->pokemon,
					pokemon_backup->pokemon)) {
				log_trace(logger, "Liberado del backup %s", pokemon_backup->pokemon);
				list_remove(appeared_backup, i);
				free(pokemon_backup->pokemon);
				free(pokemon_backup);
			}
		}
		pthread_mutex_unlock(&mutex_appeared_backup);
	}
}

void find_candidate_to_swap(t_list* remaining, t_list* leftovers, t_trainer* trainer) {
	uint32_t i;
	t_list* aux = list_create();
	list_add_all(aux, remaining);
	for (i = 0; i < list_size(aux); i++) {
		char* pokemon = list_get(aux, i);
		t_to_deadlock* has_pokemon_i_need = get_leftover_from_to_deadlock_by_pokemon(pokemon);
		if (has_pokemon_i_need != NULL) {
			char* pokemon_he_needs = get_pokemon_he_needs(leftovers, has_pokemon_i_need->remaining);
			if (pokemon_he_needs != NULL) {
				t_deadlock_matcher* deadlock_match = malloc(sizeof(t_deadlock_matcher));
				deadlock_match->trainer1 = trainer;
				deadlock_match->pokemon1 = pokemon_he_needs;
				deadlock_match->trainer2 = has_pokemon_i_need->trainer;
				deadlock_match->pokemon2 = pokemon;
				log_info(logger, "Deadlock directo. Entrenador %d recibe %s. Entrenador %d recibe %s",
						trainer->name, pokemon, has_pokemon_i_need->trainer->name, pokemon_he_needs);
				pthread_mutex_lock(&mutex_matched_deadlocks);
				list_add(matched_deadlocks, deadlock_match);
				pthread_mutex_unlock(&mutex_matched_deadlocks);
				list_remove_by_value(leftovers, pokemon_he_needs);
				list_remove_by_value(remaining, pokemon);
				list_remove_by_value(has_pokemon_i_need->leftovers, pokemon);
				list_remove_by_value(has_pokemon_i_need->remaining, pokemon_he_needs);
			} else {
				//NO MATCHEO
			}
		} else {
			//NO MATCHEO
		}
	}
}

t_to_deadlock* get_leftover_from_to_deadlock_by_pokemon(char* pokemon) {
	bool get_by_pokemon(t_to_deadlock* element) {
		bool get_by_pokemon2(char* pokemon_leftover) {
			return string_equals_ignore_case(pokemon, pokemon_leftover);
		}
		return list_find(element->leftovers, get_by_pokemon2);
	}

	return list_find(to_deadlock, get_by_pokemon);
}

char* get_pokemon_he_needs(t_list* leftovers, t_list* remaining) {
	bool i_have_one(void* pokemon_remaining) {
		bool get_by_pokemon(void* pokemon_leftover) {
			return string_equals_ignore_case(pokemon_leftover, pokemon_remaining);
		}
		return list_find(leftovers, get_by_pokemon);
	}

	return list_find(remaining, i_have_one);
}

void validate_deadlock(t_trainer* trainer) {
	t_dictionary* caught = trainer->caught;
	t_dictionary* objectives = trainer->objective;

	t_list* leftovers = get_dictionary_difference(caught, objectives);
	t_list* remaining = get_dictionary_difference(objectives, caught);

	if(list_is_empty(leftovers) && list_is_empty(remaining)) {
		trainer->status = EXIT;
		//Y agregar demas cosas cuando termina OK.
	} else {
		log_info(logger, "Inicio Algoritmo de deteccion de deadlocks directos para entrenador %d", trainer->name);
		find_candidate_to_swap(remaining, leftovers, trainer);
		t_to_deadlock* trainer_to_deadlock = malloc(sizeof(t_to_deadlock));
		trainer_to_deadlock->trainer = trainer;
		trainer_to_deadlock->leftovers = leftovers;
		trainer_to_deadlock->remaining = remaining;
		log_info(logger, "Terminado algoritmo de deteccion de deadlock para entrenador %d", trainer->name);
		list_add(to_deadlock, trainer_to_deadlock);
	}

}

bool is_trainer_full(t_trainer* trainer) {
	uint32_t q_caught = dictionary_add_values(trainer->caught);
	uint32_t q_objective = dictionary_add_values(trainer->objective);

	if(q_caught - q_objective) {
		return false;
	} else {
		return true;
	}
}


void change_status_to(t_trainer* trainer, status status) {
	trainer->status = status;
}

void move_to_position(t_trainer* trainer, uint32_t pos_x, uint32_t pos_y, t_deadlock_matcher* deadlock_matcher, bool with_validate_desalojo) {
	uint32_t q_mov_x = abs(trainer->pos_x - pos_x);
	uint32_t q_mov_y = abs(trainer->pos_y - pos_y);
	log_info(logger, "Moviendo entrenador %d de posicion x: %d y: %d a posicion x: %d y: %d",trainer->name, trainer->pos_x, trainer->pos_y, pos_x, pos_y);


	uint32_t i;
	for(i = 0; i < q_mov_x; i++) {
		move_one_step(&trainer->pos_x, pos_x);
		increment_q_ciclos_cpu(trainer);
		log_info(logger, "Movido entrenador %d a posicion x: %d y: %d", trainer->name,trainer->pos_x, trainer->pos_y);
		trainer->estimacion_actual = trainer->estimacion_actual - 1;
		if(with_validate_desalojo) {
			trainer_must_go_on(trainer, deadlock_matcher);
		}
	}
	for(i = 0; i < q_mov_y; i++) {
		move_one_step(&trainer->pos_y, pos_y);
		increment_q_ciclos_cpu(trainer);
		log_info(logger, "Movido entrenador %d a posicion x: %d y: %d", trainer->name,trainer->pos_x, trainer->pos_y);
		trainer->estimacion_actual = trainer->estimacion_actual - 1;
		if(with_validate_desalojo) {
			trainer_must_go_on(trainer, deadlock_matcher);
		}
	}
}

void trainer_must_go_on(t_trainer* trainer, t_deadlock_matcher* deadlock_matcher) {
	if(string_equals_ignore_case(ALGORITMO_PLANIFICACION, "SJF-CD")) {
		bool is_first = true;
		double aux;
		uint32_t i;
		if(deadlock_matcher == NULL) {
			t_match_pokemon_trainer* to_exec = NULL;
			pthread_mutex_lock(&mutex_matches);
			log_trace(logger, "Estimacion actual para entrenador activo %d: %f", trainer->name, trainer->estimacion_actual);
			for(i = 0; i < list_size(matches); i++) {
				t_match_pokemon_trainer* match_pokemon_trainer = list_get(matches, i);
				double estimacion = calculate_estimacion_actual_rafaga(match_pokemon_trainer->closest_trainer);
				log_trace(logger, "Estimacion actual para entrenador %d: %f", match_pokemon_trainer->closest_trainer->name, estimacion);
				if(is_first || estimacion < aux) {
					is_first = false;
					aux = estimacion;
					to_exec = match_pokemon_trainer;
				}
			}
			pthread_mutex_unlock(&mutex_matches);
			if(to_exec != NULL && to_exec->closest_trainer->estimacion_actual < trainer->estimacion_actual) {
				desalojar(trainer, false, deadlock_matcher, false);
			}
		} else {
			t_deadlock_matcher* to_exec = NULL;
			pthread_mutex_lock(&mutex_queue_deadlocks);
			log_trace(logger, "Estimacion actual para entrenador activo %d: %f", trainer->name, trainer->estimacion_actual);
			for(i = 0; i < list_size(queue_deadlock); i++) {
				t_deadlock_matcher* deadlock_matcher = list_get(queue_deadlock, i);
				double estimacion = calculate_estimacion_actual_rafaga(deadlock_matcher->trainer1);
				log_trace(logger, "Estimacion actual para entrenador %d: %f", deadlock_matcher->trainer1->name, estimacion);
				if(is_first || estimacion < aux) {
					is_first = false;
					aux = estimacion;
					to_exec = deadlock_matcher;
				}
			}
			pthread_mutex_unlock(&mutex_queue_deadlocks);
			if(to_exec != NULL && to_exec->trainer1->estimacion_actual < trainer->estimacion_actual) {
				desalojar(trainer, false, deadlock_matcher, false);
			}
		}
	} else if(string_equals_ignore_case(ALGORITMO_PLANIFICACION, "RR")) {
		trainer->pcb_trainer->quantum = trainer->pcb_trainer->quantum + 1;
		if(trainer->pcb_trainer->quantum >= QUANTUM) {
			desalojar(trainer, false, deadlock_matcher, false);
		}
	}
}


void move_one_step(uint32_t* pos1, uint32_t pos2) {
	if(*pos1  < pos2) {
	    sleep(RETARDO_CICLO_CPU);
	    *pos1 = *pos1 + 1;
	} else {
		sleep(RETARDO_CICLO_CPU);
		*pos1 = *pos1 - 1;
	}
}

void catch_pokemon(t_trainer* trainer, t_appeared_pokemon* appeared_pokemon) {
	log_info(logger, "Operacion de catch a pokemon %s, bloqueado entrenador %d en posicion x: %d y: %d", appeared_pokemon->pokemon,
			trainer->name, trainer->pos_x, trainer->pos_y);
	sleep(RETARDO_CICLO_CPU);
	increment_q_ciclos_cpu(trainer);
	uint32_t broker_msg_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_msg_connection == -1) {
		log_error(logger, "Entrenador %d: Error de comunicacion con el broker, realizando operacion catch default", trainer->name);
		trainer->pcb_trainer->result_catch = 1;
		wait_catch(trainer,false);
	} else {
		t_catch_pokemon* pokemon_to_catch = malloc(sizeof(t_catch_pokemon));
		pokemon_to_catch->pos_x = appeared_pokemon->pos_x;
		pokemon_to_catch->pos_y = appeared_pokemon->pos_y;
		pokemon_to_catch->pokemon_len = appeared_pokemon->pokemon_len;
		pokemon_to_catch->pokemon = appeared_pokemon->pokemon;
		t_buffer* buffer = serialize_t_catch_pokemon_message(pokemon_to_catch);
		trainer->pcb_trainer->msg_connection = broker_msg_connection;
		send_message(broker_msg_connection, CATCH_POKEMON, NULL, NULL, buffer);
		pthread_t thread = create_thread_with_param(receive_message_id, trainer, "receive_message_id");
		wait_catch(trainer, true);
	}
	trainer->estimacion_actual = trainer->estimacion_actual - 1;//Atrapar consume un ciclo de cpu
	create_thread_with_param(handle_catch, trainer, "handle_catch");
}

void wait_catch(t_trainer* trainer, bool lock_sem_caught) {
	log_info(logger, "Desalojando entrenador %d", trainer->name);
	pthread_mutex_unlock(&mutex_planning);
	trainer->status = CATCHING;
	if(lock_sem_caught) {
		pthread_mutex_lock(&trainer->pcb_trainer->sem_caught);
	}
}

void desalojar(t_trainer* trainer, bool refresh_estimation, t_deadlock_matcher* deadlock_matcher, bool lock_sem_caught) {
	log_info(logger, "Desalojando entrenador %d", trainer->name);
	trainer->pcb_trainer->quantum = 0;
	pthread_mutex_unlock(&mutex_planning);

	if(lock_sem_caught) {
		pthread_mutex_lock(&trainer->pcb_trainer->sem_caught);
	}

	trainer->status = READY;
	if(refresh_estimation) {
		trainer->real_anterior = trainer->estimacion_anterior - trainer->estimacion_actual;
		trainer->sjf_calculado = false;
	}

	bool catch = trainer->pcb_trainer->status == EXEC;

	if(catch) {
		t_match_pokemon_trainer* match_pokemon_trainer_by_pokemon = malloc(sizeof(t_match_pokemon_trainer));
		match_pokemon_trainer_by_pokemon->closest_pokemon = trainer->pcb_trainer->pokemon_to_catch;
		match_pokemon_trainer_by_pokemon->closest_trainer = trainer;
		pthread_mutex_lock(&mutex_matches);
		list_add(matches, match_pokemon_trainer_by_pokemon);
		pthread_mutex_unlock(&mutex_matches);
		sem_post(&sem_count_matches);
	} else {
		pthread_mutex_unlock(&mutex_planning_deadlock);
		pthread_mutex_lock(&mutex_queue_deadlocks);
		list_add(queue_deadlock, deadlock_matcher);
		pthread_mutex_unlock(&mutex_queue_deadlocks);
		sem_post(&sem_count_queue_deadlocks);
	}
	pthread_mutex_lock(&trainer->sem);
}

void receive_message_id(t_trainer* trainer) {
	listen(trainer->pcb_trainer->msg_connection, SOMAXCONN);
	uint32_t msg_id;
	recv(trainer->pcb_trainer->msg_connection, &msg_id, sizeof(uint32_t), MSG_WAITALL);
	trainer->pcb_trainer->id_message = msg_id;
	close(trainer->pcb_trainer->msg_connection);
}


t_trainer* obtener_trainer_mensaje(t_message* msg) {
	uint32_t i;
	for(i = 0; i < list_size(trainers); i++) {
		t_trainer* trainer = list_get(trainers, i);
		if(trainer->pcb_trainer->id_message == msg->correlative_id) {
			log_trace(logger, "Respuesta caught asociada a trainer pos x %d pos y %d", trainer->pos_x, trainer->pos_y);
			return trainer;
		}
	}
	return NULL;
}


