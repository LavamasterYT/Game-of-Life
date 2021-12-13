# Game-of-Life
John Conway's Game of Life created in C using SDL

# Building
If on Windows, make sure you have the following installed:
- MinGW
- SDL

Then, run `make` in the command line. The binary should be located in `build/gol.exe`.

It should be easy to compile on other platforms. You just need to compile [this version of NativeFileDialog](https://github.com/btzy/nativefiledialog-extended), put the libraries in the folders, and edit the Makefile. This is my first real project in C so sorry if it sucks.

# Libraries used
- [NativeFileDialog-extended](https://github.com/btzy/nativefiledialog-extended)
- [SDL](https://github.com/libsdl-org/SDL)
