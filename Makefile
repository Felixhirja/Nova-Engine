CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS =

SHELL = /usr/bin/bash

GLFW_CFLAGS := $(shell pkg-config --cflags glfw3 2>/dev/null)
GLFW_LIBS  := $(shell pkg-config --libs glfw3 2>/dev/null)

# Fallback if pkg-config fails
ifeq ($(GLFW_LIBS),)
GLFW_CFLAGS := -IC:/msys64/mingw64/include
GLFW_LIBS := -LC:/msys64/mingw64/lib -lglfw3
endif

ifneq ($(GLFW_LIBS),)
	CXXFLAGS += $(GLFW_CFLAGS) -DUSE_GLFW
	LDLIBS += $(GLFW_LIBS)
	LDLIBS += -lopengl32 -lglu32 -lfreeglut
$(info GLFW detected: building with GLFW support)
else
$(info GLFW not found; building ASCII fallback)
endif

SRC := $(wildcard src/*.cpp) $(wildcard src/ecs/*.cpp)
OBJ := $(filter-out src/test_sdl.o src/main_test.o, $(patsubst %.cpp,%.o,$(SRC)))
TARGET := star-engine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: tests/test_simulation

tests/test_simulation: tests/test_simulation.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_simulation.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_camera: tests/test_camera.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_camera.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

clean:
	rm -f $(OBJ) $(TARGET) tests/test_simulation

.PHONY: all test clean

