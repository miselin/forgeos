
DIRS := .

ASMFILES := $(abspath $(shell find $(DIRS) -maxdepth 1 -type f -name "*.s"))
SRCFILES := $(abspath $(shell find $(DIRS) -maxdepth 1 -type f -name "*.c"))
HDRFILES := $(abspath $(shell find $(DIRS) -maxdepth 1 -type f -name "*.h"))
DEPFILES := $(abspath $(patsubst %.c,$(OBJDIR)/%.d,$(SRCFILES)))

test_queue: test_queue.c ../kernel/queue.c
	$(HOSTCXX) -D_UNIT_TESTING -I../kernel/include/shared -o $@ $^ -lgtest -lgtest_main -pthread

test_queue.asan: test_queue.c ../kernel/queue.c
	$(HOSTCXX) -D_UNIT_TESTING -I../kernel/include/shared -fsanitize=address -o $@ $^ -lgtest -lgtest_main -pthread

test_queue.tsan: test_queue.c ../kernel/queue.c
	$(HOSTCXX) -D_UNIT_TESTING -I../kernel/include/shared -fsanitize=thread -o $@ $^ -lgtest -lgtest_main -pthread

-include $(DEPFILES)
