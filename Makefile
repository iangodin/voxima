.PHONY: default all config clean debug

default: all

all:
	@cmake --build build --config Release

debug:
	@cmake --build build --config Debug

clean:
	@cmake -E remove_directory build

test: debug
	@ctest --preset conan-default --build-config Debug --output-on-failure

conan:
	@conan install . --build=missing -s build_type=Debug
	@conan install . --build=missing -s build_type=Release

config: conan
	@cmake --preset conan-default -G "Ninja Multi-Config"

