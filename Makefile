CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g -O0   
LDFLAGS = -lhdf5_cpp -lhdf5                  

SRC = main.cpp
OUT = NeuralynxConverter

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)
