import sys
import tkinter as tk
from gui.link import FakeLink, SerialLink

if len(sys.argv) > 1:
    link = SerialLink(sys.argv[1])
else:
    link = FakeLink()

root = tk.Tk()
root.title("Tic-Tac-Toe")

status = tk.Label(root, text="Not connected")
status.grid(row=0, column=0, columnspan=3)

buttons = []
for i in range(9):
    b = tk.Button(root, text=".", width=6, height=3, command=lambda i=i: on_cell(i))
    b.grid(row=1 + i // 3, column=i % 3)
    buttons.append(b)


def draw_board(board):
    for i in range(9):
        buttons[i].config(text=board[i])
        
def set_cells(enabled):
    for b in buttons:
        b.config(state="normal" if enabled else "disabled")

def on_cell(i):
    reply = link.send(f"MOVE:{i}")
    if reply.startswith("OK:BOARD:"):
        draw_board(reply[9:])
        status.config(text="Move accepted")
    elif reply.startswith("WIN:"):
        status.config(text=f"Player {reply[4:]} wins!")
        draw_board(link.send("STATE")[6:])
        set_cells(False)
    elif reply == "DRAW":
        status.config(text="Game is a draw")
        draw_board(link.send("STATE")[6:])
        set_cells(False)
    else:
        status.config(text="Rejected: " + reply)
        
def on_connect():
    reply = link.send("CONNECT")
    if reply == "OK":
        status.config(text="Connected")
    else:
        status.config(text="Error: " + reply)

def on_disconnect():
    reply = link.send("DISCONNECT")
    if reply == "OK":
        status.config(text="Disconnected")
        set_cells(False)
    else: 
        status.config(text="Error: " + reply)

def on_new():
    reply = link.send("NEW")
    if reply.startswith("OK:BOARD:"):
        draw_board(reply[9:])
        set_cells(True)
        status.config(text="New game started")
    else:
        status.config(text="Error: " + reply)

def on_resign():
    reply = link.send("RESIGN")
    if reply == "OK:GAMEOVER":
        status.config(text="You resigned. Game over")
        draw_board(link.send("STATE")[6:])
        set_cells(False)
    else:
        status.config(text="Rejected: " + reply)

tk.Button(root, text="Connect", command=on_connect).grid(row=4, column=0)
tk.Button(root, text="New Game", command=on_new).grid(row=4, column=1)
tk.Button(root, text="Disconnect", command=on_disconnect).grid(row=4, column=2)
tk.Button(root, text="Resign", command=on_resign).grid(row=5, column=1)

def on_close():
    link.send("DISCONNECT")
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)
set_cells(False)
root.mainloop()