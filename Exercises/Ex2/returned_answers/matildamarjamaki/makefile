CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g

# Target executable
TARGET = hello

# For deleting the target
TARGET_DEL = hello.exe

# Source files
SRCS = hello.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm -f $(TARGET) $(OBJS)