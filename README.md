# GOFSM

**GOFSM (Goal-Oriented Finite State Machine)** is a control paradigm where the developer specifies only the target state, and the automaton plans and executes the path to it. Instead of embedding logic in states, GOFSM moves all active logic into transitions, which can perform checks, delays, and side effects. States themselves act only as checkpoints without internal logic.

## This Repository

This repository contains a generic GOFSM implementation used in a local project. It is not intended for ongoing maintenance or feature expansion.

## Why Use GOFSM

- **Flexibility**: Dynamically block/unblock transitions and change goals at runtime.
- **Goal-Driven Planning**: Automatically finds the shortest path to the target via reverse BFS.
- **Repeatable Transitions**: Transition functions can return `Failure` to implement waiting or retries.
- **Centralized Logic**: All behavior logic resides in transitions, simplifying debugging and modularity.
- **Lightweight**: Minimal dependencies, suitable for embedded systems.

## When to Avoid

- **Overkill for simple FSMs**: For few states, a `switch–case` implementation is more efficient.
- **Resource Constraints**: Uses O(N+E) memory for buffers; node count limited to 255.
- **Non-deterministic Timing**: Reverse BFS on goal change is O(N+E), which can introduce jitter in hard real-time systems.

## Quick Start

Two initialization methods are available:

1. **Dynamic Allocation** via `GOFSM_Init` (heap-based buffers)
2. **Static Allocation** via `GOFSM_STATIC_ALLOCATE` (compile-time buffers)

### 1. Dynamic Allocation

```c
#include "gofsm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Define states
enum {
    STATE_IDLE = 0,
    STATE_WORK,
    STATE_DONE
};

// Transition functions
GOFSM_Transition_Result_t toWork(GOFSM_Transition_t* t) {
    printf("Transition to WORK\n");
    return GOFSM_Transition_Result_Success;
}
GOFSM_Transition_Result_t toDone(GOFSM_Transition_t* t) {
    printf("Transition to DONE\n");
    return GOFSM_Transition_Result_Success;
}

int main(void) {
    GOFSM_t fsm;

    // Initialize with dynamic buffers
    GOFSM_Init(&fsm, /* transitions_capacity = */ 2, /* nodes_capacity = */ 3);

    // Configure transitions
    GOFSM_Transition_t t1, t2;
    GOFSM_Transition_Init(&t1, STATE_IDLE, STATE_WORK, toWork);
    GOFSM_Transition_Init(&t2, STATE_WORK, STATE_DONE, toDone);
    GOFSM_AddTransition(&fsm, &t1);
    GOFSM_AddTransition(&fsm, &t2);

    // Set current and target
    GOFSM_SetCurrent(&fsm, STATE_IDLE);
    GOFSM_SetTarget(&fsm, STATE_DONE);

    // Run the automaton
    while (fsm.current_node_index != STATE_DONE) {
        GOFSM_OnTick(&fsm);
    }

    // Clean up
    GOFSM_Deinit(&fsm);
    return 0;
}
```

### 2. Static Allocation

```c
#include "gofsm.h"
#include <stdio.h>
#include <stdint.h>

// Statically allocate FSM and buffers at compile time
enum {
    STATE_IDLE = 0,
    STATE_WORK,
    STATE_DONE
};

GOFSM_STATIC_ALLOCATE(  /* STORAGE: */ static,
                     fsm_static, /* name */
                     2,          /* transitions */
                     3           /* nodes */
);

// Transition functions
GOFSM_Transition_Result_t toWork(GOFSM_Transition_t* t) {
    printf("Transition to WORK\n");
    return GOFSM_Transition_Result_Success;
}
GOFSM_Transition_Result_t toDone(GOFSM_Transition_t* t) {
    printf("Transition to DONE\n");
    return GOFSM_Transition_Result_Success;
}

int main(void) {
    // Initialize static FSM
    GOFSM_InitStatic(&fsm_static);

    // Configure transitions
    GOFSM_Transition_t t1, t2;
    GOFSM_Transition_Init(&t1, STATE_IDLE, STATE_WORK, toWork);
    GOFSM_Transition_Init(&t2, STATE_WORK, STATE_DONE, toDone);
    GOFSM_AddTransition(&fsm_static, &t1);
    GOFSM_AddTransition(&fsm_static, &t2);

    // Set current and target
    GOFSM_SetCurrent(&fsm_static, STATE_IDLE);
    GOFSM_SetTarget(&fsm_static, STATE_DONE);

    // Run the automaton
    while (fsm_static.current_node_index != STATE_DONE) {
        GOFSM_OnTick(&fsm_static);
    }
    return 0;
}
```

## API Reference

```c
#define GOFSM_STATIC_ALLOCATE(
    STORAGE,  // storage specifier, e.g., static or extern (optional)
    name,     // instance identifier
    TCOUNT,   // max number of transitions (uint8_t)
    NCOUNT    // max number of nodes (uint8_t)
)
```

