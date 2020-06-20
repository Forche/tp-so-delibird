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

		if (pcb_trainer->result_catch) { //Tiene basura y sale por true
			add_to_dictionary(trainer->caught, pcb_trainer->pokemon_to_catch->pokemon);
			log_info(logger, "Atrapado pokemon %s", pcb_trainer->pokemon_to_catch->pokemon);
		} else {
			log_info(logger, "Error atrapar pokemon %s", pcb_trainer->pokemon_to_catch->pokemon);
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
