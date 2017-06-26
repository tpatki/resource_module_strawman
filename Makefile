TARGETS   := resource
CPP       := g++ #clang++
LDFLAGS   := -O0 -g
CPPFLAGS  := -O0 -g -Wall -std=c++11
INCLUDES  := -I/usr/include

MATCHERS  := CA \
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
SCALES    := mini #small medium medplus large largest
GRAPHS    := $(foreach m, $(MATCHERS), $(foreach s, $(SCALES), $(m).$(s)))

all: $(TARGETS)

graphs: $(GRAPHS) 

resource: resource.o resource_gen.o test_resource_spec.o
	$(CPP) $(LDFLAGS) $^ -o $@

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
	rm -f *.o *~ $(TARGETS)

clean-graphs: 
	rm -f -r graphs_dir/*

