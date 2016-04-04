SRC =../../src
CFLAGS=-I$(SRC) -O0 -Wno-write-strings -Wno-format-security
LIBS=-ldl -lGL -lX11 -lXi `pkg-config --cflags --libs gtk+-2.0` -DUSE_GTK

papaya: $(SRC)/papaya_platform_linux.cpp papaya_core.o papaya_lib.o papaya_util.o ./imgui.o
	g++ -MMD -MF $@.d $^ $(LIBS) $(CFLAGS) -o $@

papaya_core.o: $(SRC)/papaya_core.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

papaya_lib.o: $(SRC)/papaya_lib.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

papaya_util.o: $(SRC)/papaya_util.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

imgui.o: $(SRC)/imgui.cpp
	g++ -MMD -MF $@.d $< $(LIBS) $(CFLAGS) -o $@ -c

clean:
	rm -f *.d *.o papaya


# depend: .depend
#
# .depend: $(SRCS)
# 	rm -f ./.depend
# 	$(CC) $(CFLAGS) -MM $^ -MF  ./.depend;
#
# include .depend
#
# papaya: 
# 	gcc -o papaya @^ $(CFLAGS) $(LIBS)
#
# obj/papaya_util.o: $(SRC)/papaya_util.c

# _DEPS = hellomake.h
# DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
#
# _OBJ = imgui.o hellofunc.o
# OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
#
#
# $(ODIR)/%.o: ../../src/%.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS)
#
# papaya: $(OBJ)
# 	gcc -o $@ $^ $(CFLAGS) $(LIBS)

# .PHONY: clean
#
# clean:
# 	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ ./.depend
