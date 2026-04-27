.PHONY: default all config clean debug

default: all

all:
	@cmake --build --preset conan-release

debug:
	@cmake --build --preset conan-debug

clean:
	@cmake -E remove_directory build

test: all
	@ctest --preset conan-release --output-on-failure

conan:
	@conan install . --build=missing -s build_type=Release
	@conan install . --build=missing -s build_type=Debug

config: conan
	@cmake --preset conan-release
	@cmake --preset conan-debug

