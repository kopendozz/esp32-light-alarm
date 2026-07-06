#include "alarm_logic.h"

const char *alarm_state_name(alarm_state_t state)
{
    switch (state) {
    case ALARM_STATE_IDLE: return "IDLE";
    case ALARM_STATE_SOUNDING: return "SOUNDING";
    case ALARM_STATE_DISMISSED: return "DISMISSED";
    }
    return "?";
}

alarm_state_t alarm_next_state(alarm_state_t state, bool is_light, bool pressed)
{
    alarm_state_t next_state = state;
    switch (state) {
    case ALARM_STATE_IDLE:
        if (is_light) {
            next_state = ALARM_STATE_SOUNDING;
        }
        break;
    case ALARM_STATE_SOUNDING:
        if (pressed) {
            next_state = ALARM_STATE_DISMISSED;
        } else if (!is_light) {
            next_state = ALARM_STATE_IDLE;
        }
        break;
    case ALARM_STATE_DISMISSED:
        if (!is_light) {
            next_state = ALARM_STATE_IDLE;
        }
        break;
    }
    return next_state;
}

bool alarm_next_buzzer(alarm_state_t state, bool buzzer_on)
{
    return (state == ALARM_STATE_SOUNDING) ? !buzzer_on : false;
}
