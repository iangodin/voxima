#! /usr/bin/env python3

# Downloads the text-to-speech models (only needed for narration) from Hugging Face:
#
#     python3 download_models.py [output_dir] [--all] [--list]
#
# They go to output_dir, in the folders voxima expects. Without it they go where voxima looks: "models" here, in a
# source tree, or share/voxima/models when this is the installed script. Files already there are skipped, and an
# interrupted download is resumed.
#
# Both are Q8_0 GGUF conversions in https://huggingface.co/audio-cpp/audio.cpp-gguf. A conversion adds no terms
# of its own; each model is under the license of the original: Chatterbox (MIT) and Qwen3-ForcedAligner (Apache-2.0).
#
# --all also downloads the F16 versions: larger and a little better, used with --tts-model / --aligner-model.

import argparse
import os
import sys

repo = "https://huggingface.co/audio-cpp/audio.cpp-gguf/resolve/main/"

# A source tree has the build next to this script; an installed one is in share/voxima, with the models beside it
here = os.path.dirname(os.path.realpath(__file__))
default_dir = "models" if os.path.exists(os.path.join(here, "CMakeLists.txt")) else os.path.join(here, "models")

parser = argparse.ArgumentParser(description = "Download the text-to-speech models (only needed for narration).", allow_abbrev = False)
parser.add_argument("output_dir", nargs = "?", default = default_dir, help = "where they go (default: %(default)s)")
parser.add_argument("--all", action = "store_true", help = "also the larger F16 versions, used with --tts-model / --aligner-model")
parser.add_argument("--list", action = "store_true", help = "only say what would be downloaded, and where")
options = parser.parse_args()
output_dir = options.output_dir
get_all = options.all

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

if options.list:
  for name, path in models:
    print(f"{name}: {os.path.join(output_dir, path)}")
  sys.exit()

import requests # (only now, so that --help and --list work without it)

for name, path in models:
  download(name, path)
