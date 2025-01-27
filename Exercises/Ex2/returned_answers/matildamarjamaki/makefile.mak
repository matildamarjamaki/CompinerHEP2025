
# Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall
TARGET = hello

all: $(TARGET)

$(TARGET): hello.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) hello.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean