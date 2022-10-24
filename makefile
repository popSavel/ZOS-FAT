CC=gcc
CXX=g++

all:	clean main

main:	main.cpp
	${CXX} -o main main.cpp

clean: 
	rm -f main

