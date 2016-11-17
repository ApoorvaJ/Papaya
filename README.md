Papaya is a free, open-source and cross-platform image editor.

![screenshot 1](/web/img.0.0.jpg?raw=true)

Papaya is completely free. The entire source code is open and is MIT-licensed, which means anyone is free to use and extend the code, even for commercial purposes.

Papaya runs natively on Windows and Linux. There is an OS X fork at [chemecse/Papaya](https://github.com/chemecse/Papaya/tree/osx-dev).

Download
--------

You can download a stable, inital version of Papaya on the [Releases page](https://github.com/ApoorvaJ/Papaya/releases).

Development status
------------------

Papaya is a part-time project, and very much a work in progress.

The current development focus is on getting a node infrastructure in place that will enable layering of bitmaps and effects.

The following features are functional so far:
* Brush with adjustable size, hardness and opacity
* Eye dropper
* Color picker
* Loading jpg & png images, saving png images
* Undo/Redo
* Drawing tablet support with pressure sensitivity

For development updates, you can follow [@PapayaEditor](https://twitter.com/PapayaEditor) on Twitter.

Building
--------

To build on Linux, go to `build/linux/` in your Papaya folder, and run the `make` command in your terminal. Papaya currently depends on GTK+ on Linux, for open/save dialog boxes, so you will need to install that before building Papaya.

To build on Windows, go to `build/windows` and open the Visual Studio 2015 solution. You should also be able build successfully in older versions of Visual Studio by changing the `Platform Toolset` in the Project Properties page in the General tab.

**Papaya's master branch is currently very unstable because some [foundational work](https://handmade.network/forums/t/1561) is being done, on adding nodes for layering and effects. Please do not report segfaults or build failures on the master branch until the branch stabilizes, at which point I will again welcome bug reports and pull requests.**

**If you still want to explore and play with code, switch to the 0.1 release, but be warned - the code has changed substantially since.**

<!-- Contributing -->

<!-- Papaya welcomes your contributions. If you are interested in helping, please read the pertinent [wiki page](https://github.com/ApoorvaJ/Papaya/wiki/Contributing-to-Papaya), an [overview of the structure of the project](https://github.com/ApoorvaJ/Papaya/wiki/Project-structure), and check out the [open issues](https://github.com/ApoorvaJ/Papaya/issues). -->

Credits
------

Developed by [Apoorva Joshi](http://apoorvaj.io/) with the help of many wonderful people who have contributed directly and indirectly. I plan to have a more detailed credits section in-application when we ship a more substantial version.

Uses Omar Cornut's [ImGui](https://github.com/ocornut/imgui) for UI.

Uses Sean Barrett's [stb libraries](https://github.com/nothings/stb).

Inspired by [Casey Muratori](http://mollyrocket.com/casey/about.html) and his excellent educational project [Handmade Hero](https://handmadehero.org/).

The photo featured in the image above is creative commons licensed by its original creator, [Bipin](https://www.flickr.com/photos/brickartisan/16846948646/in/photostream/).

License
-------

Papaya is licensed under the MIT License, see LICENSE for more information.
