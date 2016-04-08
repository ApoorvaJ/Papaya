VPATH=../../src ../../src/imgui ../../src/glew
SRCS=papaya_platform_linux.cpp \
	 papaya_core.cpp \
	 glew.cpp \
	 imgui.cpp \
	 single_libs.cpp \
	 picker.cpp \
	 prefs.cpp 

OBJS=$(subst .cpp,.o,$(SRCS))
CFLAGS=-I../../src -O0 -Wno-write-strings -Wno-format-security
LIBS=-ldl -lGL -lX11 -lXi `pkg-config --cflags --libs gtk+-2.0` -DUSE_GTK


papaya: $(OBJS) img
	g++ $(OBJS) $(LIBS) $(CFLAGS) -o $@

$(OBJS): %.o: %.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

img: ../../img/ui.png
	cp -u $^ .

clean:
	rm -f *.d *.o *.png papaya
