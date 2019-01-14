# selain

Experiment on how to build a browser UI with Vi like keyboard bindings that
uses [WebKit] as it's engine.

## How to compile

Selain requires these two libraries to be installed:

- [GTKmm]
- [WebKitGTK]

You also need [CMake] to compile this application. On Ubuntu, all of these can
be installed with following command:

```bash
$ sudo apt install libgtkmm-3.0-dev libwebkit2gtk-4.0-dev cmake
```

After dependencies have been installed, you can compile the application like
this:

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```

After this, you can run the application by executing `selain` binary found in
the `build` directory, or you can alternatively install it to your system with
`sudo make install`.

[WebKit]: https://webkit.org/
[GTKmm]: https://www.gtkmm.org/
[WebKitGTK]: https://webkitgtk.org/
[CMake]: https://cmake.org/
