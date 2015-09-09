g++ -o papaya  ../../src/linux_papaya.cpp -ldl -lGL -lX11 -O0 `pkg-config --cflags --libs gtk+-2.0` && ./papaya
