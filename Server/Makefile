CC=g++

LDFLAGS=-I/usr/local/Cellar/boost/1.63.0/include

LLIBFLAGS=-L/usr/local/Cellar/boost/1.63.0/lib

LINKFLAGS=-lboost_thread-mt -lboost_system-mt

FLAGS=-std=c++11 $(LLIBFLAGS) $(LDFLAGS) $(LINKFLAGS)


TBDServer: TouchBarDinoXServer.cpp
	$(CC) $(FLAGS) -o TouchBarDinoXServer.out TouchBarDinoXServer.cpp
