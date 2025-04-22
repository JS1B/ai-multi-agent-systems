CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2
INCLUDES = -Iinclude

SRC_DIR = src
INC_DIR = include
OBJ_DIR = build

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(INC_DIR)/*.h)
HPP_SOURCES = $(wildcard $(SRC_DIR)/*.hpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
HPP_OBJECTS = $(HPP_SOURCES:$(SRC_DIR)/%.hpp=$(OBJ_DIR)/%.o)
EXECUTABLE = searchclient

# Create object directory if it doesn't exist
$(shell mkdir -p $(OBJ_DIR))

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(HPP_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: all clean run 