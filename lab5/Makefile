BUILD := build

FUSE_CFLAGS := -DFUSE_USE_VERSION=26 $(shell pkg-config --cflags fuse 2>/dev/null)
FUSE_LDFLAGS := $(shell pkg-config --libs fuse 2>/dev/null)

EXTRA_CFLAGS ?=

CC	:= gcc
CFLAGS	:= -MD -O1 -g -c -Wall -std=c11 -D_DEFAULT_SOURCE $(FUSE_CFLAGS) $(EXTRA_CFLAGS)

FSDRIVER_OBJS	:=	bitmap.o \
			dir.o \
			disk_map.o \
			inode.o \
			panic.o \
			fsdriver.o
FSDRIVER_OBJS	:= $(patsubst %.o,$(BUILD)/%.o,$(FSDRIVER_OBJS))

all: $(BUILD)/fsdriver $(BUILD)/fsformat
	@:


$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/fsformat: $(BUILD)/fsformat.o
	$(CC) -o $@ $(BUILD)/fsformat.o

$(BUILD)/fsdriver: $(FSDRIVER_OBJS)
	$(CC) -o $@ $(FSDRIVER_OBJS) $(FUSE_LDFLAGS)

-include $(BUILD)/*.d

clean:
	rm -rf $(BUILD)

grade: always
	@echo $(MAKE) clean
	@$(MAKE) -s --no-print-directory clean
	./grade-lab

.PHONY: clean always
