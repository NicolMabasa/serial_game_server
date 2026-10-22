# Serial Game Server

## What it is
A Tic-Tac-Toe game. An STM32 board is the server and a Tkinter GUI is the client.

## Install
Run this from the project folder. It installs everything listed in requirements.txt.

```
pip install -r requirements.txt
```

## Run the GUI
Fake server (no board needed):

```
python -m gui.app
```

Real board (replace COM4 with your board's port):

```
python -m gui.app COM4
```

Find your port with `python -m serial.tools.list_ports`. It can change on another computer or USB socket.

## Run the tests
Fake server:

```
python -m pytest -v
```

Real board (PowerShell):

```
$env:SERIAL_PORT="COM4"; python -m pytest -v
```

## Folders
- `gui`: the Tkinter program the player uses. It shows the board and sends requests. It never decides if a move is legal. `link.py` holds the two links: `FakeLink` for the fake server and `SerialLink` for the real board.
- `tests`: the pytest tests. They send commands and check the replies. They run on the fake server by default, and on the real board when `SERIAL_PORT` is set.
- `fake_server`: a Python copy of the server rules. We use it to build and test the GUI and tests before the board is ready.
- `stm32`: the STM32 server project. It holds the real game state and checks every move.