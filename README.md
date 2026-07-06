# esp32-light-alarm

A light-triggered alarm clock built on ESP-IDF: when a photoresistor detects
that the room has gotten light, a piezo buzzer sounds and a status LED lights
up, until a button dismisses it. The alarm re-arms once the room goes dark
again.

The alarm state machine (`components/alarm_logic/`) is a small, pure,
hardware-independent module — no ESP-IDF includes at all — so its behavior is
covered by fast host-side unit tests that run without any ESP-IDF toolchain
or hardware attached.

## Hardware

This is a single-board project, not a general-purpose ESP-IDF example: it
targets one specific board, **ESP32-S3-DevKitC-1 (16MB flash)**, wired as
follows.

| Signal | GPIO (default) | Notes |
| --- | --- | --- |
| Status LED | 48 | Onboard addressable (WS2812-style) LED, driven over RMT. GPIO38 on DevKitC-1 v1.1, GPIO48 on v1.0 — check your board revision. |
| Light sensor (LDR) | 4 (ADC1 channel 3) | Voltage divider: `3V3 -> LDR -> GPIO4 -> 10k -> GND`. Fixed by the wiring, not Kconfig-configurable. |
| Buzzer | 5 | Passive piezo buzzer, driven via LEDC PWM. |
| Dismiss button | 6 | Active-low, using the internal pull-up — no external resistor needed. |

All of the above except the LDR pin are configurable via `idf.py menuconfig`
(see [Configuration](#configuration)).

## Getting Started

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html)
(developed against v5.5.4).

```sh
idf.py set-target esp32s3
idf.py menuconfig   # optional: tune GPIOs, light threshold, buzzer tone
idf.py -p PORT build flash monitor
```

(To exit the serial monitor, type `Ctrl-]`.)

## Configuration

Run `idf.py menuconfig` and open **esp32-light-alarm Configuration** to set
GPIO assignments, the buzzer's PWM tone frequency, and the light threshold.

The light threshold (`ALARM_LIGHT_THRESHOLD`) is a raw 12-bit ADC1 reading
(0–4095) above which the room counts as "light". The app always logs the
current raw reading and threshold to the serial monitor, so watch `idf.py
monitor` while covering/uncovering the photoresistor to pick a value that
matches your wiring.

## Testing

Two independent test tiers:

- **Host-side logic tests** — no ESP-IDF toolchain or hardware required, just
  Python and a system C compiler:
  ```sh
  pytest components/alarm_logic/test
  ```
- **Hardware/build test** — needs the ESP-IDF Python environment and a
  connected ESP32-S3 board:
  ```sh
  pytest pytest_esp32_light_alarm.py --target esp32s3
  ```

## Architecture

See [docs/architecture.md](docs/architecture.md) for the alarm state machine
and pin/wiring reference.

## Hardware Design

Schematic and PCB design (KiCad) are planned; see
[hardware/README.md](hardware/README.md) for the intended layout.

## License

MIT — see [LICENSE](LICENSE).

## Roadmap

CI (GitHub Actions running the host-side test suite and a build check), the
KiCad schematic/PCB, and editor/devcontainer tooling (VSCode, ESP-IDF
devcontainer) are planned follow-ups, not yet present in this repo.
