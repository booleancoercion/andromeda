# andromeda
A remake of my [earlier website](https://github.com/booleancoercion/boolco.dev), but with a bit more ✨pizzazz✨. Also in C++ this time, because let's face it, it's harder than doing it in Rust.

## Building
This project was only tested on a linux-based system.

First, make sure you've cloned the repo with all the submodules:
```bash
git clone https://github.com/booleancoercion/andromeda --recurse-submodules
```

Then, to build, make sure you have:
- CMake >= 3.19 (tested with 4.3.3)
- Ninja (tested with 1.13.2)
- Clang (tested with 22.1.6)
- Python 3 (tested with 3.14.5 - necessary for MbedTLS)

And run:
```bash
cmake --preset <preset>
cmake --build --preset <preset>
```
Where `<preset>` can be one of `clang-debug`, `clang-release`.

## Running
The executable will be generated under either `build/debug` or `build/release`, depending on your preset. There is also a debug vscode run configuration.
To run, just execute:
```bash
andromeda
```
However, you need to make sure that you have a configuration file. This is the file that I use:
```json
{
    "listen_urls": [
        "http://0.0.0.0:8080"
    ],
    "tls_key": "key.pem",
    "tls_cert": "cert.pem",
    "db": "andromeda.db"
}
```
(Where key.pem and cert.pem are supplied by me, and andromeda.db will be created if it doesn't already exist)