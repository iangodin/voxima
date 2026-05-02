This design transforms your video/audio generator from a hard-coded application into a data-driven **localization and production engine**. By using HUML as the "Director's Script," you maintain a clean separation between C++ mechanics and creative intent.

### 1. The Core Architecture
*   **Unified Source of Truth:** Each language has one HUML file containing metadata, speaker-to-voice mappings, visual text (LaTeX), and audio prose.
*   **The "Deep Merge" Pipeline:** At load time, regional HUML files (e.g., `fr-CA`) are recursively overlaid onto a base language (e.g., `en-US`). This ensures every key has a valid value while allowing specific regional overrides.
*   **The "Missing" Proxy:** Your C++ `Document` uses a specialized `Missing` value to detect structural errors. In debug mode, these trigger high-visibility **magenta blocks** in the video output.

### 2. Text & Audio Handling
*   **Visual vs. Spoken Separation:** Visuals (via MFL for math or HarfBuzz for labels) are decoupled from audio prose. This allows the script to be "human-friendly" while the screen remains mathematically precise.
*   **Timing & Sync:** C++ coroutines drive the animation, using **Sherpa-ONNX token timestamps** to align audio playback with subtitle line breaks (defined by HUML newlines).
*   **The BiDi Gap:** You will implement a BiDi engine (like FriBidi) to reorder UTF-8 strings into visual order before passing them to HarfBuzz for shaping.

### 3. Data-Driven Sequencing
*   **The "Playlist" Approach:** Instead of a hard-coded flow, a `sequence` list in the HUML dictates the order of scenes.
*   **Self-Contained Scenes:** Each C++ coroutine is responsible for its own state initialization. This removes cross-scene dependencies, enabling perfect **hot-reloading** and the ability to preview any segment instantly.
*   **Logic vs. Parameterization:** HUML remains a configuration format, not a script. It provides parameters (durations, colors, speaker profiles) while the C++ "Stagehands" handle the execution logic.

### 4. Workflow Benefits
*   **High-Speed Preview:** The hot-reloading loop allows script writers to tweak narration or math formulas and see the results immediately without recompilation.
*   **Scalability:** Adding a new language is as simple as creating a new `.huml` file; the engine automatically handles the inheritance, voice casting, and animation stretching to fit the new audio lengths.

This setup gives you the flexibility of a professional animation suite with the performance and rigour of a modern C++ codebase. Ready to start on that BiDi implementation?
