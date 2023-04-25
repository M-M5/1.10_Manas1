main: poke327.cpp
	g++ -Wall -Werror poke327.cpp -o a.out -lncurses

clean:
	-rm a.out