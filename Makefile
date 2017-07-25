CPP           := g++ #clang++
BOOST_LDFLAGS := -L/usr/local/Cellar/boost/1.64.0_1/lib \
	         -lboost_system -lboost_filesystem -lboost_graph
LDFLAGS       := -O0 -g $(BOOST_LDFLAGS)
CPPFLAGS      := -O0 -g -Wall -std=c++11 -MMD -MP
INCLUDES      := -I/usr/include
OBJS          := resource.o \
                 grug2dot.o \
                 resource_gen.o \
                 resource_gen_spec.o
MATCHERS      := CA \
                 IBA \
                 IBBA \
                 PFS1BA \
                 PA \
                 C+IBA \
                 C+PFS1BA \
                 C+PA \
                 IB+IBBA \
                 C+P+IBA \
                 ALL
DEPS          := $(OBJS:.o=.d)
SCALES        := mini-5subsystems-fine \
		 medium-5subsystems-fine 
GRAPHS        := $(foreach m, $(MATCHERS), $(foreach s, $(SCALES), $(m).$(s)))

TARGETS       := resource grug2dot

all: $(TARGETS)

graphs: $(GRAPHS) 

resource: resource.o resource_gen.o resource_gen_spec.o
	$(CPP) $^ -o $@ $(LDFLAGS)

grug2dot: grug2dot.o resource_gen_spec.o
	$(CPP) $^ -o $@ $(LDFLAGS)

$(GRAPHS): resource
	mkdir -p graphs_dir/$(subst .,$(empty),$(suffix $@))
	mkdir -p graphs_dir/$(subst .,$(empty),$(suffix $@))/images
	$< --grug=conf/$(subst .,$(empty),$(suffix $@)).graphml \
		--matcher=$(basename $@) \
		--output=graphs_dir/$(subst .,$(empty),$(suffix $@))/$@
	if [ -f graphs_dir/$(subst .,$(empty),$(suffix $@))/$@.dot ]; \
	then \
		cd graphs_dir/$(subst .,$(empty),$(suffix $@)) \
		&& dot -Tsvg $@.dot -o images/$@.svg; \
	fi
	mkdir -p graphmls_dir/$(subst .,$(empty),$(suffix $@))
	$< --grug=conf/$(subst .,$(empty),$(suffix $@)).graphml \
		--matcher=$(basename $@) \
		--graph-format=graphml \
		--output=graphmls_dir/$(subst .,$(empty),$(suffix $@))/$@

%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) $< -c -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) $(DEPS) $(TARGETS) *~ *.dot *.svg

clean-graphs: 
	rm -f -r graphs_dir/* graphmls_dir/*

-include $(DEPS)
