
# Compiler and flags
CXX = g++
CXXFLAGS = $(shell root-config --cflags)
LDFLAGS = $(shell root-config --libs)

# Output executables
EXEC1 = generateTree
EXEC2 = plotHistogram

# Source files
SRC1 = generateTree.C
SRC2 = plotHistogram.C

# Default target
all: $(EXEC1) $(EXEC2)

# Rule to compile the .cpp scripts into executables
$(EXEC1): $(SRC1)
	$(CXX) $(CXXFLAGS) $(SRC1) -o $(EXEC1) $(LDFLAGS)

$(EXEC2): $(SRC2)
	$(CXX) $(CXXFLAGS) $(SRC2) -o $(EXEC2) $(LDFLAGS)

# Run both executables
run: $(EXEC1) $(EXEC2)
	./$(EXEC1)
	./$(EXEC2)

# Clean up generated files
clean:
	rm -f $(EXEC1) $(EXEC2) histogram.png

# Phony targets (not actual files)
.PHONY: all run clean
