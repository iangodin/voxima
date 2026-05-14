#pragma once

#include <utils/Huml.h>

#include <filesystem>
#include <memory>
#include <string>

class Script
{
public:
  Script() = default;
  Script(huml::Value &&root);

  Script(const Script &) = default;
  Script(Script &&) = default;

  ~Script() = default;

  Script &operator=(const Script &) = default;
  Script &operator=(Script &&) = default;

  void set_directory(std::filesystem::path directory)
  {
    _directory = std::move(directory);
  }

  const std::filesystem::path &directory() const
  {
	  return _directory;
  }

  bool load(const std::string &lang);

  const huml::Value &root() const {
    if (_impl) {
      return _impl->root;
    } else {
      return _missing;
    }
  }

private:
  struct Impl {
    huml::Value root;
  };

  huml::Value load_recursive(const std::string &lang);

  std::shared_ptr<Impl> _impl = nullptr;
  huml::Value _missing = {huml::Missing{}};
  std::filesystem::path _directory;
};
