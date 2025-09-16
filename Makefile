CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS  := $(shell pkg-config --libs sdl2 2>/dev/null)

ifeq ($(SDL_LIBS),)
$(info SDL2 not found; building ASCII fallback)
else
CXXFLAGS += $(SDL_CFLAGS) -DUSE_SDL
LDLIBS += $(SDL_LIBS)
$(info SDL2 detected: building with SDL2 support)
endif

SRC := $(wildcard src/*.cpp) $(wildcard src/ecs/*.cpp)
OBJ := $(patsubst %.cpp,%.o,$(SRC))
TARGET := star-engine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDLIBS)

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

