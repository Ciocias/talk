#########################################################
#							#
#  A starting point towards automated project building  #
#							#
#########################################################

.phony: build-linux clean-linux build-win clean-win

build-linux:
	mkdir build && cd build && cmake .. && make && cd ..

clean-linux:
	rm -rf build

build-win:
	mkdir build && cd build && cmake .. && "C:\MinGW\bin\mingw32-make.exe" && cd ..

clean-win:
	del build\* && rmdir build