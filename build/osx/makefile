VPATH=../../src              \
	  ../../src/core         \
	  ../../src/libs         \
	  ../../src/libs/imgui   \
	  ../../src/libs/glew

SRCS=papaya_core.cpp         \
	 glew.cpp                \
	 imgui.cpp               \
	 single_header_libs.cpp  \
	 picker.cpp              \
	 prefs.cpp

OBJS=$(subst .cpp,.o,$(SRCS))
LIBS=-framework Cocoa -framework CoreVideo -framework OpenGL
CC=g++
CFLAGS=-I../../src -std=c++11 -O0 -Wno-write-strings -Wno-format-security -Wno-non-pod-varargs -DPAPAYA_NO_EASYTAB_SUPPORT=1

papaya: ../../src/papaya_platform_osx.mm $(OBJS) img
	$(CC) ../../src/papaya_platform_osx.mm $(OBJS) $(LIBS) $(CFLAGS) -o $@

$(OBJS): %.o: %.cpp
	$(CC) -MMD -MF $@.d $< $(CFLAGS) -o $@ -c

img: ../../img/ui.png
	cp $^ .

clean:
	rm -f *.d *.o *.png papaya
