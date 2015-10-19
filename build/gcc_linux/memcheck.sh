#!/bin/sh
gtk=0 # gtk=0 : GTK+ is not linked. File open/save dialogs will not work.
      # gtk=1 : GTK+ 2.0 is linked. File open/save dialogs will work, but might give errors on certain linux distros (e.g. Elementary OS).


if [ "$gtk" = "1" ] ; then
    g++ -o papaya  ../../src/linux_papaya.cpp -ldl -lGL -lX11 -O0 -g `pkg-config --cflags --libs gtk+-2.0` -DUSE_GTK && ./papaya
else
    g++ -o papaya  ../../src/linux_papaya.cpp -ldl -lGL -lX11 -O0 -g && valgrind ./papaya
fi
