CXX = g++
CC = gcc

CFLAGS = -std=c++17 -O2 -Wall -g 
CFLAGS_C = -O2 -Wall -g

TARGET = server
OBJS = ../code/log/*.cpp  ../code/timer/*.cpp \
       ../code/http/*.cpp ../code/server/*.cpp \
       ../code/buffer/*.cpp ../code/main.cpp

OBJ_JATCC = ../code/compiler/jatcc.c



all: $(OBJS) $(OBJ_JATCC)
	$(CC) $(CFLAGS_C) -c $(OBJ_JATCC) -o ../bin/jatcc.o
	$(CXX) $(CFLAGS) $(OBJS) ../bin/jatcc.o -o ../bin/$(TARGET) -pthread 

clean:
	rm -rf ../bin/*.o $(TARGET)

