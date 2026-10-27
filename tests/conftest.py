import os
import pytest
from gui.link import FakeLink, SerialLink


@pytest.fixture
def link():
    port = os.environ.get("SERIAL_PORT")
    if port:
        lk = SerialLink(port)
        lk.ser.reset_input_buffer()
        lk.send("RESIGN")
    else:
        lk = FakeLink()
    yield lk
    lk.close()          # always release the COM port, even when a test fails


@pytest.fixture
def real_link(link):
    if not os.environ.get("SERIAL_PORT"):
        pytest.skip("needs the real board")
    return link