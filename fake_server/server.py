WIN_LINES = [
(0, 1, 2),
(3, 4, 5),
(6, 7, 8),
(0 ,3, 6),
(1, 4, 7),
(2, 5, 8),
(0, 4, 8),
(2, 4, 6)
]

def check_result(board):
    for line in WIN_LINES:
        a, b, c = line
        if board[a] == board[b] == board[c] and board[a] != '.':
            return board[a]
       
    if "." not in board:
        return "DRAW"
    return None

class GameServer:
    def __init__(self):
        self.board = "........."
        self.turn = "X"
        self.state = "IDLE"
        
    def new_game(self):
        self.board = "........."
        self.turn = "X"
        self.state = "IN_PROGRESS"

    def make_move(self, index):
        if self.state != "IN_PROGRESS":
            return "NACK:NOGAME"
        if index < 0 or index > 8:
            return "NACK:RANGE"
        if self.board[index] != ".":
            return "NACK:OCCUPIED"
        
        self.board = self.board[:index] + self.turn + self.board[index + 1:]
        result = check_result(self.board)
        if result in ("X", "O"):
            self.state = "GAME_OVER"
            return "WIN:" + result
        if result == "DRAW":
            self.state = "GAME_OVER"
            return "DRAW"
        self.turn = "O" if self.turn == "X" else "X"
        return "OK:BOARD:" + self.board
    
    def handle(self, line):
        line = line.strip()
        if line == "CONNECT":
            return "OK"
        if line == "NEW":
            if self.state == "IN_PROGRESS":
                return "NACK:NOGAME"
            self.new_game()
            return "OK:BOARD:" + self.board
        if line == "STATE":
            if self.state == "IDLE":
                return "NACK:NOGAME"
            return "BOARD:" + self.board
        
        if line.startswith("MOVE:"):
            text = line[5:]
            if not text.isdigit():
                return "NACK:BADCMD"
            return self.make_move(int(text))
        
        if line == "RESIGN":
            if self.state != "IN_PROGRESS":
                return "NACK:NOGAME"
            self.state = "GAME_OVER"
            return "OK:GAMEOVER"
        if line == "DISCONNECT":
            self.state = "IDLE"
            return "OK"
        
        return "NACK:BADCMD"
   