CC = g++
CFLAGS = -I../utility/ -Wall -Wextra -pedantic -Wshadow -Wstrict-aliasing -Wconversion -Wunused-result -lm -pthread -g -O2

a.out: main.cpp simulation_plot.hpp simulation.hpp node.hpp method3.hpp store.hpp map.hpp edge.hpp
	$(CC) $(CFLAGS) -o a.out main.cpp

clean:
	rm -f *.o main
