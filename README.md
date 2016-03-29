Papaya is a free and open-source GPU-powered image editor, built with the following goals:

![screenshot 1](/web/img.0.0.jpg?raw=true)

**Fast:** Papaya is designed for efficiency. It starts up quickly, and is already [significantly faster](http://apoorvaj.io/building-a-fast-modern-image-editor.html) than other popular editors at zooming, panning and brushing.

**Open:** Papaya is completely free. The entire source code is open and is MIT-licensed, which means anyone is free to use and extend the code, even for commercial purposes.

**Beautiful:** While most other open editors use bulky UI toolkits, Papaya is rendered entirely on the GPU, with pixel-perfect precision and complete control over the visual design. I'm designing Papaya to be as beautiful as the images it will help create.

**Cross-platform:** Papaya runs natively on Windows and Linux.

Development status
------------------

Papaya is very much a work in progress. The following features are functional so far:
* Brush with adjustable size, hardness and opacity
* Eye dropper
* Color picker
* Loading jpg & png images, saving png images
* Undo/Redo
* Drawing tablet support with pressure sensitivity

For development updates, you can follow [@PapayaEditor](https://twitter.com/PapayaEditor) on Twitter.

Building
--------

(I am currently in the process of changing the build process a little, so the following instructions might be a bit rough around the edges.)

To build on Linux, go to `build/linux/` in your Papaya folder, and run the `make` command in your terminal. Papaya currently depends on GTK+ on Linux, for open/save dialog boxes, so you will need to install that before building Papaya.

To build on Windows, go to `build/windows` and open the Visual Studio 2015 solution. You should also be able build successfully in older versions of Visual Studio by changing the `Platform Toolset` in the Project Properties page in the General tab.

Credits
------

Developed by [Apoorva Joshi](http://apoorvaj.io/).

Uses Omar Cornut's [ImGui](https://github.com/ocornut/imgui) for UI.

Uses Sean Barrett's [stb libraries](https://github.com/nothings/stb).

Inspired by [Casey Muratori](http://mollyrocket.com/casey/about.html) and his excellent educational project [Handmade Hero](https://handmadehero.org/).

The photo featured in the image above is creative commons licensed by its original creator, [Bipin](https://www.flickr.com/photos/brickartisan/16846948646/in/photostream/).

License
-------

Papaya is licensed under the MIT License, see LICENSE for more information.
