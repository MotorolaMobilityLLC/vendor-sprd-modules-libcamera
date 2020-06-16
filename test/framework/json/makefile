CXX=g++
CXXFLAGS=-g -std=c++11 -Wall -pedantic
TARGET=jsontest

MAKEFILEPATH = $(shell pwd)
INC = -I$(MAKEFILEPATH)/include
LDFLAGS = -L$(MAKEFILEPATH)/lib
JSON_LIB = $(MAKEFILEPATH)/lib/libjsoncpp.a
CXXFLAGS += $(INC)

SRC=$(wildcard *.cpp src/*.cpp)
OBJS:=$(patsubst %.cpp,%.o,$(SRC))
$(warning $(OBJS))

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(JSON_LIB)
	rm -f $(OBJ)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ $(INC) $(LDFLAGS) $(JSON_LIB)

clean:
	rm -f $(OBJS)
	rm $(TARGET)

