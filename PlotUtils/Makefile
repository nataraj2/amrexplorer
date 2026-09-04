# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -I../include -I../build/_deps/flatbuffers-src/include

# Target executable
TARGET = slice_to_png

# Source file
SRCS = slice_to_png.cpp
OBJS = $(SRCS:.cpp=.o)

# Library paths
LDFLAGS = -L../build/src/cache \
          -L../build/src/core \
          -L../build/src/data \
          -L../build/src/expression \
          -L../build/src/io \
          -L../build/src/pipeline \
          -L../build/src/query \
          -L../build/src/remote \
          -L../build/src/render2d \
          -L../build/src/render3d \

# Libraries - Order matters for static libraries
# We list libraries in reverse dependency order (highest level first)
LIBS = -lamrexplorer_data \
       -lamrexplorer_pipeline \
       -lamrexplorer_render2d \
       -lamrexplorer_render3d \
       -lamrexplorer_query \
       -lamrexplorer_io \
       -lamrexplorer_expression \
       -lamrexplorer_core \
       -lamrexplorer_cache \
       -lpthread -ldl

# Default rule
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
