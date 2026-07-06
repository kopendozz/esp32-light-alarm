#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ALARM_STATE_IDLE,
    ALARM_STATE_SOUNDING,
    ALARM_STATE_DISMISSED,
} alarm_state_t;

const char *alarm_state_name(alarm_state_t state);

alarm_state_t alarm_next_state(alarm_state_t state, bool is_light, bool pressed);

bool alarm_next_buzzer(alarm_state_t state, bool buzzer_on);

#ifdef __cplusplus
}
#endif
