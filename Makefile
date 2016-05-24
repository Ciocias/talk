all: build

build:
	mkdir build && cd build && cmake .. && make && cd ..

.PHONY: clean

clean:
	rm -rf build bin