`GOFSM_STATIC_ALLOCATE` performs full static initialization: it declares

```c
GOFSM_Transition_t* name##_transitions[TCOUNT];
GOFSM_Node_Index_t    name##_alg_nodes_buffer[NCOUNT];
GOFSM_t               name;
```

all at compile time, without using `malloc()`. Use this in environments where
dynamic allocation is not allowed. The `STORAGE` parameter sets the C storage class
(`static`, `extern`, or empty for global scope).

```c
void GOFSM_InitStatic(
    GOFSM_t* fsm // pointer to instance created by GOFSM_STATIC_ALLOCATE
);
```

Completes static setup by:
- Resetting `current_node_index` and `target_node_index` to 0
- Clearing the list of transitions
- Setting `is_target_change` and `is_graph_reconfigured` flags
- Preparing FSM for its first ticks

> Call `GOFSM_InitStatic()` **only** on instances created via `GOFSM_STATIC_ALLOCATE()`.

```c
void GOFSM_Init(
    GOFSM_t* fsm,                  // pointer to a GOFSM instance
    uint8_t  transitions_capacity, // max number of transitions
    uint8_t  nodes_capacity        // max number of nodes
);
```

Performs dynamic initialization by allocating internal buffers:
- `transitions` array
- `alg_nodes_buffer`

Also resets `current_node_index`, `target_node_index`, clears transition count,
and sets `is_target_change` & `is_graph_reconfigured` flags.

```c
void GOFSM_Deinit(
    GOFSM_t* fsm // dynamically initialized instance
);
```

Frees buffers allocated by `GOFSM_Init()`. Safe to call multiple times. Has no effect
on statically initialized instances.

```c
void GOFSM_Transition_Init(
    GOFSM_Transition_t*         transition, // pointer to transition struct
    GOFSM_Node_Index_t          src,        // source node index (0–255)
    GOFSM_Node_Index_t          dst,        // destination node index
    GOFSM_Transition_Function_t fn          // transition function (NULL for unconditional)
);
```

Initializes a transition from `src` to `dst` using `fn`. If `fn == NULL`, the transition
is unconditional and occurs immediately when its source node is reached.

```c
void GOFSM_Transition_SetState(
    GOFSM_t*                 fsm,        // FSM instance
    GOFSM_Transition_t*      transition, // transition to block/unblock
    GOFSM_Transition_State_t state       // Blocked or Available
);
```

Blocks or unblocks the given transition, causing the pathfinder to include or ignore it.

```c
GOFSM_Error_t GOFSM_AddTransition(
    GOFSM_t*            fsm,        // FSM instance
    GOFSM_Transition_t* transition // transition to register
);
```

Adds a transition to the FSM and marks the graph as changed. Returns:
- `GOFSM_Error_No` on success
- `GOFSM_Error_OwerstackTransitions` if capacity exceeded

```c
GOFSM_Error_t GOFSM_RemoveTransition(
    GOFSM_t*            fsm,        // FSM instance
    GOFSM_Transition_t* transition // transition to remove
);
```

Removes a transition and marks the graph as changed. Returns:
- `GOFSM_Error_No` on success
- `GOFSM_Error_NotRegisteredTransition` if the transition was not found

```c
void GOFSM_SetCurrent(
    GOFSM_t*            fsm,  // FSM instance
    GOFSM_Node_Index_t  node  // new current node
);
```

Sets the current node (graph vertex) of the FSM. This is the starting point for path planning.

```c
void GOFSM_SetTarget(
    GOFSM_t*            fsm,  // FSM instance
    GOFSM_Node_Index_t  node  // goal node
);
```

Sets the target node. Triggers a new path search on the next tick.

```c
void GOFSM_OnTick(
    GOFSM_t* fsm // FSM instance
);
```

Performs one automaton step:
1. If `current == target`, returns immediately.
2. If a previous transition failed or the target/graph changed, recomputes the next step via reverse BFS.
3. Executes the chosen transition's function:
   - On `Success`, updates `current_node_index` to the transition's destination.
   - On `Failure`, retains the transition for retry on the next tick.

Transition conditions are defined by user-provided functions (`GOFSM_Transition_Function_t`),
enabling checks on external signals, timers, etc.

## Implementation Details

- Uses a single buffer (`alg_nodes_buffer`) of size N to store **working**, **planned**, and **visited** nodes.
- Reverse BFS explores from the target toward the current node, finding the nearest available predecessor.
- Memory complexity: O(N+E). Time complexity per change: O(N+E).
- Only the next step is discovered (not the full path), minimizing memory usage.

## Possible Enhancements

- **Precomputed cost table**: Build a cost matrix `cost_table[N][N]` for O(1) next-step lookup at the cost of O(N²) memory.
- **Support for self-transitions**: Handle `A → A` transitions explicitly for idle behaviors.
- **Transition cache**: Maintain a cache of computed steps to reduce BFS frequency.

---

*Documentation generated with ChatGPT.*

