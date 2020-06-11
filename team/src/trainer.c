#include "trainer.h"

void handle_trainer(t_trainer* trainer) {
	trainer->thread = pthread_self();
	pthread_mutex_init(&trainer->sem, NULL);
	pthread_mutex_lock(&trainer->sem); // Para que quede en 0.
	pthread_mutex_init(&trainer->pcb_trainer->sem_caught, NULL);
	pthread_mutex_lock(&trainer->pcb_trainer->sem_caught);

	while(1) {
		pthread_mutex_lock(&trainer->sem);

		//Propuesta para saber si lo que sigue es atrapar o manejar deadlock: trainer->hacer_lo_que_sigue();

		t_pcb_trainer* pcb_trainer = trainer->pcb_trainer;

		move_to_position(trainer, pcb_trainer->pokemon_to_catch->pos_x, pcb_trainer->pokemon_to_catch->pos_y);
		//trainer->pcb_trainer->id_message = catch_pokemon(trainer, pcb_trainer->pokemon_to_catch);
		log_info(logger, "Send catch, bloqueado");

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

		trainer->pcb_trainer = NULL;
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

void catch_pokemon(t_trainer trainer, t_appeared_pokemon* appeared_pokemon) {
	t_catch_pokemon* pokemon_to_catch = malloc(sizeof(t_catch_pokemon));
	pokemon_to_catch->pos_x = appeared_pokemon->pos_x;
	pokemon_to_catch->pos_y = appeared_pokemon->pos_y;
	pokemon_to_catch->pokemon_len = appeared_pokemon->pokemon_len;
	pokemon_to_catch->pokemon = appeared_pokemon->pokemon;

	t_buffer* buffer = serialize_t_catch_pokemon_message(pokemon_to_catch);

	send_message(broker_connection, CATCH_POKEMON, NULL, NULL, buffer);
}
