#include "trainer.h"

void handle_trainer(t_trainer* trainer) {
	trainer->thread = pthread_self();
	pthread_mutex_init(&trainer->sem, NULL);
	pthread_mutex_lock(&trainer->sem); // Para que quede en 0.


	while(1) {
		pthread_mutex_lock(&trainer->sem);

		sleep(5);
		log_info(logger, "Ejecutado entrenador en: Pos x: %d, Pos y: %d", trainer->pos_x, trainer->pos_y);

		//Atrapa el pokemon o resuelve deadlock
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

void catch_pokemon(t_trainer trainer, t_appeared_pokemon* appeared_pokemon, uint32_t conection) {
	t_catch_pokemon* pokemon_to_catch = malloc(sizeof(t_catch_pokemon));
	pokemon_to_catch->pos_x = appeared_pokemon->pos_x;
	pokemon_to_catch->pos_y = appeared_pokemon->pos_y;
	pokemon_to_catch->pokemon_len = appeared_pokemon->pokemon_len;
	pokemon_to_catch->pokemon = appeared_pokemon->pokemon;

	t_buffer* buffer = serialize_t_catch_pokemon_message(pokemon_to_catch);

	send_message(conection, CATCH_POKEMON, NULL, NULL, buffer);
}
