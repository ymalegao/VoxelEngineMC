# Variables
CXX = /usr/bin/g++
CXXFLAGS = -std=c++17 -fdiagnostics-color=always -Wall -g
INCLUDES = -I./header -I./include -I./ 
LIBS = -L./lib -lglfw.3
FRAMEWORKS = -framework OpenGL
RPATH = -Wl,-rpath,./lib
IMGUI_DIR = ./include/imgui
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp \
                $(IMGUI_DIR)/imgui_demo.cpp \
                $(IMGUI_DIR)/imgui_draw.cpp \
                $(IMGUI_DIR)/imgui_tables.cpp \
                $(IMGUI_DIR)/imgui_widgets.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp



SOURCES = $(wildcard ./src/*.cpp) $(IMGUI_SOURCES)
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