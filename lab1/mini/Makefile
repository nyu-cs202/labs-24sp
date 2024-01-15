BUILD  := build
ifneq ($(shell arch), aarch64) 
    STATIC := static-x86
else
    STATIC := static-arm64
endif

SOURCES := part1.c part2.c part3.c 

SOBJS :=	$(STATIC)/part1_harness.o \
		$(STATIC)/part2_harness.o \
		$(STATIC)/part3_harness.o 

CC     := gcc
CFLAGS := -Werror -std=gnu99 -g

OBJS :=	$(BUILD)/part1 \
	$(BUILD)/part2 \
	$(BUILD)/part3 \


all: $(OBJS)
	@:

real_all: $(OBJS)

$(BUILD)/part%: part%.c $(SOBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -g -c part$*.c -o $(BUILD)/part$*.o
	$(CC) $(CFLAGS) $(STATIC)/part$*_harness.o $(BUILD)/part$*.o -lm -o $(BUILD)/part$*

clean-logs: always
	rm -f *.out

clean: always clean-logs
	rm -rf $(BUILD)

grade:
	@echo $(MAKE) clean
	@$(MAKE) -s --no-print-directory clean
	@./grade-lab

TARBALL  := lab1.tar.gz

# Most of the next two rules ripped from MIT JOS.
handin-from-git:
	@if ! git diff-files --quiet || ! git diff-index --quiet --cached HEAD; then \
		git status; \
		echo; \
		echo "You have uncomitted changes.  Please commit or stash them."; \
		false; \
	fi
	@if test -n "`git ls-files -o --exclude-standard`"; then \
		git status; \
		read -p "Untracked files will not be handed in.  Continue? [y/N] " r; \
		test "$$r" = y; \
	fi
	tar -cf - `git ls-files` | gzip > $(TARBALL)

handin: $(SOURCES)
	tar czvf $(TARBALL) $(SOURCES)

.PHONY: all always
