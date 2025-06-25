#include <GOFSM/gofsm.h>

GOFSM_Transition_t* GOFSM_SearchNextStep(GOFSM_t* gofsm){
	GOFSM_Node_Index_t target = gofsm->target_node_index;
	GOFSM_Node_Index_t current = gofsm->current_node_index;

	uint8_t index_planned = 0;
	uint8_t planned_length = 1;
	uint8_t visited_length = 1;
	gofsm->alg_nodes_buffer[index_planned] = target;

	while(planned_length){
		GOFSM_Node_Index_t* working = gofsm->alg_nodes_buffer+index_planned;
		uint8_t working_length = planned_length;

		GOFSM_Node_Index_t* planned = gofsm->alg_nodes_buffer+index_planned+planned_length;
		planned_length = 0;

		GOFSM_Node_Index_t* visited = gofsm->alg_nodes_buffer;

		index_planned+=working_length;

		for(uint8_t i=0; i<working_length; i++){// пробегаем по нодам
			GOFSM_Node_Index_t node = working[i];

			// для каждой ноды надо найти соседей
			for(uint8_t j=0; j<gofsm->transitions_count; j++){
				GOFSM_Transition_t* transition = gofsm->transitions[j];
				if(transition->destination_node_index==node){
					if(transition->state==GOFSM_Transition_State_Available){
						GOFSM_Node_Index_t prev_node = transition->source_node_index;

						// предварительная проверка
						if(prev_node==current)
							return transition;

						// определяем факт посещения
						uint8_t is_visited = 0;
						for(uint8_t k=0; k<visited_length; k++){
							if(visited[k]==prev_node){
								is_visited=1;
								break;
							}
						}
						if(!is_visited){
							planned[planned_length] = prev_node;
							planned_length++;
							visited_length++;
						}
					}
				}
			}
		}
	}
	return NULL;
}

void GOFSM_Init(GOFSM_t* gofsm, uint8_t transitions_capacity, uint8_t nodes_capacity){
	gofsm->is_dyn = 1;

	gofsm->transitions_capacity = transitions_capacity;
	gofsm->nodes_capacity = nodes_capacity;

	gofsm->transitions = (GOFSM_Transition_t**)malloc(transitions_capacity * sizeof(GOFSM_Transition_t*));
	gofsm->alg_nodes_buffer = (GOFSM_Node_Index_t*)malloc(nodes_capacity * sizeof(GOFSM_Node_Index_t));

	GOFSM_InitStatic(gofsm);
}
void GOFSM_Deinit(GOFSM_t* gofsm){
	if(!gofsm->is_dyn) return;
	free(gofsm->transitions);
	free(gofsm->alg_nodes_buffer);
}
void GOFSM_InitStatic(GOFSM_t* gofsm){
	GOFSM_ASSERT(gofsm!=NULL);
	GOFSM_ASSERT(gofsm->transitions!=NULL);
	GOFSM_ASSERT(gofsm->alg_nodes_buffer!=NULL);
	gofsm->current_node_index = 0;
	gofsm->transitions_count = 0;
	gofsm->transition_current = NULL;
	for(uint8_t i=0; i<gofsm->transitions_capacity; i++)
		gofsm->transitions[i] = 0;
	gofsm->is_target_change = 1;
	gofsm->is_transition_failure = 0;
	gofsm->is_graph_reconfigured = 1;
}


void GOFSM_Transition_Init(GOFSM_Transition_t* transition, GOFSM_Node_Index_t source_node_index, GOFSM_Node_Index_t destination_node_index, GOFSM_Transition_Function_t function){
	GOFSM_ASSERT(transition!=NULL);
	transition->source_node_index = source_node_index;
	transition->destination_node_index = destination_node_index;
	transition->function = function;
	transition->state = GOFSM_Transition_State_Available;
}
void GOFSM_Transition_SetState(GOFSM_t* gofsm, GOFSM_Transition_t* transition, GOFSM_Transition_State_t state){
	GOFSM_ASSERT(transition!=NULL);
	transition->state = state;
	gofsm->is_graph_reconfigured = 1;
}

GOFSM_Error_t GOFSM_AddTransition(GOFSM_t* gofsm, GOFSM_Transition_t* transition){
	GOFSM_ASSERT(gofsm!=NULL);
	GOFSM_ASSERT(transition!=NULL);
	if(gofsm->transitions_count == gofsm->transitions_capacity)
		return GOFSM_Error_OwerstackTransitions;

	gofsm->transitions[gofsm->transitions_count] = transition;
	gofsm->transitions_count++;
	gofsm->is_graph_reconfigured = 1;
	return GOFSM_Error_No;
}
GOFSM_Error_t GOFSM_RemoveTransition(GOFSM_t* gofsm, GOFSM_Transition_t* transition){
	GOFSM_ASSERT(gofsm!=NULL);
	GOFSM_ASSERT(transition!=NULL);
    for(uint32_t i=0; i<gofsm->transitions_count; i++)
        if(gofsm->transitions[i]==transition) {
        	uint32_t remaining = gofsm->transitions_count - i - 1;
        	memmove(gofsm->transitions+i, gofsm->transitions+i+1, remaining * sizeof(GOFSM_Transition_t*));
        	gofsm->transitions_count--;
            return GOFSM_Error_No;
        }
    return GOFSM_Error_NotRegisteredTransition;
}

void GOFSM_SetCurrent(GOFSM_t* gofsm, GOFSM_Node_Index_t node_index){
	GOFSM_ASSERT(gofsm!=NULL);
	gofsm->current_node_index = node_index;
	gofsm->is_target_change = 1;
}
void GOFSM_SetTarget(GOFSM_t* gofsm, GOFSM_Node_Index_t node_index){
	GOFSM_ASSERT(gofsm!=NULL);
	gofsm->target_node_index = node_index;
	gofsm->is_target_change = 1;
}

void GOFSM_OnTick(GOFSM_t* gofsm){
	GOFSM_ASSERT(gofsm!=NULL);
	if(gofsm->current_node_index==gofsm->target_node_index){
		return;
	}
	if(!gofsm->is_transition_failure || gofsm->is_target_change || gofsm->is_graph_reconfigured){
		gofsm->transition_current = GOFSM_SearchNextStep(gofsm);
		GOFSM_ASSERT(gofsm->transition_current!=NULL);
		gofsm->is_target_change = 0;
		gofsm->is_graph_reconfigured = 0;
	}
	GOFSM_Transition_t* transition = gofsm->transition_current;
	GOFSM_Transition_Result_t result = GOFSM_Transition_Result_Success;

	if(transition==NULL){
		gofsm->is_transition_failure = 0;
		return;
	}
	if(transition->function!=NULL){
		result = transition->function(transition);
	}
	gofsm->is_transition_failure = !result;

	if(result==GOFSM_Transition_Result_Success){
		gofsm->current_node_index = transition->destination_node_index;
	}
}
