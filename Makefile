CXX = clang++

BUILD_DIR := build
BIN_DIR := bin

DEBUG := n
ifeq ($(DEBUG),y)
CXXFLAGS += -Og -g3
endif


DEPS := \
  Common.h \
  Cxbx.h \
  Error.h \
  Exe.h \
  Xbe.h

OBJS := \
  $(BUILD_DIR)/Common.obj \
  $(BUILD_DIR)/Error.obj \
  $(BUILD_DIR)/Exe.obj \
  $(BUILD_DIR)/OpenXDK.obj \
  $(BUILD_DIR)/Xbe.obj


all: $(BIN_DIR)/cdxt $(BIN_DIR)/cexe $(BIN_DIR)/cxbe $(BIN_DIR)/readxbe

$(BUILD_DIR)/%.obj: %.cpp $(DEPS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o '$@' '$<'

$(BIN_DIR)/cdxt: $(BUILD_DIR)/Cdxt.obj $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o '$@' $^

$(BIN_DIR)/cexe: $(BUILD_DIR)/Cexe.obj $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o '$@' $^

$(BIN_DIR)/cxbe: $(BUILD_DIR)/Cxbe.obj $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o '$@' $^

$(BIN_DIR)/readxbe: $(BUILD_DIR)/ReadXBE.obj $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o '$@' $^

.PHONY: clean
clean:
	rm -f \
		cdxt $(BUILD_DIR)/Cdxt.obj \
		cexe $(BUILD_DIR)/Cexe.obj \
		cxbe $(BUILD_DIR)/Cxbe.obj \
		readxbe $(BUILD_DIR)/ReadXBE.obj \
		$(OBJS)
