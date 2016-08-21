VPATH=../../src                 \
	  ../../src/core            \
	  ../../src/libs            \
	  ../../src/libs/imgui

SRCS=papaya_platform_linux.cpp  \
	 papaya_core.cpp            \
	 imgui.cpp                  \
	 single_header_libs.cpp     \
	 crop_rotate.cpp            \
	 picker.cpp                 \
	 prefs.cpp                  \
	 undo.cpp 

OBJS=$(subst .cpp,.o,$(SRCS))
LIBS=-ldl -lGL -lX11 -lXi `pkg-config --cflags --libs gtk+-2.0` -DUSE_GTK
CFLAGS=-I../../src -O0 -Wno-write-strings -Wno-format-security -g


papaya: $(OBJS) misc_data
	g++ $(OBJS) $(LIBS) $(CFLAGS) -o $@

$(OBJS): %.o: %.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

misc_data: ../../img/ui.png ../../src/shaders
	cp -ru $^ .

clean:
	rm -f *.d *.o *.png papaya
	rm -rf shaders

