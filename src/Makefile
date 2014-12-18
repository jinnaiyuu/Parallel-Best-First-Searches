MARCH=$(shell uname -m)

# These are all really the same thing here
ifeq ($(MARCH), i686)
MARCH=i386
endif
ifeq ($(MARCH), i586)
MARCH=i386
endif

LOCKFREEDIR=lockfree
LIBLOCKFREE=$(LOCKFREEDIR)/liblockfree-$(MARCH).a

OBJDIR=objs.$(MARCH)

SRC= \
	sync_wastar.cc \
	get_search.cc \
	state.cc \
	search_domain.cc \
	open_list.cc \
	lf_openlist.cc \
	queue_open_list.cc \
	pq_vector_open_list.cc \
	closed_list.cc \
	incons_list.cc \
	lf_closedlist.cc \
	synch_closed_list.cc \
	search.cc \
	astar.cc \
	optimistic.cc \
	awastar.cc \
	arastar.cc \
	multi_a_star.cc \
	breadth_first_search.cc \
	cost_bound_dfs.cc \
	ida_star.cc \
	heuristic.cc \
	h_zero.cc \
	weighted_heuristic.cc \
	projection.cc \
	div_merge_project.cc \
	util/eps.cc \
	util/thread.cc \
	util/completion_counter.cc \
	util/mutex.cc \
	util/timer.cc \
	util/cpu_timer.cc \
	util/timeout.cc \
	util/atomic_float.cc \
	util/cumulative_ave.cc \
	util/solution_stream.cc \
	util/sync_solution_stream.cc \
	util/serial_solution_stream.cc \
	util/fhist.cc \
	grid/grid_state.cc \
	grid/grid_world.cc \
	tiles/tiles_state.cc \
	tiles/tiles.cc \
	tiles/tiles_costs.cc \
	psdd/nblock.cc \
	psdd/nblock_graph.cc \
	psdd_search.cc \
	wbfpsdd/nblock.cc \
	wbfpsdd/nblock_graph.cc \
	wbfpsdd_search.cc \
	bfpsdd_search.cc \
	idpsdd_search.cc \
	dynamic_bounded_psdd.cc \
	wpbnf/nblock.cc \
	wpbnf/nblock_graph.cc \
	wpbnf_search.cc \
	arpbnf_search.cc \
	arpbnf/nblock.cc \
	arpbnf/nblock_graph.cc \
	opbnf_search.cc \
	opbnf/nblock.cc \
	opbnf/nblock_graph.cc \
	pbnf/nblock.cc \
	pbnf/nblock_graph.cc \
	pbnf_search.cc \
	kbfs.cc \
	pastar.cc \
	lpastar.cc \
	prastar.cc \
	prastar_vector.cc \
	wprastar.cc

OBJ=$(addprefix $(OBJDIR)/, $(SRC:.cc=.o))
OBJ+=$(LIBLOCKFREE)

ifeq ($(MARCH), sparc64)
FLAGS+=-mcpu=v9
OBJ+=$(OBJDIR)/util/sparc64_atomic_asm.o
OBJ+=$(OBJDIR)/util/sparc64_atomic_int.o
else ifeq ($(MARCH), sun4v)
OBJ+=$(OBJDIR)/util/sparc64_atomic_int.o
else ifeq ($(MARCH), i386)
OBJ+=$(OBJDIR)/util/i386_atomic_int.o
OBJ+=$(OBJDIR)/util/cmpxchg_i386.o
else ifeq ($(MARCH), x86_64)
OBJ+=$(OBJDIR)/util/x86_64_atomic_int.o
CXXFLAGS+=-D_64_BIT_LONG
else
$(error Unknown machine type)
endif

############################################################
# Compiler specific flags
ifeq ($(MARCH), sun4v) # Sun compilers
CC=cc
CXX=CC
#DEBUG=-g -xO0   # use the dbx debugger
DEBUG+=-DNDEBUG
FLAGS+=-mt -errwarn=%all $(Debug)
ASFLAGS+=-xc99=%none
CFLAGS+=-xc99=%none
LDFLAGS+=-lpthread -lmtmalloc
else                   # GNU compilers
PROFILE+=
#PROFILE+=-pg
DEBUG+=-O3 -DNDEBUG
#DEBUG+=-g -O0 -fno-default-inline -fno-implicit-inline-templates -fno-implement-inlines
FLAGS+=-Wall -Werror -pipe $(DEBUG) $(PROFILE)
CXXFLAGS+=-Wnon-virtual-dtor
LDFLAGS+=-lpthread $(PROFILE)
#LDFLAGS+=-lpthread $(PROFILE) -ltcmalloc
#LDFLAGS+=-L /home/aifs2/eaburns/lib -ljemalloc_mt -lpthread $(PROFILE)
endif
############################################################

