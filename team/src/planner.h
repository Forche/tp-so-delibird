#ifndef SRC_PLANNER_H_
#define SRC_PLANNER_H_



#include<commons/collections/list.h>
#include "team.h"

void* planning_catch();
void* planning_deadlock();
double calculate_estimacion_actual_rafaga(t_trainer* trainer);

#endif /* SRC_PLANNER_H_ */
