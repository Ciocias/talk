# Talk!

TODO: Write a project description

Cross-platform server/client chat service

Testing is done on:
  + Fedora 23
  + ElementaryOS Freya
  + Windows 7 64bit
  + Windows 10 64bit

## Prerequisites

General
- **CMake** >= 2.6

Platform dependent:
- **make**
- **VisualStudio**

## Building

On **GNU/Linux** there is a basic Makefile to automate everything

- To build fire up a terminal in the project root and type
```
make build
```
- To clean building files type
```
make clean
```

On **Windows** launch a terminal inside the project root
  - Create a build directory and enter it
  ```
  mkdir build
  cd build
  ```
  - Generate a VisualStudio Solution
  ```
  cmake ..\
  ```
  - Open the resulting Solution file (.sln)

## Usage

For both Windows and GNU/Linux users:

- To Launch a server instance

  ```
  ./build/src/server/tlk_server <port_number>
  ```

- To connect as a client

  ```
  ./build/src/client/tlk_client <ip_address> <port_number> <nickname>
  ```

Where
  - **port_number** is a number between 1024 and 49151
  - **ip_address** is the server address
  - **nickname** is a word of 10 characters at most

Test it in your local machine using *127.0.0.1* as the ip address
