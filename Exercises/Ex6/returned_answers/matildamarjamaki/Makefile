
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 `root-config --cflags`
LDFLAGS = `root-config --libs`

EXEC1 = generateTree
EXEC2 = plotHistogram

SRC1 = generateTree.C
SRC2 = plotHistogram.C

OBJ1 = $(SRC1:.C=.o)
OBJ2 = $(SRC2:.C=.o)

all: $(EXEC1) $(EXEC2)

$(EXEC1): $(OBJ1)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(EXEC2): $(OBJ2)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.C
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ1) $(OBJ2) $(EXEC1) $(EXEC2) histogram.png random_numbers.root