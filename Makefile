CC = clang
CFLAGS = -Wall
OBJDIR = out
LIBDIR = lib
LIBNAME = libkraken
LIBTYPE = static

SRCS = $(wildcard *.c */*.c)
OB = $(notdir $(patsubst %.c,%.o,$(SRCS)))
OBJS = $(addprefix $(OBJDIR)/,$(OB))

INCLUDEPATH = -I./external/include
LIBPATH = -L./external/lib

ifdef SHARED
	LIBTYPE = shared
endif

ifeq ($(LIBTYPE),shared)
	LIBFILE = $(LIBDIR)/$(LIBNAME).so
	CFLAGS += -fPIC
	LDFLAGS = -shared
else
	LIBFILE = $(LIBDIR)/$(LIBNAME).a
endif

all: main $(LIBFILE)

debug:
	@echo 'SRCS: "$(SRCS)"'
	@echo 'OB: "$(OB)"'
	@echo 'OBJS: "$(OBJS)"'
	@echo 'LIBFILE: "$(LIBFILE)"'
	@echo 'CFLAGS: "$(CFLAGS)"'
	@echo 'LDFLAGS: "$(LDFLAGS)"'

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBPATH) -lowl

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDEPATH) -c -o $@ $<

$(OBJDIR)/%.o: */%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIBDIR):
	mkdir $(LIBDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

ifeq ($(LIBTYPE),shared)
$(LIBFILE): $(OBJS) | $(LIBDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ 
else
$(LIBFILE): $(OBJS) | $(LIBDIR)
	ar -rcs $@ $^
endif

.PHONY: clean all debug

clean:
	rm -rf *.o */*.o main $(LIBDIR)/*.a $(LIBDIR)/*.so $(OBJDIR) $(LIBDIR)