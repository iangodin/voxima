
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout

class MyProjectConan(ConanFile):
  name = "canim"
  version = "0.0"
  settings = "os", "compiler", "build_type", "arch"

  def requirements(self):
    self.requires("stb/cci.20240531")
    self.requires("catch2/3.13.0")
    self.requires("cpptrace/1.0.4")
    self.requires("lunasvg/3.5.1")
    self.requires("plutovg/1.3.2")
    self.requires("freetype/2.14.1")
    self.requires("harfbuzz/12.3.0")
    self.requires("doctest/2.4.12")
    self.requires("cli11/2.6.2")
    self.requires("ctre/3.10.0")

  def generate(self):
    deps = CMakeDeps(self)
    deps.generate()
    tc = CMakeToolchain(self)
    tc.generate()

  def layout(self):
    cmake_layout(self)
