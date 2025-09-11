"""HIL test suite general configuration.

Copyright Melexis N.V.

This product includes software developed at Melexis N.V. (https://www.melexis.com).

Melexis N.V. has provided this code according to LICENSE file attached to repository
"""
import logging
import pytest
import git
from pymcmclient import MelexisCompactMaster

LOGGER = logging.getLogger()


def pytest_addoption(parser):
    """Add options to the pytest cli."""
    parser.addoption("--serial",
                     action="store",
                     help="serial number of the MCM")
    parser.addoption("--hostname",
                     action="store",
                     help="hostname of the MCM")


@pytest.fixture(name="serial_number", scope="session")
def fixture_serial_number(request):
    """Getter for the serial number

    Returns
        str: serial number of the MCM.
    """
    return request.config.getoption("--serial")


@pytest.fixture(name="hostname", scope="session")
def fixture_hostname(request):
    """Getter for the hostname

    Returns
        str: hostname of the MCM.
    """
    return request.config.getoption("--hostname")


@pytest.fixture(name="git_version", scope="session")
def fixture_git_version():
    """Getter for the git version number string.

    Returns
        str: git version number string.
    """
    repo = git.Repo()
    return repo.git.describe()


@pytest.fixture(name="use_mcm_usb", scope="session")
def fixture_use_mcm_usb():
    """Connect to the MCM module via USB."""
    mcm = MelexisCompactMaster()

    def _connect_mcm(serial=None):
        mcm.open_all(serial)
        return mcm

    yield _connect_mcm

    mcm.close_all()
