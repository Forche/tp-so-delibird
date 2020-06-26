#include "trainer.h"

void handle_trainer(t_trainer* trainer) {
	trainer->thread = pthread_self();
	trainer->pcb_trainer->status = NEW;
	pthread_mutex_init(&trainer->sem, NULL);
	pthread_mutex_lock(&trainer->sem); // Para que quede en 0.
	pthread_mutex_init(&trainer->pcb_trainer->sem_caught, NULL);
	pthread_mutex_lock(&trainer->pcb_trainer->sem_caught);

	while(1) {
		pthread_mutex_lock(&trainer->sem);

		t_pcb_trainer* pcb_trainer = trainer->pcb_trainer;
		pcb_trainer->status = EXEC;

		//TODO Emular tiempo de CPU
		move_to_position(trainer, pcb_trainer->pokemon_to_catch->pos_x, pcb_trainer->pokemon_to_catch->pos_y);
		catch_pokemon(trainer, pcb_trainer->pokemon_to_catch);

		if (pcb_trainer->result_catch) { //Tiene basura y sale por true
			add_to_dictionary(trainer->caught, pcb_trainer->pokemon_to_catch->pokemon);

			pthread_mutex_lock(&mutex_caught_pokemons);
			add_to_dictionary(caught_pokemons, pcb_trainer->pokemon_to_catch->pokemon);
			pthread_mutex_unlock(&mutex_caught_pokemons);

			pthread_mutex_lock(&mutex_remaining_pokemons);
			substract_from_dictionary(remaining_pokemons, pcb_trainer->pokemon_to_catch->pokemon);
			pthread_mutex_unlock(&mutex_remaining_pokemons);

			log_info(logger, "Atrapado pokemon %s", pcb_trainer->pokemon_to_catch->pokemon);
		} else {
			log_info(logger, "Error atrapar pokemon %s", pcb_trainer->pokemon_to_catch->pokemon);
		}

		pthread_mutex_lock(&mutex_being_caught_pokemons);
		substract_from_dictionary(being_caught_pokemons, pcb_trainer->pokemon_to_catch->pokemon);
		pthread_mutex_unlock(&mutex_being_caught_pokemons);

		if(is_trainer_full(trainer)) {
			change_status_to(trainer, FULL);
			validate_deadlock(trainer);

			if(list_size(remaining_pokemons) == 0) {
				sem_post(&sem_all_pokemons_caught);
			}
		} else {
			trainer->pcb_trainer->status = NEW;
			change_status_to(trainer, BLOCK);
			sem_post(&sem_trainer_available);
		}

		pthread_mutex_unlock(&mutex_planning);

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
				log_info(logger, "Deadlock directo entre %s y %s", pokemon, pokemon_he_needs);
				list_add(matched_deadlocks, deadlock_match);
				list_remove(leftovers, pokemon_he_needs);
				list_remove(remaining, pokemon);
				list_remove(has_pokemon_i_need->leftovers, pokemon);
				list_remove(has_pokemon_i_need->remaining, pokemon_he_needs);
			} else {
				//NO MATCHEO
			}
		} else {
			//NO MATCHEO
		}
	}
}

t_to_deadlock* get_leftover_from_to_deadlock_by_pokemon(char* pokemon) {
	bool get_by_pokemon(void* element) {
		return string_equals_ignore_case(pokemon, element);
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
		log_info(logger, "buscando deadlock directos");
		//ESTA MURIENDO ACA ADENTRO
		find_candidate_to_swap(remaining, leftovers, trainer);
		t_to_deadlock* trainer_to_deadlock = malloc(sizeof(t_to_deadlock));
		trainer_to_deadlock->trainer = trainer;
		trainer_to_deadlock->leftovers = leftovers;
		trainer_to_deadlock->remaining = remaining;
		log_info(logger, "terminado busqueda deadlock directa");
		list_add(to_deadlock, trainer_to_deadlock);
	}

}

