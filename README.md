MSRT
==============
***microStudio Native Runtime***

The **MSRT** is an attempt to create a native platform for the **microStudio** web engine by **Koffeeware**. It is built here in C++ with performance in mind, though the original JS scripts are still being used through QuickJS. It is expected to be a runtime fit for desktop platforms as well as for consoles.

## Libraries

* Raylib *(5.5)*
* QuickJS
* Minizip
* rectpack2D
* robin_hood.h
* GTK3 on Linux *(not included, as to abide by LGPL-2.1)*

## Building

The building process has only been tested on Linux so far, it is a simple CMake script that fetches everything the project needs before building it.

```
cmake --build ./build --target all --
```

## License

TBD

## AI Disclosure

* None, except for some assistance with the CMake build script.