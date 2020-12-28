LDLIBS = -lpng -lpcb
CXXFLAGS = -g

emboarden : emboarden.o svg.o image-io.o color.o drl.o geometry.o opt.o
	$(LINK.cc) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean :
	$(RM) emboarden *.o *~
