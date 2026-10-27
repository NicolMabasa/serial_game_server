# PC test of the server logic (no board). Needs gcc.   Run:  make -C host_tests test
SRC = ../stm32/Core/Src
INC = ../stm32/Core/Inc

test: test_server.c $(SRC)/game.c $(SRC)/protocol.c $(SRC)/framer.c
	gcc -std=gnu11 -Wall -Wextra -DFRAMER_HOST_TEST -I$(INC) -o test_server $^
	./test_server

clean:
	rm -f test_server test_server.exe
