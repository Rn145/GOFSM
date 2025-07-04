# GOFSM

GOFSM (Goal-Oriented Finite State Machine) — парадигма управления, в которой поведение системы строится на целеуказании: разработчик задаёт лишь целевое состояние, а автомат сам планирует и выполняет путь к нему. Вместо последовательного переключения состояний логика реализуется через переходы, которые могут выполнять проверки, задержки и побочные действия. Поведение самих состояний при этом не регламентируется.

## Текущий репозиторий

Данный репозиторий реализует вышеописанную парадигму GOFSM и использовался в рамках одного локального проекта. Это библиотека общего назначения, но проект не предполагается к сопровождению или развитию.

## Причины использовать

- **Гибкость**: возможность динамически блокировать/разблокировать переходы и изменять цели во время выполнения.
- **Целевое планирование**: автомат сам ищет кратчайший путь к цели (обратный BFS).
- **Повторяемость переходов**: функции-переходы могут возвращать `Failure` до выполнения условий (реализация задержек и ожиданий).
- **Централизация логики**: упрощает отладку и модульность — вся логика хранится в одном месте.
- **Лёгковесность**: минимальные зависимости, подходит для embedded-систем.

## Причины отказаться

- **Избыточность для простых FSM**: при небольшом числе состояний ручная реализация через `switch–case` будет оптимальнее.
- **Ограничения по ресурсам**: память O(N+E) для буфера и индексов; ограничение нод до 255.
- **Непредсказуемое время планирования**: при изменении цели выполняется BFS O(N+E), что может вызвать джиттер в жёстком real-time.

## Быстрый старт

В этом разделе приведены два способа инициализации:

1. **Динамическое выделение буферов** (через `GOFSM_Init`)
2. **Статическая аллокация** (через `GOFSM_STATIC_ALLOCATE` в глобальной области)

### 1. Динамическое выделение буферов

```c
#include "gofsm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Определяем состояния
enum {
    STATE_IDLE = 0,
    STATE_WORK,
    STATE_DONE
};

// Функции-переходы
GOFSM_Transition_Result_t toWork(GOFSM_Transition_t* t) {
    printf("Transition to WORK\n");
    return GOFSM_Transition_Result_Success;
}
GOFSM_Transition_Result_t toDone(GOFSM_Transition_t* t) {
    printf("Transition to DONE\n");
    return GOFSM_Transition_Result_Success;
}

int main(void) {

    // Инициализация с динамическим выделением буферов
    GOFSM_t fsm;
    GOFSM_Init(&fsm, /* transitions_capacity = */ 2, /* nodes_capacity = */ 3);

    // Добавление переходов
    GOFSM_Transition_t transition_toWork;
    GOFSM_Transition_t transition_toDone;
    GOFSM_Transition_Init(&transition_toWork, STATE_IDLE, STATE_WORK, toWork);
    GOFSM_Transition_Init(&transition_toDone, STATE_WORK, STATE_DONE, toDone);
    GOFSM_AddTransition(&fsm, &transition_toWork);
    GOFSM_AddTransition(&fsm, &transition_toDone);

    // Устанавливаем позицию и цель
    GOFSM_SetCurrent(&fsm, STATE_IDLE);
    GOFSM_SetTarget(&fsm, STATE_DONE);

    // Цикл работы автомата
    while (fsm.current_node_index != STATE_DONE) {
        GOFSM_OnTick(&fsm);
    }

    // Освобождаем ресурсы
    GOFSM_Deinit(&fsm);
    return 0;
}
```

### 2. Статическая аллокация

(в глобальной области):

```c
#include "gofsm.h"
#include <stdio.h>
#include <stdint.h>

// Создаём экземпляр GOFSM и буферы на этапе компиляции
GOFSM_STATIC_ALLOCATE(static, fsm_static, 2, 3);

// Определяем состояния
enum {
    STATE_IDLE = 0,
    STATE_WORK,
    STATE_DONE
};

// Функции-переходы
GOFSM_Transition_Result_t toWork(GOFSM_Transition_t* t) {
    printf("Transition to WORK\n");
    return GOFSM_Transition_Result_Success;
}
GOFSM_Transition_Result_t toDone(GOFSM_Transition_t* t) {
    printf("Transition to DONE\n");
    return GOFSM_Transition_Result_Success;
}

int main(void) {
    // Инициализация статического экземпляра
    GOFSM_InitStatic(&fsm_static);

    // Добавление переходов
    GOFSM_Transition_t transition_toWork;
    GOFSM_Transition_t transition_toDone;
    GOFSM_Transition_Init(&transition_toWork, STATE_IDLE, STATE_WORK, toWork);
    GOFSM_Transition_Init(&transition_toDone, STATE_WORK, STATE_DONE, toDone);
    GOFSM_AddTransition(&fsm_static, &transition_toWork);
    GOFSM_AddTransition(&fsm_static, &transition_toDone);

    // Устанавливаем позицию и цель
    GOFSM_SetCurrent(&fsm_static, STATE_IDLE);
    GOFSM_SetTarget(&fsm_static, STATE_DONE);

    // Цикл работы автомата
    while (fsm_static.current_node_index != STATE_DONE) {
        GOFSM_OnTick(&fsm_static);
    }
    return 0;
}
```



## API

```c
#define GOFSM_STATIC_ALLOCATE(
    STORAGE,  // модификатор экземпляра, например static
    name,     // имя экземпляра GOFSM
    TCOUNT,   // максимальное число переходов (uint8_t)
    NCOUNT    // максимальное число узлов (uint8_t)
)

void GOFSM_InitStatic(
    GOFSM_t* fsm // указатель на ранее статически созданный экземпляр
)
```

`GOFSM_STATIC_ALLOCATE` — это макрос для полной статической инициализации автомата. Он создаёт экземпляр `GOFSM_t` и выделяет все необходимые внутренние буферы  **на этапе компиляции**, полностью исключая использование динамической памяти. Это делает его особенно удобным для embedded-систем, где важно детерминированное поведение и жёсткий контроль за памятью.

Параметр `STORAGE` задаёт спецификатор хранения, например `static` или может быть оставлен пустым — в зависимости от области видимости, где размещается автомат.

После использования макроса необходимо вызвать `GOFSM_InitStatic()` — этот вызов инициализирует автомат: сбрасывает текущее и целевое состояние, очищает список переходов, устанавливает флаги, и подготавливает автомат к работе.

> `GOFSM_InitStatic()` должен вызываться **только** для экземпляров, созданных через `GOFSM_STATIC_ALLOCATE()`.

```c
void GOFSM_Init(
    GOFSM_t* fsm,                  // указатель на экземпляр GOFSM
    uint8_t transitions_capacity,  // максимальное число переходов
    uint8_t nodes_capacity         // максимальное число узлов
)
```

Выделяет и инициализирует внутренние буферы автомата: массив указателей на переходы и служебный буфер для поиска маршрута. Вызывать только для экземпляров, созданных не статически (не через `GOFSM_STATIC_ALLOCATE`).

Инициализирует автомат: сбрасывает текущее и целевое состояние, очищает список переходов, устанавливает флаги, и подготавливает автомат к работе.

```c
void GOFSM_Deinit(
    GOFSM_t* fsm // указатель на экземпляр GOFSM
)
```

Освобождает память, выделенную при помощи `GOFSM_Init()`. Ничего не делает при повторном вызове или вызове к статически инициализированным экземплярам.

```c
void GOFSM_Transition_Init(
    GOFSM_Transition_t* transition,       // указатель на структуру перехода
    GOFSM_Node_Index_t src,               // индекс исходного узла (0–255)
    GOFSM_Node_Index_t dst,               // индекс целевого узла (0–255)
    GOFSM_Transition_Function_t fn        // функция логики перехода (или NULL)
)
```

Инициализирует переход между узлами `src` и `dst` с заданной функцией `fn`. Если `fn` равна `NULL`, переход считается мгновенным (безусловным) и выполняется автоматически при достижении узла-источника.

```c
void GOFSM_Transition_SetState(
    GOFSM_t* fsm,                         // указатель на экземпляр GOFSM
    GOFSM_Transition_t* transition,       // указатель на переход
    GOFSM_Transition_State_t state        // состояние: Blocked/Available
)
```

