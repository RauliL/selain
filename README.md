# selain

Experiment on how to build a browser UI with Vi like keyboard bindings that
uses [WebKit] as it's engine.

## Features

- Vim style modes and keyboard bindings.
- Vim style splitted views.

## How to compile

Selain requires these two libraries to be installed:

- [GTKmm]
- [WebKitGTK]

You also need [CMake] to compile this application. On Ubuntu, all of these can
be installed with following command:

```shell
sudo apt install libgtkmm-4.0-dev libwebkitgtk-6.0-dev cmake build-essential
```

After dependencies have been installed, you can compile the application like
this:

```shell
git clone https://github.com/RauliL/selain.git
cd selain
cmake -S . -B build
cmake --build build
```

After this, you can run the application by executing `selain` binary found in
the `build` directory, or you can alternatively install it to your system with
`sudo cmake --install build`.

[WebKit]: https://webkit.org/
[GTKmm]: https://www.gtkmm.org/
[WebKitGTK]: https://webkitgtk.org/
[CMake]: https://cmake.org/
