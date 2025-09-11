"""MCM vendor interface testsuite.

Copyright Melexis N.V.

This product includes software developed at Melexis N.V. (https://www.melexis.com).

Melexis N.V. has provided this code according to LICENSE file attached to repository
"""
import time
import pytest
from pylinframe import LinFrame


@pytest.mark.webusb
def test_identify(use_mcm_usb):
    """Test if we can request the identification."""
    mcm = use_mcm_usb()
    mcm.get_device("server").identify()


@pytest.mark.webusb
def test_read_product_name(use_mcm_usb):
    """Test if we can read the MCM product name."""
    mcm = use_mcm_usb()
    product = mcm.get_device("server").get_product_name()
    assert "Melexis Compact Master LIN" == product, f"Invalid product name '{product}'"


@pytest.mark.webusb
def test_read_serial_number(use_mcm_usb, serial_number):
    """Test if we can read the MCM serial number."""
    mcm = use_mcm_usb(serial_number)
    serial = mcm.get_device("server").get_serial_number()
    assert serial_number == serial, f"Expected version '{serial_number}' but got '{serial}'"


@pytest.mark.webusb
def test_read_version(use_mcm_usb, git_version):
    """Test if we can read the MCM firmware version."""
    mcm = use_mcm_usb()
    version = mcm.get_device("server").get_firmware_version()
    assert git_version == version, f"Expected version '{git_version}' but got '{version}'"


@pytest.mark.webusb
def test_slave_power_control(use_mcm_usb):
    """Test if we can control and get status of the slave power control."""
    mcm = use_mcm_usb()
    power_out = mcm.get_device("power_out0")
    power_out.power_down()
    assert power_out.power_state() is False
    power_out.power_up()
    assert power_out.power_state() is True
    power_out.power_down()
    assert power_out.power_state() is False


@pytest.mark.webusb
def test_lin_wake_up(use_mcm_usb):
    """Test if we can use LIN wake up generation."""
    mcm = use_mcm_usb()
    mcm.get_device("lin0").setup()
    mcm.get_device("lin0").send_wake_up_pulse()
    mcm.get_device("lin0").teardown()


@pytest.mark.webusb
def test_lin_s2m_frame_sending(use_mcm_usb):
    """Test if we can use LIN M2S/S2M frame sending."""
    txframe = LinFrame(8)
    txframe.baudrate = 19200
    txframe.master_to_slave = True
    txframe.enhanced_crc = False
    txframe.identifier = 0x3C
    txframe.databytes = [0x7F, 0x06, 0xB2, 0x00, 0xFF, 0x7F, 0xFF, 0xFF]
    rxframe = LinFrame(8)
    rxframe.baudrate = 19200
    rxframe.master_to_slave = False
    rxframe.enhanced_crc = False
    rxframe.identifier = 0x3D
    mcm = use_mcm_usb()
    mcm.get_device("lin0").setup()
    time.sleep(0.02)
    mcm.get_device("lin0").handle_message_on_bus(txframe)
    mcm.get_device("lin0").handle_message_on_bus(rxframe)
    mcm.get_device("lin0").teardown()
