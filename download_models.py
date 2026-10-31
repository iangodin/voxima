#! /usr/bin/env python3

# Downloads the text-to-speech models (only needed for narration) from Hugging Face:
#
#     python3 download_models.py [output_dir] [--all]
#
# They go to output_dir (default "models", where voxima looks, run from here), in the folders voxima's defaults
# expect. Files already there are skipped, and an interrupted download is resumed.
#
# Both are Q8_0 GGUF conversions in https://huggingface.co/audio-cpp/audio.cpp-gguf. A conversion adds no terms
# of its own; each model is under the license of the original: Chatterbox (MIT) and Qwen3-ForcedAligner (Apache-2.0).
#
# --all also downloads the F16 versions: larger and a little better, used with --tts-model / --aligner-model.

import os
import sys
import requests

repo = "https://huggingface.co/audio-cpp/audio.cpp-gguf/resolve/main/"

args = [ a for a in sys.argv[1:] if not a.startswith("--") ]
output_dir = args[0] if args else "models"
get_all = "--all" in sys.argv

models = [
  ("Chatterbox speech model (Q8_0, 2.1 GB)", "Chatterbox-GGUF/chatterbox-q8_0.gguf"),
  ("Qwen3 forced aligner (Q8_0, 1.1 GB)", "Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-q8_0.gguf"),
]
if get_all:
  models += [
    ("Chatterbox speech model (F16, 3.7 GB)", "Chatterbox-GGUF/chatterbox-f16.gguf"),
    ("Qwen3 forced aligner (F16, 1.8 GB)", "Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-f16.gguf"),
  ]

def download(name, path):
  destination = os.path.join(output_dir, path)
  os.makedirs(os.path.dirname(destination), exist_ok=True)
  print(f"Downloading {name}")

  head = requests.head(repo + path, allow_redirects=True)
  head.raise_for_status()
  size = int(head.headers["Content-Length"])

  if os.path.exists(destination) and os.path.getsize(destination) >= size:
    print(f"  Already have {path}")
    return

  # Download to .part, resuming it if it is there already
  part = destination + ".part"
  have = os.path.getsize(part) if os.path.exists(part) else 0
  headers = { "Range": f"bytes={have}-" } if have else {}
  with requests.get(repo + path, headers=headers, stream=True) as response:
    response.raise_for_status()
    if have and response.status_code != 206:
      have = 0 # the server sent it all again
    with open(part, "ab" if have else "wb") as out:
      for chunk in response.iter_content(chunk_size=1 << 20):
        out.write(chunk)
        have += len(chunk)
        print(f"\r  {have * 100 // size}% of {size / 1e9:.2f} GB", end="", flush=True)
  print()

  if os.path.getsize(part) != size:
    sys.exit(f"  Incomplete: {part} is not {size} bytes; run again to resume")
  os.replace(part, destination)
  print(f"  Saved {destination}")

for name, path in models:
  download(name, path)
