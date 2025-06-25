#ifndef GOFSM_H
#define GOFSM_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

//#define GOFSM_ASSERT_ENABLED
#ifdef GOFSM_ASSERT_ENABLED
#include "stm32F4xx.h"
#define GOFSM_ASSERT(expr) if(!(expr)){__BKPT(0);}
#else
#define GOFSM_ASSERT(expr) ((void)0)
#endif

// Строго не предполагается использование на больших графах
// По этому установлено жёсткое ограничение на 255 нод
typedef uint8_t GOFSM_Node_Index_t;

typedef enum{
	GOFSM_Transition_Result_Failure = 0,
	GOFSM_Transition_Result_Success = 1
}GOFSM_Transition_Result_t;

typedef enum{
	GOFSM_Transition_State_Blocked = 0,
	GOFSM_Transition_State_Available = 1
}GOFSM_Transition_State_t;

typedef enum{
	GOFSM_Error_No = 0,
	GOFSM_Error_OwerstackTransitions = 1,
	GOFSM_Error_NotRegisteredTransition = 2,
}GOFSM_Error_t;

struct GOFSM_Transition_t;
typedef struct GOFSM_Transition_t GOFSM_Transition_t;
typedef GOFSM_Transition_Result_t (*GOFSM_Transition_Function_t)(GOFSM_Transition_t*);
struct __attribute__((packed)) GOFSM_Transition_t {
	GOFSM_Node_Index_t source_node_index;
	GOFSM_Node_Index_t destination_node_index;
	GOFSM_Transition_Function_t function;
	GOFSM_Transition_State_t state;
};

typedef struct __attribute__((packed)){
	uint8_t nodes_capacity;
	uint8_t transitions_count;
	uint8_t transitions_capacity;
	GOFSM_Transition_t** transitions;
	GOFSM_Transition_t* transition_current;
	GOFSM_Node_Index_t current_node_index;
	GOFSM_Node_Index_t target_node_index;
	GOFSM_Node_Index_t* alg_nodes_buffer;
	uint8_t is_target_change;
	uint8_t is_transition_failure;
	uint8_t is_graph_reconfigured;
	uint8_t is_dyn;
}GOFSM_t;

#define GOFSM_STATIC_ALLOCATE(STORAGE, name, TCOUNT, NCOUNT)    \
	STORAGE GOFSM_Transition_t* name##_transitions[TCOUNT];     \
    STORAGE GOFSM_Node_Index_t name##_alg_nodes_buffer[NCOUNT]; \
    STORAGE GOFSM_t name = {                                    \
        .nodes_capacity       = (NCOUNT),                       \
        .transitions_capacity = (TCOUNT),                       \
        .transitions          = name##_transitions,             \
        .alg_nodes_buffer     = name##_alg_nodes_buffer         \
    }

// Использовать строго для экземпляров созданных через GOFSM_STATIC_ALLOCATE()
void GOFSM_InitStatic(GOFSM_t* gofsm);

// Динамическая инициализация
void GOFSM_Init(GOFSM_t* gofsm, uint8_t transitions_capacity, uint8_t nodes_capacity);
void GOFSM_Deinit(GOFSM_t* gofsm);

void GOFSM_Transition_Init(GOFSM_Transition_t* transition, GOFSM_Node_Index_t source_node_index, GOFSM_Node_Index_t destination_node_index, GOFSM_Transition_Function_t function);
void GOFSM_Transition_SetState(GOFSM_t* gofsm, GOFSM_Transition_t* transition, GOFSM_Transition_State_t state);

GOFSM_Error_t GOFSM_AddTransition(GOFSM_t* gofsm, GOFSM_Transition_t* transition);
GOFSM_Error_t GOFSM_RemoveTransition(GOFSM_t* gofsm, GOFSM_Transition_t* transition);

void GOFSM_SetCurrent(GOFSM_t* gofsm, GOFSM_Node_Index_t node_index);
void GOFSM_SetTarget(GOFSM_t* gofsm, GOFSM_Node_Index_t node_index);

void GOFSM_OnTick(GOFSM_t* gofsm);


#endif
