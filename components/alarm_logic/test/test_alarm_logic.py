"""Host-side unit tests for the pure alarm state machine in alarm_logic.c.

Compiles alarm_logic.c with the system C compiler into a shared library and
drives it via ctypes, so these run with plain python3 + a C compiler -- no
ESP-IDF toolchain or hardware required.
"""
import ctypes
import platform
import subprocess
import sys
from pathlib import Path
from typing import Iterator

import pytest

ALARM_STATE_IDLE = 0
ALARM_STATE_SOUNDING = 1
ALARM_STATE_DISMISSED = 2

COMPONENT_DIR = Path(__file__).resolve().parents[1]
LIB_EXT = 'dylib' if platform.system() == 'Darwin' else 'so'


@pytest.fixture(scope='module')
def alarm_logic(tmp_path_factory: pytest.TempPathFactory) -> Iterator[ctypes.CDLL]:
    build_dir = tmp_path_factory.mktemp('alarm_logic_build')
    lib_path = build_dir / f'alarm_logic.{LIB_EXT}'

    cc = 'cc'
    result = subprocess.run(
        [cc, '-shared', '-fPIC',
         '-I', str(COMPONENT_DIR / 'include'),
         '-o', str(lib_path), str(COMPONENT_DIR / 'alarm_logic.c')],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        pytest.fail(f'failed to compile alarm_logic.c:\n{result.stderr}')

    lib = ctypes.CDLL(str(lib_path))

    lib.alarm_next_state.argtypes = [ctypes.c_int, ctypes.c_bool, ctypes.c_bool]
    lib.alarm_next_state.restype = ctypes.c_int

    lib.alarm_next_buzzer.argtypes = [ctypes.c_int, ctypes.c_bool]
    lib.alarm_next_buzzer.restype = ctypes.c_bool

    lib.alarm_state_name.argtypes = [ctypes.c_int]
    lib.alarm_state_name.restype = ctypes.c_char_p

    yield lib


@pytest.mark.parametrize(
    ('state', 'is_light', 'pressed', 'expected'),
    [
        # IDLE
        (ALARM_STATE_IDLE, True, False, ALARM_STATE_SOUNDING),
        (ALARM_STATE_IDLE, True, True, ALARM_STATE_SOUNDING),
        (ALARM_STATE_IDLE, False, False, ALARM_STATE_IDLE),
        (ALARM_STATE_IDLE, False, True, ALARM_STATE_IDLE),
        # SOUNDING
        (ALARM_STATE_SOUNDING, True, True, ALARM_STATE_DISMISSED),
        (ALARM_STATE_SOUNDING, False, True, ALARM_STATE_DISMISSED),
        (ALARM_STATE_SOUNDING, False, False, ALARM_STATE_IDLE),
        (ALARM_STATE_SOUNDING, True, False, ALARM_STATE_SOUNDING),
        # DISMISSED
        (ALARM_STATE_DISMISSED, False, False, ALARM_STATE_IDLE),
        (ALARM_STATE_DISMISSED, False, True, ALARM_STATE_IDLE),
        (ALARM_STATE_DISMISSED, True, False, ALARM_STATE_DISMISSED),
        (ALARM_STATE_DISMISSED, True, True, ALARM_STATE_DISMISSED),
    ],
)
def test_alarm_next_state(alarm_logic: ctypes.CDLL, state: int, is_light: bool, pressed: bool, expected: int) -> None:
    assert alarm_logic.alarm_next_state(state, is_light, pressed) == expected


@pytest.mark.parametrize(
    ('state', 'buzzer_on', 'expected'),
    [
        (ALARM_STATE_SOUNDING, False, True),
        (ALARM_STATE_SOUNDING, True, False),
        (ALARM_STATE_IDLE, False, False),
        (ALARM_STATE_IDLE, True, False),
        (ALARM_STATE_DISMISSED, False, False),
        (ALARM_STATE_DISMISSED, True, False),
    ],
)
def test_alarm_next_buzzer(alarm_logic: ctypes.CDLL, state: int, buzzer_on: bool, expected: bool) -> None:
    assert alarm_logic.alarm_next_buzzer(state, buzzer_on) == expected


@pytest.mark.parametrize(
    ('state', 'expected'),
    [
        (ALARM_STATE_IDLE, b'IDLE'),
        (ALARM_STATE_SOUNDING, b'SOUNDING'),
        (ALARM_STATE_DISMISSED, b'DISMISSED'),
    ],
)
def test_alarm_state_name(alarm_logic: ctypes.CDLL, state: int, expected: bytes) -> None:
    assert alarm_logic.alarm_state_name(state) == expected


if __name__ == '__main__':
    sys.exit(pytest.main([__file__, '-v']))
