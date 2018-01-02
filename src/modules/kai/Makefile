CFLAGS += -I../.. -Iebur128/queue

LDFLAGS += -L../../framework -lmlt -lm -lpthread

include ../../../config.mak

TARGET = ../libmltkai$(LIBSUF)

OBJS = factory.o \
	   filter_extract_alpha_channel.o

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d "$(DESTDIR)$(mltdatadir)/kai"
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/kai"

ifneq ($(wildcard .depend),)
include .depend
endif
