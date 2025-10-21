CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra
CFLAGS = -Wall -Wextra
LDFLAGS =

SHELL = /usr/bin/bash

# GLAD include path (must come BEFORE system includes)
GLAD_INCLUDE := -Isrc/glad/include

GLFW_CFLAGS := $(shell pkg-config --cflags glfw3 2>/dev/null)
GLFW_LIBS  := $(shell pkg-config --libs glfw3 2>/dev/null)

# Fallback if pkg-config fails
ifeq ($(GLFW_LIBS),)
GLFW_CFLAGS := -IC:/msys64/mingw64/include
GLFW_LIBS := -LC:/msys64/mingw64/lib -lglfw3
endif

ifneq ($(GLFW_LIBS),)
	CXXFLAGS += $(GLAD_INCLUDE) $(GLFW_CFLAGS) -DUSE_GLFW
	CFLAGS += $(GLAD_INCLUDE)
	LDLIBS += $(GLFW_LIBS)
	LDLIBS += -lopengl32 -lglu32 -lfreeglut
$(info GLFW detected: building with GLFW support)
else
$(info GLFW not found; building ASCII fallback)
endif

# Include graphics subsystem and GLAD loader
SRC := $(wildcard src/*.cpp) $(wildcard src/ecs/*.cpp) $(wildcard src/graphics/*.cpp)
GLAD_SRC := src/glad/src/glad.c
OBJ := $(filter-out src/test_sdl.o src/main_test.o, $(patsubst %.cpp,%.o,$(SRC)))
GLAD_OBJ := $(patsubst %.c,%.o,$(GLAD_SRC))
TARGET := star-engine

all: $(TARGET)

# Compile GLAD first (C code)
$(GLAD_OBJ): $(GLAD_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Link with GLAD object file
$(TARGET): $(GLAD_OBJ) $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(GLAD_OBJ) $(OBJ) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: tests/test_simulation tests/test_camera tests/test_camera_follow tests/test_camera_edgecases tests/test_camera_presets tests/test_ship_assembly tests/test_shield_energy tests/test_feedback_systems tests/test_text_rendering tests/test_ecs_v2 tests/test_physics tests/test_solar_system

tests/test_simulation: tests/test_simulation.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_simulation.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_camera: tests/test_camera.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_camera.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_camera_follow: tests/test_camera_follow.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_camera_follow.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_camera_edgecases: tests/test_camera_edgecases.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_camera_edgecases.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_camera_presets: tests/test_camera_presets.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_camera_presets.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_ship_assembly: tests/test_ship_assembly.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_ship_assembly.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_shield_energy: tests/test_shield_energy.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_shield_energy.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_feedback_systems: tests/test_feedback_systems.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_feedback_systems.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_text_rendering: tests/test_text_rendering.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_text_rendering.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_ecs_v2: tests/test_ecs_v2.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_ecs_v2.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_physics: tests/test_physics.cpp $(filter-out src/main.o,$(OBJ))
	$(CXX) $(CXXFLAGS) -I./src -o $@ tests/test_physics.cpp $(filter-out src/main.o,$(OBJ)) $(LDLIBS)

tests/test_solar_system: tests/test_solar_system.cpp src/SolarSystem.cpp src/CelestialBody.cpp src/Transform.cpp src/ecs/EntityManager.cpp
	$(CXX) $(CXXFLAGS) -I./src -o $@ \
		tests/test_solar_system.cpp \
		src/SolarSystem.cpp \
		src/CelestialBody.cpp \
		src/Transform.cpp \
		src/ecs/EntityManager.cpp

clean:
	rm -f $(OBJ) $(GLAD_OBJ) $(TARGET) tests/test_simulation tests/test_camera tests/test_camera_follow tests/test_camera_edgecases tests/test_camera_presets tests/test_ship_assembly tests/test_shield_energy tests/test_feedback_systems tests/test_text_rendering tests/test_ecs_v2 tests/test_physics tests/test_solar_system

.PHONY: all test clean