Блокирует или разблокирует указанный переход. Строго устанавливает, будет ли переход учитываться при расчёте пути.

```c
GOFSM_Error_t GOFSM_AddTransition(
    GOFSM_t* fsm,                         // указатель на GOFSM
    GOFSM_Transition_t* transition        // переход для регистрации
)
```

Добавляет указанный переход в автомат.&#x20;

Возвращает `GOFSM_Error_No`, если переход успешно добавлен, или `GOFSM_Error_OwerstackTransitions`, если достигнут предел по количеству зарегистрированных переходов.

```c
GOFSM_Error_t GOFSM_RemoveTransition(
    GOFSM_t* fsm,                         // указатель на GOFSM
    GOFSM_Transition_t* transition        // переход для удаления
)
```

Удаляет ранее зарегистрированный переход из автомата.&#x20;

Возвращает `GOFSM_Error_No`, если удаление прошло успешно, или `GOFSM_Error_NotRegisteredTransition`, если указанный переход не был найден.

```c
void GOFSM_SetCurrent(
    GOFSM_t* fsm,                         // указатель на GOFSM
    GOFSM_Node_Index_t node               // новый текущий узел
)
```

Устанавливает текущее состояние (ноду, вершину графа) автомата. Это состояние служит точкой отсчёта для построения маршрута к целевому узлу при выполнении переходов.

```c
void GOFSM_SetTarget(
    GOFSM_t* fsm,                         // указатель на GOFSM
    GOFSM_Node_Index_t node               // узел-цель
)
```

Устанавливает целевое состояние (ноду, вершину графа), к которой автомат должен дойти. Это состояние используется в качестве конечной точки в маршруте, который GOFSM будет строить от текущей позиции.

```c
void GOFSM_OnTick(
    GOFSM_t* fsm // указатель на GOFSM
)
```

Выполняет один шаг автомата:

1. Если текущее состояние (`current_node_index`) совпадает с целью (`target_node_index`), функция завершает выполнение — маршрут достигнут.
2. Если ранее произошёл отказ перехода (`Failure`) либо была изменена цель (`is_target_change`) или граф (`is_graph_reconfigured`), GOFSM заново вычисляет путь к цели, начиная от текущего состояния. Для этого вызывается внутренний планировщик, который выбирает ближайший доступный переход, ведущий в сторону цели.
3. Выбранный переход сохраняется в `transition_current`, после чего вызывается его функция (`transition->function`).
   - Если функция возвращает `GOFSM_Transition_Result_Success`, автомат обновляет `current_node_index`, переходя в новое состояние.
   - Если возвращается `Failure`, переход сохраняется как текущий, и попытка будет повторена при следующем вызове `GOFSM_OnTick()`.

Условия переходов реализуются через пользовательские функции (`GOFSM_Transition_Function_t`), которые могут проверять внешние сигналы, таймеры или другие параметры. Таким образом, GOFSM может «ждать», пока переход не станет возможным, и только после этого продолжать движение к цели.

## Факты

- Эта реализация использует оптимизированный поиск в ширину (BFS), который выполняется в обратную сторону — от целевого состояния к текущему.
- Для этого применяется **один общий буфер**, равный количеству нод, где одновременно размещаются `working`, `planned` и `visited`.
- Из-за отсутствия обратного прохода, планировщик знает **только один ближайший переход**, а не весь путь — это экономит память, но ограничивает пост-анализ маршрута.
- Такой подход обеспечивает минимальное потребление памяти, но требует повторного вызова планирования при каждом шаге маршрута.

## Потенциальные улучшения

- **Предрасчёт стоимости путей**: реализовать таблицу `cost_table[N][N]`, позволяющую определять следующий шаг за O(1) вместо обратного BFS. Требует значительного объёма памяти (например, для 30 нод ≈ 3.5 KB), что может быть критично для MCU с ограниченной SRAM.
- **Добавление поддержки самозамкнутых переходов**: реализовать возможность явно регистрировать `A → A` переходы и  обрабатывать их при простое.

---

README составлен с помощью ChatGPT.

