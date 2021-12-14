CXX = clang++

DEPS := \
  Common.h \
  Cxbx.h \
  Error.h \
  Exe.h \
  Xbe.h

OBJS := \
  Common.obj \
  Error.obj \
  Exe.obj \
  OpenXDK.obj \
  Xbe.obj

DEBUG := y
ifeq ($(DEBUG),y)
CXXFLAGS += -Og -g3
endif

all: cdxt cexe cxbe

%.obj: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c -o '$@' '$<'

cdxt: Cdxt.obj $(OBJS)
	$(CXX) $(CXXFLAGS) -o '$@' $^

cexe: Cexe.obj $(OBJS)
	$(CXX) $(CXXFLAGS) -o '$@' $^

cxbe: Cxbe.obj $(OBJS)
	$(CXX) $(CXXFLAGS) -o '$@' $^

.PHONY: clean
clean:
	rm -f cdxt Cdxt.obj cexe Cexe.obj cxbe Cxbe.obj $(OBJS)
