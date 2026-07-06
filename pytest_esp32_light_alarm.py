import logging
import os

import pytest
from pytest_embedded_idf.dut import IdfDut


@pytest.mark.esp32s3
def test_binary_size(dut: IdfDut) -> None:
    binary_file = os.path.join(dut.app.binary_path, 'esp32-light-alarm.bin')
    bin_size = os.path.getsize(binary_file)
    logging.info('esp32-light-alarm_bin_size : {}KB'.format(bin_size // 1024))


@pytest.mark.esp32s3
def test_alarm_boots(dut: IdfDut) -> None:
    # confirm the app boots and starts its polling loop in the initial IDLE state
    dut.expect(r'state=IDLE')
