def test_connect_and_new(link):
    assert link.send("CONNECT") == "OK"
    assert link.send("NEW") == "OK:BOARD:........."
    
def test_legal_move(link):
    link.send("CONNECT")
    link.send("NEW")
    assert link.send("MOVE:4") == "OK:BOARD:....X...."
    assert link.send("STATE") == "BOARD:....X...."
    
def test_occupied(link):
    link.send("CONNECT")
    link.send("NEW")
    link.send("MOVE:4")
    assert link.send("MOVE:4") == "NACK:OCCUPIED"
    assert link.send("STATE") == "BOARD:....X...."
    
def test_range(link):
    link.send("CONNECT")
    link.send("NEW")
    assert link.send("MOVE:9") == "NACK:RANGE"
    
def test_win(link):
    link.send("CONNECT")
    link.send("NEW")
    replies = [link.send("MOVE:" + str(i)) for i in (0, 3, 1, 4, 2)]
    assert replies[-1] == "WIN:X"
    
def test_move_after_game_over(link):
    link.send("CONNECT")
    link.send("NEW")
    for i in (0, 3, 1, 4, 2):
        link.send("MOVE:" + str(i))
    assert link.send("MOVE:5") == "NACK:NOGAME"
    
def test_draw(link):
    link.send("CONNECT")
    link.send("NEW")
    replies = [link.send("MOVE:" + str(i)) for i in (0, 1, 2, 4, 3, 5, 7, 6, 8)]
    assert replies[-1] == "DRAW"

def test_unknown_command(link):
    link.send("CONNECT")
    link.send("NEW")
    assert link.send("HELLO") == "NACK:BADCMD"
    assert link.send("STATE") == "BOARD:........."
    
def test_move_not_a_number(link):
    link.send("CONNECT")
    link.send("NEW")
    assert link.send("MOVE:abc") == "NACK:BADCMD"
    assert link.send("STATE") == "BOARD:........."

def test_move_empty(link):
    link.send("CONNECT")
    link.send("NEW")
    assert link.send("MOVE:") == "NACK:BADCMD"
    assert link.send("STATE") == "BOARD:........."
    
def test_move_before_new(link):
    link.send("CONNECT")
    assert link.send("MOVE:0") == "NACK:NOGAME"
    assert link.send("STATE") == "NACK:NOGAME"