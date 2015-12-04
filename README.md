Papaya is a free and open-source GPU-powered image editor, built with the following goals:

![screenshot 1](/web/img.0.0.jpg?raw=true)

**Fast:** Papaya is designed for efficiency. It starts up quickly, and is already [significantly faster](http://thegamecoder.com/building-a-fast-modern-image-editor/) than other popular editors at zooming, panning and brushing.

**Open:** Papaya is completely free. The entire source code is open and is MIT-licensed, which means anyone is free to use and extend the code, even for commercial purposes.

**Beautiful:** While most other open editors use bulky UI toolkits, Papaya is rendered entirely on the GPU, with pixel-perfect precision and complete control over the visual design. I'm designing Papaya to be as beautiful as the images it will help create.

**Cross-platform:** Papaya runs natively on Windows and Linux.

Development status
------------------

Papaya is very much a work in progress. The following features are functional so far:
* Brush with adjustable size, hardness and opacity
* Color picker
* Loading jpg & png images, saving png images
* Undo/Redo
* Drawing tablet support with pressure sensitivity

For development updates, you can follow [@PapayaEditor](https://twitter.com/PapayaEditor) on Twitter.

Building
--------

Papaya supports Windows and Linux, and is set up to use single file compilation. Compile only win32_papaya.cpp or linux_papaya.cpp, and you're done.

Project files for Visual Studio 2015 and shell build scripts for gcc are provided in the /build folder. If you're using other versions of Visual Studio, just set every c/cpp file except win32_papaya.cpp to be treated as a "C/C++ header" item type in the properties window.

Currently, the Linux build relies on GTK+ for open/save dialog boxes, but otherwise linking is optional. The build.sh script provides a flag to turn GTK+ on/off during compilation.

Credits
------

Developed by [Apoorva Joshi](http://thegamecoder.com).

Uses Omar Cornut's [ImGui](https://github.com/ocornut/imgui) for UI.

Uses Sean Barrett's [stb libraries](https://github.com/nothings/stb).

Inspired by [Casey Muratori](http://mollyrocket.com/casey/about.html) and his excellent educational project [Handmade Hero](https://handmadehero.org/).

The photo featured in the image above is creative commons licensed by its original creator, [Bipin](https://www.flickr.com/photos/brickartisan/16846948646/in/photostream/).

License
-------

Papaya is licensed under the MIT License, see LICENSE for more information.
