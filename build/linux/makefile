VPATH=../../src                 \
	  ../../src/ui              \
	  ../../src/ui/components   \
	  ../../src/ui/libs         \
	  ../../src/ui/libs/imgui   \
	  ../../src/libpapaya

SRCS=linux_ui.cpp               \
	 common_ui.cpp              \
	 imgui.cpp                  \
	 imgui_draw.cpp             \
	 imgui_demo.cpp             \
	 single_header_libs.cpp     \
	 crop_rotate.cpp            \
	 metrics_window.cpp         \
	 node.cpp                   \
	 color_panel.cpp            \
	 graph_panel.cpp            \
	 node_properties_panel.cpp  \
	 prefs.cpp                  \
	 undo.cpp                   \
	 libpapaya.cpp

OBJS=$(subst .cpp,.o,$(SRCS))
LIBS=-ldl -lGL -lX11 -lXi `pkg-config --cflags --libs gtk+-2.0` -DUSE_GTK
CFLAGS=-I../../src/ui -I../../src/libpapaya -O0 -g

.SILENT:

papaya: $(OBJS) misc_data
	g++ $(OBJS) $(LIBS) $(CFLAGS) -o $@

$(OBJS): %.o: %.cpp
	g++ -MMD -MP -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

-include $(OBJS:.o=.o.d)

misc_data: ../../img/ui.png ../../src/ui/shaders
	cp -ru $^ .

clean:
	rm -f *.d *.o *.png papaya
	rm -rf shaders

