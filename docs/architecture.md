# Architecture

## State machine

The alarm's behavior is a 3-state machine, implemented as pure logic in
[`components/alarm_logic/alarm_logic.c`](../components/alarm_logic/alarm_logic.c)
with no ESP-IDF or hardware dependencies. `main/alarm_main.c` polls the
sensor/button every 200ms, feeds the readings into this state machine, and
drives the buzzer/LED outputs from its result.

```
                is_light
        ┌───────────────────────┐
        │                       ▼
    ┌───────┐              ┌───────────┐
    │ IDLE  │◀─────────────│ SOUNDING  │
    └───────┘   !is_light  └───────────┘
                                 │  ▲
                        pressed  │  │ is_light
                                 ▼  │
                            ┌────────────┐
                            │ DISMISSED  │
                            └────────────┘
                                 │
                            !is_light
                                 │
                                 ▼
                             (-> IDLE)
```

- **IDLE**: waiting. Transitions to **SOUNDING** as soon as the room reads
  light (`is_light`).
- **SOUNDING**: buzzer toggles on/off each poll tick (see
  `alarm_next_buzzer`), status LED tracks the buzzer. Transitions to
  **DISMISSED** if the button is pressed, or back to **IDLE** if the room
  goes dark again before dismissal.
- **DISMISSED**: silent, alarm suppressed while the room is still light.
  Transitions back to **IDLE** once the room goes dark, re-arming the alarm
  for the next light trigger.

Button presses have no effect in **IDLE** or once already **DISMISSED** —
only a currently-**SOUNDING** alarm can be dismissed.

## Pin / wiring reference

See the [README's hardware table](../README.md#hardware) for the full
signal/GPIO/notes list. In summary:

- **LDR** (light sensor): fixed wiring, `3V3 -> LDR -> GPIO4 -> 10k -> GND`,
  read via ADC1 channel 3. Not Kconfig-configurable since the ADC unit/channel
  is tied to this specific GPIO on ESP32-S3.
- **Status LED**: onboard addressable LED strip (RMT-driven), GPIO
  configurable via `ALARM_STATUS_LED_GPIO`.
- **Buzzer**: passive piezo via LEDC PWM, GPIO configurable via
  `ALARM_BUZZER_GPIO`, tone frequency via `ALARM_BUZZER_FREQ_HZ`.
- **Dismiss button**: active-low with internal pull-up, GPIO configurable via
  `ALARM_BUTTON_GPIO`.

## Module boundaries

- `components/alarm_logic/` — pure state machine, unit-testable on the host
  (see `components/alarm_logic/test/`).
- `main/` — hardware orchestration: ADC read, LEDC PWM, GPIO, LED strip
  driver, and the polling loop that ties readings to `alarm_logic` and drives
  outputs.
