# Talk!

TODO: Write a project description

Cross-platform server/client chat service

Testing is done on:
  + Fedora 23
  + ElementaryOS Freya
  + Windows 7 64bit
  + Windows 10 64bit

## Prerequisites

TODO: Write prerequisites

*CMake* >= 2.6

## Building

TODO: Describe the installation process

- Create a build/ directory
- From inside the newly created directory type **cmake ../**
- Now you can build & compile on your system!
  + on Windows: open the resulting **VisualStudio** Solution
  + on GNU/Linux: type **make**

## Usage

TODO: Write usage instructions

- Launch build/src/server/**tlk_server** with:
 + *port_number*

- Launch build/src/client/**tlk_client** with:
 + *IP_address*
 + *port_number*

Where *port_number* must be equal and *IP_address* must be the IP address of the server we're connecting to

e.g. (local machine test):
  - ./build/src/server/tlk_server 3000
  - ./build/src/client/tlk_client 127.0.0.1 3000

## Contributing

TODO: Write a contributions how-to

## History

TODO: Write history

## Credits

TODO: Write credits

## License

TODO: Write license
