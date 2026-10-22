import serial
from fake_server.server import GameServer


class FakeLink:
    def __init__(self):
        self.server = GameServer()

    def send(self, line):
        return self.server.handle(line)


class SerialLink:
    def __init__(self, port, baud=115200):
        self.ser = serial.Serial(port, baud, timeout=1)

    def send(self, line):
        self.ser.write((line + "\n").encode())
        reply = self.ser.readline().decode(errors="replace").strip()
        if reply == "":
            return "TIMEOUT"
        return reply