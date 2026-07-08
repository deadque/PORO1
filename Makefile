CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall

SOURCE = game.cpp
TARGET_WIN = game.exe
TARGET_LIN = game

OBJECTS = $(SOURCE:.cpp=.o)

ifeq ($(OS),Windows_NT)
  TARGET = $(TARGET_WIN)
else
  TARGET = $(TARGET_LIN)
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
