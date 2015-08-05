Papaya is an open source GPU-powered image editor built using C++ and OpenGL.

![screenshot 1](/web/screen_01.png?raw=true)

Papaya is designed to be fast. It starts up quickly, and is already [noticeably faster](http://thegamecoder.com/building-a-fast-modern-image-editor/) than other popular editors at zooming, panning and brush rendering.

Papaya is very much a work in progress, and is not ready for real world usage. For development updates, you can follow [@PapayaEditor](https://twitter.com/PapayaEditor) on Twitter.

Building
--------

Papaya supports Windows and Linux, and is set up to use single file compilation. Compile only win32_papaya.cpp or linux_papaya.cpp, and you're done.

Project files for Visual Studio 2015, 2013 and 2012 and shell build scripts for gcc are provided in the /build folder.

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
