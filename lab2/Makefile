TARGET_EXEC ?= ls
SRCS := $(shell find . -name *.c)
OBJS := $(SRCS:%=%.o)
CFLAGS ?= -pedantic -Wall -std=c11 -g -D_DEFAULT_SOURCE -fsanitize=address,undefined -ggdb -fno-omit-frame-pointer
LDFLAGS ?= -fsanitize=address,undefined
.DEFAULT_GOAL := all

%.c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: clean all test

all: $(TARGET_EXEC)

clean:
	$(RM) $(TARGET_EXEC) $(OBJS)

test: $(TARGET_EXEC) 
	@exec ./test.bats
