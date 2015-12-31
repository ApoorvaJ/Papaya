#!/bin/sh
m64=1 # m64=0 : 32-bit compilation
      # m64=1 : 64-bit compilation

gtk=1 # gtk=0 : GTK+ is not linked. File open/save dialogs will not work.
      # gtk=1 : GTK+ 2.0 is linked. File open/save dialogs will work, but might give errors on certain linux distros (e.g. Elementary OS).


cmd="g++ -o papaya  ../../src/linux_papaya.cpp -ldl -lGL -lX11 -lXi -O0"

if [ "$m64" = "1" ] ; then
    cmd="${cmd} -m64"
fi

if [ "$gtk" = "1" ] ; then
    cmd="${cmd} \`pkg-config --cflags --libs gtk+-2.0\` -DUSE_GTK"
fi

cmd="${cmd} && ./papaya"

eval $cmd
