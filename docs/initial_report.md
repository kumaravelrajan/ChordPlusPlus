# Initial Report

#### Team Name/Members, Implementation
- Team Name: DHT-4
- Team Members:
  - Kumaravel Rajan
  - Maximilian Barmetler
- Chosen Implementation: Chord

#### Chosen programming language and operating system and reasons for choosing them
- **Programming Language**: C++, because of its efficiency benefits. We chose it over C mainly because namespaces, operators, templates, and easier memory management.
- **Operating system**: linux, so that it can be easily dockerized and used on any system.

#### Build System:
- cmake. For now only works with gcc and clang, but we might make it work with Visual C++. But because WSL and docker exist, there is no real incentive to do so.

#### Intended measures to guarantee quality of your software
- ctest
- **Quality control**: clang-format, clang-tidy, cppcheck(maybe?)
- Client scripts for testing the api, or even the p2p connection.

#### Available libraries
- benhoyt/inih
- cap'n proto
- asio standalone

We might use CPM for handling libraries. For now, however, they are found in `/usr/lib`, and in the future the Dockerfile will make sure that they are installed.

#### License you intend to assign your project’s software and reasons for doing so
**MIT** license

The MIT license in incredibly simple and short. Also, it has no restrictions as to what can be done with the project, but protects the authors from being liable for any damage caused by the code.

Since we don't know what license the VoidPhone project will use, the MIT license is good, since it explicitly allows sublicensing.

Since this is just an educational project, the relevance of this is debatable, so the decision was based on the hypothetical scenario of this being a real product.
   
#### Previous programming experience of your team members, which is relevant to this project
- Maxi:
  - Game Engine Design, Game Physics, Concepts of C++ programming (currently)
  - ASP, GAD
- Kumaravel:
  - Embedded systems

#### Planned workload distribution
- Maxi
  - **API**: Message [de-]serialization, asynchronous networking, error handling
  - **DHT** implementation
- Kumaravel
  - **Conf** Parsing / Command-line **Arguments**
  - **DHT** implementation
