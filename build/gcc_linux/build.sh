#!/bin/sh
gtk=0

if [ "$gtk" = "1" ] ; then
	g++ -o papaya  ../../src/linux_papaya.cpp -ldl -lGL -lX11 -O0 `pkg-config --cflags --libs gtk+-2.0` -DUSE_GTK && ./papaya
else
	g++ -o papaya  ../../src/linux_papaya.cpp -ldl -lGL -lX11 -O0 && ./papaya
fi
