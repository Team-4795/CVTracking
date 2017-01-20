SRC_FILES = $(wildcard src/*.cpp)
BUILD_FILES = $(patsubst src/%.cpp, build/%.o, ${SRC_FILES})
LIBS_OPENCV = $(shell pkg-config --libs opencv)

all: build ${BUILD_FILES}
	g++ -o build/CVTracking ${BUILD_FILES} $(LIBS_OPENCV)
clean:
	-rm -rf build/
build/%.o: src/%.cpp
	g++ -std=gnu++11 -c -g -o $@ $^
build:
	mkdir build
