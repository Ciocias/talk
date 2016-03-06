.phony: build clean

build:
	mkdir build && cd build && cmake .. && make && cd ..

clean:
	rm -rf build
