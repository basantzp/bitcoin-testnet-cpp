CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -I./include -pthread
LDFLAGS = -lssl -lcrypto -lpthread

SRCS = src/crypto.cpp src/script.cpp src/blockchain.cpp src/server.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = bin/bitcoin-testnet-core

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	@echo "✓ Successfully built $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: src/crypto.o src/script.o src/blockchain.o tests/test_script_vm.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -I./src src/crypto.o src/script.o src/blockchain.o tests/test_script_vm.cpp -o bin/test_script_vm $(LDFLAGS)
	./bin/test_script_vm

clean:
	rm -rf $(OBJS) bin

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test
