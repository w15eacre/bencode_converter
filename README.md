# bencode_converter
[![Build and Test](https://github.com/w15eacre/bencode_converter/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/w15eacre/bencode_converter/actions/workflows/build_and_test.yml)

The header-only library to convert to\from bencode format. This is library based on the std::variant that gave you ability to use the std library to get a value.

This library allows to unpack a parsed variant to homogeneous/heterogeneous container with C++ types that give you convenient use this library.

### Requires
- C++20

### Supported compiles
- [X] - GCC 11.04
- [ ] - Clang 14
- [ ]  - MSVC

### Dependencies
- GTest
- fmt

#### TODO

- [ ] Implements Hono/Getero container for the List
- [ ] Implements Hono/Getero container for The Dict
- [ ] Concept of BaseType::Dict and BaseType::Str
- [ ] CI clang and MSVC
- [ ] Trasparent for Dictionary
- [ ] Compare performance with other solution
- [ ] Check key order in Dict
- [ ] PVS
- [ ] Tests
    - [X] type_traits
    - [X] details
    - [ ] API
