.PHONY: default all config clean debug

default: all

all:
	@cmake --build --preset conan-release

debug:
	@cmake --build --preset conan-debug

clean:
	@cmake -E remove_directory build

test: debug
	@ctest --preset conan-default --output-on-failure

conan:
	@conan install . --build=missing -s build_type=Debug
	@conan install . --build=missing -s build_type=Release

config: conan
	@cmake --preset conan-release -G "Ninja"
	@cmake --preset conan-debug -G "Ninja"

