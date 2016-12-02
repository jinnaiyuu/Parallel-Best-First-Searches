# MARCH=$(shell uname -m)

# # These are all really the same thing here
# ifeq ($(MARCH), i686)
# MARCH=i386
# endif
# ifeq ($(MARCH), i586)
# MARCH=i386
# endif

# use the following to turn debugging on
#DEBUG=-g -O0

# use the following to turn debugging off
DEBUG=-DNDEBUG -O3

CFLAGS=$(DEBUG) -Wall -Werror -Iinclude

SRC=				\
	src/mem.c 		\
	src/queue.c		\
	src/prio_queue.c	\
	src/hashtbl.c	\
	src/ordlist_harris.c

# this is the deprecated ordered list
#	src/ordlist_valois.c

OBJDIR=obj-$(MARCH)
OBJS=$(subst src,$(OBJDIR), $(SRC:.c=.o))

OBJS+=$(OBJDIR)/atomic.o

all: $(OBJDIR) $(LIBLOCKFREE)		\
	atomic_test-$(MARCH)		\
	mem_test-$(MARCH)		\
	queue_test-$(MARCH)		\
	ordlist_test-$(MARCH)		\
	prio_queue_test-$(MARCH)

$(OBJDIR)/atomic.o: src/atomic_$(MARCH).c
	$(COMPILE.c) $^ -o $@

LIBLOCKFREE=liblockfree-$(MARCH).a
$(LIBLOCKFREE): $(OBJS)
	$(AR) crs $@ $^

prio_queue_test-$(MARCH): $(OBJDIR)/prio_queue_test.o $(LIBLOCKFREE)
	$(CC) $(CFLAGS) $^ -lpthread -o $@

ordlist_test-$(MARCH): $(OBJDIR)/ordlist_test.o $(LIBLOCKFREE)
	$(CC) $(CFLAGS) $^ -lpthread -o $@

queue_test-$(MARCH): $(OBJDIR)/queue_test.o $(LIBLOCKFREE)
	$(CC) $(CFLAGS) $^ -lpthread -o $@

atomic_test-$(MARCH): $(OBJDIR)/atomic_test.o $(LIBLOCKFREE)
	$(CC) $(CFLAGS) $^ -lpthread -o $@

mem_test-$(MARCH): $(OBJDIR)/mem_test.o $(LIBLOCKFREE)
	$(CC) $(CFLAGS) $^ -lpthread -o $@

$(OBJDIR):
	mkdir $@

$(OBJDIR)/%.o: src/%.c
	$(COMPILE.c) $^ -o $@

clean:
	rm -fr obj-$(MARCH)
	rm -f $(LIBLOCKFREE)
	rm -f atomic_test-$(MARCH)
	rm -f mem_test-$(MARCH)
	rm -f queue_test-$(MARCH)
	rm -f ordlist_test-$(MARCH)
	rm -f prio_queue_test-$(MARCH)
