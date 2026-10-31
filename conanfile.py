
import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy

# For working on Sonimation (make config: conan install .), and for packaging it (conan create .), so others can
# build plugins of scenes against it (see examples/).
class SonimationConan(ConanFile):
  name = "sonimation"
  version = "0.1.0" # as in CMakeLists.txt's project()
  license = "MIT"
  settings = "os", "compiler", "build_type", "arch"
  generators = "CMakeDeps"
  exports_sources = (
    "CMakeLists.txt", "cmake/*", "src/*", "extern/*", "themes/*", "tools/*", "tests/*", "demos/*",
    "download_fonts.py", "LICENSE", "THIRD_PARTY_NOTICES.md", "doc/*", "tts.md", "examples/*" )

  def requirements(self):
    self.test_requires("catch2/3.13.0")
    self.requires("cpptrace/1.0.4")
    self.requires("lunasvg/3.5.0")
    self.requires("plutovg/1.3.2")
    self.requires("freetype/2.14.1")
    self.requires("harfbuzz/12.3.0")
    self.requires("cli11/2.6.2")
    self.requires("ctre/3.10.0")
    self.requires("libsndfile/1.2.2")
    self.requires("soxr/0.1.3")
    self.requires("fribidi/1.0.13")
    self.requires("fontconfig/2.17.1")
    self.requires("clipper2/2.0.1")
    self.requires("sdl/3.4.14")
    # Syntax highlighting. The grammars ask for an older tree-sitter; the newer one reads their parsers too.
    self.requires("tree-sitter/0.26.9", force=True)
    self.requires("tree-sitter-c/0.23.2")
    self.requires("tree-sitter-cpp/0.23.4")

  def layout(self):
    cmake_layout(self, generator="Ninja Multi-Config")

  def generate(self):
    tc = CMakeToolchain(self)
    tc.generator = "Ninja"
    tc.generate()

    # The licenses of the libraries it is built with, as their packages ship them, for CMake to install with it
    # (licenses/conan/<package>/, see THIRD_PARTY_NOTICES.md). Not the ones only for tests, or for building.
    for require, dependency in self.dependencies.host.items():
      if require.test:
        continue
      copy(self, "*", os.path.join(dependency.package_folder, "licenses"),
           os.path.join(self.generators_folder, "licenses", dependency.ref.name))

  def build(self):
    cmake = CMake(self)
    cmake.configure()
    cmake.build()

  def package(self):
    # Only Sonimation's own: audio.cpp has install rules of its own (see CMakeLists.txt)
    cmake = CMake(self)
    cmake.install(component="Sonimation")
    copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))

  def package_info(self):
    # Found with its own config (find_package( Sonimation ) gives Sonimation::core and sonimation_add_plugin),
    # not one made by Conan
    self.cpp_info.set_property("cmake_find_mode", "none")
    # The package itself, as a CMake prefix: find_package finds lib/cmake/Sonimation in it, on every platform
    self.cpp_info.builddirs = ["."]
    self.cpp_info.libs = ["sonimation_core"]
    self.cpp_info.includedirs = [os.path.join("include", "sonimation")]
