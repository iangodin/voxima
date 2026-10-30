.PHONY: default all config clean debug tsan install package

default: all

all:
	@cmake --build --preset conan-release

debug:
	@cmake --build --preset conan-debug

clean:
	@cmake -E remove_directory build

# A ThreadSanitizer build of sonimation, in build/Tsan (needs "make config" first, for the Conan packages). The
# TTS is CPU-only there. Run it like the Release one: build/Tsan/sonimation --preview ...
tsan:
	@cmake -S . -B build/Tsan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/build/Debug/generators/conan_toolchain.cmake \
		-DCMAKE_CXX_FLAGS=-fsanitize=thread -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread \
		-DCANIM_TTS_CUDA=OFF
	@cmake --build build/Tsan --target sonimation

# Install the Release build: make install PREFIX=<dir> (see examples/ for a plugin project built against it)
install: all
	@cmake --install build/Release --prefix "$(PREFIX)" --component Sonimation

# Packages of the Release build, as installed (a .zip on Windows; a .tar.gz and a .deb elsewhere), in build/packages
package: all
	@cpack --config build/Release/CPackConfig.cmake -B build/packages

test: debug
	@ctest --preset conan-debug --output-on-failure

conan:
	@conan install . --build=missing -s build_type=Debug
	@conan install . --build=missing -s build_type=Release

config: conan
	@cmake --preset conan-release -G "Ninja"
	@cmake --preset conan-debug -G "Ninja"

