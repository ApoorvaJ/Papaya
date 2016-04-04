SRC =../../src
CFLAGS=-I$(SRC) -O0 -Wno-write-strings -Wno-format-security
LIBS=-ldl -lGL -lX11 -lXi `pkg-config --cflags --libs gtk+-2.0` -DUSE_GTK
FINAL=$(SRC)/papaya_platform_linux.cpp papaya_core.o papaya_lib.o papaya_util.o imgui.o


papaya: $(FINAL) img
	g++ -MMD -MF $@.d $(FINAL) $(LIBS) $(CFLAGS) -o $@

papaya_core.o: $(SRC)/papaya_core.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

papaya_lib.o: $(SRC)/papaya_lib.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

papaya_util.o: $(SRC)/papaya_util.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

imgui.o: $(SRC)/imgui.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

img: ../../img/ui.png
	cp -u $^ .

clean:
	rm -f *.d *.o *.png papaya
