import time
import serial
from fake_server.server import GameServer


class FakeLink:
    def __init__(self):
        self.server = GameServer()

    def send(self, line):
        return self.server.handle(line)

    def close(self):
        pass


class SerialLink:
    def __init__(self, port, baud=115200):
        self.ser = serial.Serial(port, baud, timeout=1)
        time.sleep(0.3)               # let the ST-Link virtual COM port settle after opening
        self.ser.reset_input_buffer()

    def send(self, line):
        self.ser.write((line + "\n").encode())
        reply = self.ser.readline().decode(errors="replace").strip()
        if reply == "":
            return "TIMEOUT"
        return reply

    def close(self):
        self.ser.close()
