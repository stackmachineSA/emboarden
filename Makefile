LDLIBS = -lpng -lpcb
CXXFLAGS = -g

emboarden : emboarden.o svg.o image-io.o color.o
	$(LINK.cc) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean :
	$(RM) emboarden *.o *~
