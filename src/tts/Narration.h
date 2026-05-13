#pragma once

#include "TTSClient.h"

#include <audio/Microphone.h>

#include <string>
#include <vector>

struct Stage;

namespace tts {

class Narration {
public:
  void synthesize(Stage &stage, const std::string &speaker,
                  const std::string &language,
                  const std::vector<std::string> &text);

  void speak(Microphone &mic, size_t samples = 2000);

  bool is_done() const;

private:
  std::vector<SynthesisResult> _results;
  size_t _current_result_index = 0;
  size_t _offset = 0;
};

} // namespace tts
