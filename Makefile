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

clean:
	rm -rf $(OBJS) bin

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
