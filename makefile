CC=gcc
CXX=g++

all:	clean main

main:	main.c
	${CXX} -o main main.c

clean: 
	rm -f main