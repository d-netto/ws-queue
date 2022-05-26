CC=gcc

CC_FLAGS=-std=c11 -Wall -Wno-attributes -O2
BINDIR=bin

CL_DEQUE_SRCS=src/array.c src/array_stack.c src/cl_deque.c # src/vn_cl_deque.c
CL_DEQUE_OBJS=$(CL_DEQUE_SRCS:%.c=$(BINDIR)/%.o)

TEST_SRCS=test/graph_bench.c
TEST_OBJS=$(TEST_SRCS:%.c=$(BINDIR)/%.o)

all: graph_bench

graph_bench: $(CL_DEQUE_OBJS) $(TEST_OBJS)
	@$(CC) $^ $(CC_FLAGS) -o graph_bench

$(BINDIR)/src/%.o: src/%.c src/%.h
	@mkdir -p $(@D)
	@$(CC) $< $(CC_FLAGS) -pthread -c -o $@

$(BINDIR)/test/%.o: test/%.c test/%.h
	@mkdir -p $(@D)
	@$(CC) $< $(CC_FLAGS) -pthread -c -o $@

clean:
	@rm -rf bin && rm -f graph_bench