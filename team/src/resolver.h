#ifndef SRC_RESOLVER_H_
#define SRC_RESOLVER_H_

#include"team.h"

void proceed_to_finish();
t_list* get_trainers_with_deadlock();
bool trainer_not_exit(t_trainer* trainer);
void handle_deadlock(t_list* trainers_with_deadlock);
t_list* get_dictionary_difference(t_dictionary* a, t_dictionary* b);
//void swap_pokemons(t_deadlock_matcher* deadlock_matcher);

#endif /* SRC_RESOLVER_H_ */
