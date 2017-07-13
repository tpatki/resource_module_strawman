CPP           := g++ #clang++
BOOST_LDFLAGS := -L/usr/local/Cellar/boost/1.64.0_1/lib \
	         -lboost_system -lboost_filesystem -lboost_graph
LDFLAGS       := -O0 -g $(BOOST_LDFLAGS)
CPPFLAGS      := -O0 -g -Wall -std=c++11 -MMD -MP
INCLUDES      := -I/usr/include
OBJS          := resource.o \
                 resource_gen.o \
                 test_resource_spec.o
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
                 VA \
                 V+PFS1BA \
                 ALL
DEPS          := $(OBJS:.o=.d)
SCALES        := mini #small medium medplus large largest
GRAPHS        := $(foreach m, $(MATCHERS), $(foreach s, $(SCALES), $(m).$(s)))

TARGETS       := resource 

all: $(TARGETS)

graphs: $(GRAPHS) 

resource: resource.o resource_gen.o test_resource_spec.o
	$(CPP) $^ -o $@ $(LDFLAGS)

$(GRAPHS): resource
	mkdir -p graphs_dir/$(subst .,$(empty),$(suffix $@))
	mkdir -p graphs_dir/$(subst .,$(empty),$(suffix $@))/images
	$< --graph-scale=$(subst .,$(empty),$(suffix $@)) \
		--matcher=$(basename $@) \
		--output=graphs_dir/$(subst .,$(empty),$(suffix $@))/$@
	cd graphs_dir/$(subst .,$(empty),$(suffix $@)) && \
	dot -Tsvg $@.dot -o images/$@.svg
	mkdir -p graphmls_dir/$(subst .,$(empty),$(suffix $@))
	$< --graph-scale=$(subst .,$(empty),$(suffix $@)) \
		--matcher=$(basename $@) \
		--graph-format=graphml \
		--output=graphmls_dir/$(subst .,$(empty),$(suffix $@))/$@

resource_gen: resource_gen.o
	$(CPP) $(LDFLAGS) $^ -o $@

test_resource_spec: test_resource_spec.o
	$(CPP) $(LDFLAGS) $^ -o $@

%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) $< -c -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) $(DEPS) $(TARGETS) *~ *.dot

clean-graphs: 
	rm -f -r graphs_dir/*

-include $(DEPS)