bool is_trainer_full(t_trainer* trainer) {
	uint32_t q_caught = dictionary_add_values(trainer->caught);
	uint32_t q_objective = dictionary_add_values(trainer->objective);

	log_info(logger, "Cantidad atrapados: %d. Cantidad objetivo: %d", q_caught, q_objective);

	if(q_caught - q_objective) {
		log_info(logger, "Todavia tengo que atrapar");
		return false;
	} else {
		log_info(logger, "Toy lleno");
		return true;
	}
}


void change_status_to(t_trainer* trainer, status status) {
	trainer->status = status;
}

void move_to_position(t_trainer* trainer, uint32_t pos_x, uint32_t pos_y) {
	log_info(logger, "Moviendo entrenador de posicion x: %d y: %d", trainer->pos_x, trainer->pos_y);
	trainer->pos_x = pos_x;
	trainer->pos_y = pos_y;
	log_info(logger, "Movido entrenador a posicion x: %d y: %d", pos_x, pos_y);
}

void catch_pokemon(t_trainer* trainer, t_appeared_pokemon* appeared_pokemon) {
	t_catch_pokemon* pokemon_to_catch = malloc(sizeof(t_catch_pokemon));
	pokemon_to_catch->pos_x = appeared_pokemon->pos_x;
	pokemon_to_catch->pos_y = appeared_pokemon->pos_y;
	pokemon_to_catch->pokemon_len = appeared_pokemon->pokemon_len;
	pokemon_to_catch->pokemon = appeared_pokemon->pokemon;

	t_buffer* buffer = serialize_t_catch_pokemon_message(pokemon_to_catch);

	log_info(logger, "Send catch a pokemon %s, bloqueado trainer pos x: %d pos y: %d", appeared_pokemon->pokemon,
			trainer->pos_x, trainer->pos_y);

	uint32_t broker_msg_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_msg_connection == -1) {
		log_error(logger, "Trainer: Error de comunicacion con el broker, realizando operacion default");
		trainer->pcb_trainer->result_catch = 1;
	} else {
		trainer->pcb_trainer->msg_connection = broker_msg_connection;
		send_message(broker_msg_connection, CATCH_POKEMON, NULL, NULL, buffer);
		pthread_t thread = create_thread_with_param(receive_message_id, trainer, "receive_message_id");
		change_status_to(trainer, BLOCK);
		pthread_mutex_unlock(&mutex_planning);

		pthread_mutex_lock(&trainer->pcb_trainer->sem_caught);

		pthread_mutex_lock(&mutex_planning);
		change_status_to(trainer, EXEC);
	}

}

void receive_message_id(t_trainer* trainer) {
	listen(trainer->pcb_trainer->msg_connection, SOMAXCONN);
	uint32_t msg_id;
	recv(trainer->pcb_trainer->msg_connection, &msg_id, sizeof(uint32_t), MSG_WAITALL);
	trainer->pcb_trainer->id_message = msg_id;
	log_info(logger, "Id del mensaje %d", msg_id);
	close(trainer->pcb_trainer->msg_connection);
}


t_trainer* obtener_trainer_mensaje(t_message* msg) {
	uint32_t i;
	for(i = 0; i < list_size(trainers); i++) {
		t_trainer* trainer = list_get(trainers, i);
		if(trainer->pcb_trainer->id_message == msg->correlative_id) {
			log_info(logger, "Respuesta caught asociada a trainer pos x %d pos y %d", trainer->pos_x, trainer->pos_y);
			return trainer;
		}
	}
	return NULL;
}


uint32_t dictionary_add_values(t_dictionary *self) {
	int table_index;
	uint32_t value = 0;
	for (table_index = 0; table_index < self->table_max_size; table_index++) {
		t_hash_element *element = self->elements[table_index];

		while (element != NULL) {
			uint32_t* data = element->data;
			value += (*data);
			element = element->next;
		}
	}
	return value;
}
