SRC_FILES = $(wildcard src/*.cpp)
BUILD_FILES = $(patsubst src/%.cpp, build/%.o, ${SRC_FILES})
LIBS_OPENCV = -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_objdetect

all: build ${BUILD_FILES}
	g++ -o build/CVTracking ${BUILD_FILES} $(LIBS_OPENCV)
clean:
	-rm -rf build/
build/%.o: src/%.cpp
	g++ -std=gnu++11 -c -g -o $@ $^
build:
	mkdir build
