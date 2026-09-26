# Tree-sitter highlight queries

`highlights.scm` from the grammars' own repositories, at the versions Conan builds (see `conanfile.py`); the Conan
packages ship only the compiled parsers. MIT licensed (see each `LICENSE`).

- `c/`: tree-sitter-c v0.23.2
- `cpp/`: tree-sitter-cpp v0.23.4. C++ builds on C: its queries are used after C's (see `src/code/Highlight.cpp`).

They are compiled into the program (CMakeLists.txt makes `code/Queries.h` from them).
