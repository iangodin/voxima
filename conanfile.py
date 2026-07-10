
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout

class MyProjectConan(ConanFile):
  name = "Sonimation"
  version = "0.0"
  settings = "os", "compiler", "build_type", "arch"
  generators = "CMakeDeps"

  def requirements(self):
    self.requires("catch2/3.13.0")
    self.requires("cpptrace/1.0.4")
    self.requires("lunasvg/3.5.0")
    self.requires("plutovg/1.3.2")
    self.requires("freetype/2.14.1")
    self.requires("harfbuzz/12.3.0")
    self.requires("cli11/2.6.2")
    self.requires("ctre/3.10.0")
    self.requires("asio/1.36.0")
    self.requires("capnproto/1.1.0")
    self.requires("libsndfile/1.2.2")
    self.requires("soxr/0.1.3")
    self.requires("fribidi/1.0.13")
    self.requires("fontconfig/2.17.1")
    self.requires("clipper2/2.0.1")

  def layout(self):
    cmake_layout(self, generator="Ninja Multi-Config")

  def generate(self):
    tc = CMakeToolchain(self)
    tc.generator = "Ninja"
    tc.generate()

