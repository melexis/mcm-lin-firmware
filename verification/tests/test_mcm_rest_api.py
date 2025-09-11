"""MCM REST API interface testsuite.

Copyright Melexis N.V.

This product includes software developed at Melexis N.V. (https://www.melexis.com).

Melexis N.V. has provided this code according to LICENSE file attached to repository
"""
from http import HTTPStatus
import time
import pytest
import requests


@pytest.mark.rest
def test_request_reboot(hostname):
    """Test if we can request a system reboot."""
    resp = requests.put(f"https://{hostname}/api/v1/system/reboot",
                        data={},
                        timeout=2,
                        verify=False)
    assert HTTPStatus.NO_CONTENT == resp.status_code
    time.sleep(5)  # allow MCM to reboot and re-connect to WiFi


@pytest.mark.rest
def test_identify(hostname):
    """Test if we can request the identification."""
    resp = requests.put(f"https://{hostname}/api/v1/system/identify",
                        data={},
                        timeout=2,
                        verify=False)
    assert HTTPStatus.NO_CONTENT == resp.status_code


@pytest.mark.rest
def test_read_device_information(hostname, git_version):
    """Test if we can read the MCM device information."""
    resp = requests.get(f"https://{hostname}/api/v1",
                        timeout=2,
                        verify=False)
    assert HTTPStatus.OK == resp.status_code
    data = resp.json()
    assert "Melexis Compact Master LIN" == data["model"], f"Invalid product name '{data['model']}'"
    assert git_version == data["firmware_version"], f"Expected version '{git_version}' but got '{data['firmware_version']}'"


@pytest.mark.rest
def test_read_system_wifi_configuration(hostname, serial_number):
    """Test if we can read the MCM system configuration."""
    resp = requests.get(f"https://{hostname}/api/v1/system/wifi",
                        timeout=2,
                        verify=False)
    assert HTTPStatus.OK == resp.status_code
    data = resp.json()
    assert serial_number == data["mac"], f"Expected version '{serial_number}' but got '{data['mac']}'"
    assert hostname == data["hostname"], f"Expected hostname '{hostname}' but got '{data['hostname']}'"
