# Variables
CXX = /usr/bin/g++
CXXFLAGS = -std=c++17 -fdiagnostics-color=always -Wall -g
INCLUDES = -I./header -I./include -I./ 
LIBS = -L./lib -lglfw.3
FRAMEWORKS = -framework OpenGL
RPATH = -Wl,-rpath,./lib
SOURCES = $(wildcard ./src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = ./main.exe

# Default target
all: $(SOURCES) $(EXECUTABLE)

# Linking
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(INCLUDES) ./src/glad.c $(LIBS) $(FRAMEWORKS) $(RPATH) -o $@

# Compilation
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)