#FLAGS+=-DENABLE_IMAGES
#FLAGS+=-DINSTRUMENTED
#FLAGS+=-DQUEUE_SIZES
#FLAGS+=-DTIME_QUEUES
#FLAGS+=-DCOUNT_FS
#FLAGS+=-DUNLAZY

CFLAGS+=$(FLAGS)
ASFLAGS+=$(FLAGS)
CXXFLAGS+=$(FLAGS)

# Other progs
# 	thread_tes
# 	atomic_int_test
# 	psdd_nblock_graph_test
# 	pbnf_nblock_graph_test
# 	tiles_hash_test
# 	tiles_read_test
# 	atomic_float_test
# 	pq_test
# 	one_tile_project_test
# 	two_tile_project_test
# 	two_tile_no_blank_project_test
PROGS=$(addsuffix .$(MARCH), \
	grid_search \
	tiles_search \
	grid_search_sync \
	tiles_search_sync \
)

all: objdirs $(PROGS)

grid_search.$(MARCH): $(OBJ) grid_search.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) grid_search.cc

tiles_search.$(MARCH): $(OBJ) tiles_search.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles_search.cc

grid_search_sync.$(MARCH): $(OBJ) grid_search_sync.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) grid_search_sync.cc

tiles_search_sync.$(MARCH): $(OBJ) tiles_search_sync.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles_search_sync.cc

thread_test.$(MARCH): $(OBJ) util/thread_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) util/thread_test.cc

atomic_int_test.$(MARCH): $(OBJ) util/atomic_int_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) util/atomic_int_test.cc

psdd_nblock_graph_test.$(MARCH): $(OBJ) psdd/nblock_graph_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) psdd/nblock_graph_test.cc

pbnf_nblock_graph_test.$(MARCH): $(OBJ) pbnf/nblock_graph_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) pbnf/nblock_graph_test.cc

tiles_hash_test.$(MARCH): $(OBJ) tiles/tiles_hash_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles/tiles_hash_test.cc

tiles_read_test.$(MARCH): $(OBJ) tiles/tiles_read_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles/tiles_read_test.cc

atomic_float_test.$(MARCH): $(OBJ) util/atomic_float_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) util/atomic_float_test.cc

div_merge_project_test.$(MARCH): $(OBJ) tiles/div_merge_project_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles/div_merge_project_test.cc

one_tile_project_test.$(MARCH): $(OBJ) tiles/one_tile_project_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles/one_tile_project_test.cc

two_tile_project_test.$(MARCH): $(OBJ) tiles/two_tile_project_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles/two_tile_project_test.cc

two_tile_no_blank_project_test.$(MARCH): $(OBJ) tiles/two_tile_no_blank_project_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) tiles/two_tile_no_blank_project_test.cc

pq_open_list_test.$(MARCH): $(OBJ) pq_open_list_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) pq_open_list_test.cc

pq_test.$(MARCH): $(OBJ) util/pq_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS) util/pq_test.cc

.PHONY: $(LIBLOCKFREE)
$(LIBLOCKFREE):
	cd $(LOCKFREEDIR); $(MAKE) MARCH=$(MARCH) ; cd ..

create_bins: grid_search.$(MARCH) tiles_search.$(MARCH) grid_search_sync.$(MARCH) tiles_search_sync.$(MARCH)
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo Is debugging turned off?
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo Is lockfree debugging turned off?
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	@echo
	cp grid_search.$(MARCH) grid_search.$(MARCH).bin
	cp tiles_search.$(MARCH) tiles_search.$(MARCH).bin
	cp grid_search_sync.$(MARCH) grid_search_sync.$(MARCH).bin
	cp tiles_search_sync.$(MARCH) tiles_search_sync.$(MARCH).bin

$(OBJDIR)/%.o: %.cc
	$(COMPILE.cc) $< -o $@

$(OBJDIR)/%.o: %.c
	$(COMPILE.c) $< -o $@

$(OBJDIR)/%.o: %.S
	$(COMPILE.S) $< -o $@

.PHONY: depend
depend:
	rm -f .depend
	touch .depend
	(find . -name \*.cc -printf "%f\n"; find . -name \*.h -printf "%f\n") | xargs makedepend -f.depend -Y -p$(OBJDIR)/
	rm -f .depend.bak

.PHONY: objdirs
objdirs:
	$(foreach file, $(SRC), $(shell test -d $(OBJDIR)/$(dir $(file)) || mkdir -p $(OBJDIR)/$(dir $(file))))

.PHONY: clean
 clean:
	rm -fr $(OBJDIR)
	rm -f $(PROGS)
	cd $(LOCKFREEDIR) ; $(MAKE) MARCH=$(MARCH) clean ; cd ..

-include .depend
