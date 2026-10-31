.PHONY: default all config clean debug tsan install package docs

default: all

all:
	@cmake --build --preset conan-release

debug:
	@cmake --build --preset conan-debug

clean:
	@cmake -E remove_directory build

# A ThreadSanitizer build of voxima, in build/Tsan (needs "make config" first, for the Conan packages). The
# TTS is CPU-only there. Run it like the Release one: build/Tsan/voxima --preview ...
tsan:
	@cmake -S . -B build/Tsan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/build/Debug/generators/conan_toolchain.cmake \
		-DCMAKE_CXX_FLAGS=-fsanitize=thread -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread \
		-DVOXIMA_TTS_CUDA=OFF
	@cmake --build build/Tsan --target voxima

# Install the Release build: make install PREFIX=<dir> (see examples/ for a plugin project built against it)
install: all
	@cmake --install build/Release --prefix "$(PREFIX)" --component Voxima

# Packages of the Release build, as installed (a .zip on Windows; a .tar.gz and a .deb elsewhere), in build/packages
package: all
	@cpack --config build/Release/CPackConfig.cmake -B build/packages

# The developer manual (doc/manual, with mkdocs-material, see doc/requirements.txt) as a static site in doc/html,
# which is committed, and installed. Run it after changing the manual, and commit the result.
docs:
	@mkdocs build --strict

test: debug
	@ctest --preset conan-debug --output-on-failure

conan:
	@conan install . --build=missing -s build_type=Debug
	@conan install . --build=missing -s build_type=Release

config: conan
	@cmake --preset conan-release -G "Ninja"
	@cmake --preset conan-debug -G "Ninja"

