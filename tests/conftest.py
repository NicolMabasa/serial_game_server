import os
import pytest
from gui.link import FakeLink, SerialLink
from fake_server.server import GameServer





@pytest.fixture
def link():
    port = os.environ.get("SERIAL_PORT")
    if port:
        return SerialLink(port)
    return FakeLink()