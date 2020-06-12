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

		//Propuesta para saber si lo que sigue es atrapar o manejar deadlock: trainer->hacer_lo_que_sigue();

		t_pcb_trainer* pcb_trainer = trainer->pcb_trainer;
		pcb_trainer->status = EXEC;
		move_to_position(trainer, pcb_trainer->pokemon_to_catch->pos_x, pcb_trainer->pokemon_to_catch->pos_y);
		catch_pokemon(trainer, pcb_trainer->pokemon_to_catch);
		log_info(logger, "Send catch, bloqueado trainer pos x: %d pos y: %d", trainer->pos_x, trainer->pos_y);

		trainer->pcb_trainer->id_message = 10;

		change_status_to(trainer, BLOCK);
		pthread_mutex_unlock(&mutex_planning);

		pthread_mutex_lock(&pcb_trainer->sem_caught);

		pthread_mutex_lock(&mutex_planning);
		change_status_to(trainer, EXEC);

		if (pcb_trainer->result_catch) {
			add_to_dictionary(trainer->caught, pcb_trainer->pokemon_to_catch->pokemon);
			log_info(logger, "Atrapado perri");
		} else {
			// No se puede atrapar el pokemon
		}

		trainer->pcb_trainer->status = NEW;
		//TODO Manejar deadlock

		change_status_to(trainer, BLOCK);
		sem_post(&sem_trainer_available);
		pthread_mutex_unlock(&mutex_planning);
	}
}


void change_status_to(t_trainer* trainer, status status) {
	trainer->status = status;
}

void move_to_position(t_trainer* trainer, uint32_t pos_x, uint32_t pos_y) {
	trainer->pos_x = pos_x;
	trainer->pos_y = pos_y;
}

void catch_pokemon(t_trainer* trainer, t_appeared_pokemon* appeared_pokemon) {
	t_catch_pokemon* pokemon_to_catch = malloc(sizeof(t_catch_pokemon));
	pokemon_to_catch->pos_x = appeared_pokemon->pos_x;
	pokemon_to_catch->pos_y = appeared_pokemon->pos_y;
	pokemon_to_catch->pokemon_len = appeared_pokemon->pokemon_len;
	pokemon_to_catch->pokemon = appeared_pokemon->pokemon;

	t_buffer* buffer = serialize_t_catch_pokemon_message(pokemon_to_catch);

	send_message(broker_connection, CATCH_POKEMON, NULL, NULL, buffer);
	struct sockaddr_in client_addr;
	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t client_socket;
	uint32_t msg_id;
	if ((client_socket = accept((*server_socket), (void*) &client_addr, &addr_size))
			!= -1) {
		recv(client_socket, &msg_id, sizeof(event_code), MSG_WAITALL);
		trainer->pcb_trainer->id_message = msg_id;
		log_info(logger, "Id del mensaje %d", msg_id);
	}
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
