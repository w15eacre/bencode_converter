# bencode_converter
[![Build and Test](https://github.com/w15eacre/bencode_converter/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/w15eacre/bencode_converter/actions/workflows/build_and_test.yml)
[![clang-format-command](https://github.com/w15eacre/bencode_converter/actions/workflows/clang_format_review.yml/badge.svg?branch=master)](https://github.com/w15eacre/bencode_converter/actions/workflows/clang_format_review.yml)
[![clang-tidy-review](https://github.com/w15eacre/bencode_converter/actions/workflows/clang_tidy_review.yml/badge.svg?branch=master)](https://github.com/w15eacre/bencode_converter/actions/workflows/clang_tidy_review.yml)

The header-only library to convert to\from bencode format. This is library based on the std::variant that gave you ability to use the std library to get a value.

This library allows to unpack a parsed variant to homogeneous/heterogeneous container with C++ types that give you convenient use this library.

### Supported compilers
- GCC

### Requirements
- A Compiler must support C++20

### Dependencies
- GTest
- fmt
