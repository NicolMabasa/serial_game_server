import os
import pytest
from gui.link import FakeLink, SerialLink


@pytest.fixture
def link():
    port = os.environ.get("SERIAL_PORT")
    if port:
        return SerialLink(port)
    return FakeLink()


@pytest.fixture
def real_link(link):
    if not os.environ.get("SERIAL_PORT"):
        pytest.skip("needs the real board")
    return link