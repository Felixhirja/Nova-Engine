SRC = src/main.cpp src/MainLoop.cpp src/Viewport3D.cpp src/Simulation.cpp src/Player.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = star-engine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
