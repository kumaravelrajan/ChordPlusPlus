# Initial Report

#### Team Name/Members, Implementation
- Team Name: DHT-4
- Team Members:
  - Kumaravel Rajan
  - Maximilian Barmetler
- Chosen Implementation: Chord

#### Chosen programming language and operating system and reasons for choosing them
- Programming Language: C++, because it's a compiler language and because of that very efficient.
- Operating system: linux, so that it can be easily dockerized and used on any system.

#### Build System:
- cmake, gcc (later docker for build environment)

#### Intended measures to guarantee quality of your software
- ctest, because it is part of cmake
- Quality control: clang-format, cppcheck(maybe?)
- ...

#### Available libraries
- benhoyt/inih
- cap'n Proto
- ASIO

#### License you intend to assign your project’s software and reasons for doing so
- MIT license
- GNU General Public License 3
   
#### Previous programming experience of your team members, which is relevant to this project
- Maxi:
  - Game Engine Design, Game Physics, Concepts of C++ programming (currently)
  - ASP, GAD
- Kumaravel:
  - Embedded systems

#### Planned workload distribution
- Maxi
  - API
  - DHT implementation
- Kumaravel
  - Conf Parsing / Command-line Arguments
  - DHT implementation
