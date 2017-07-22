# Flux Resource API Strawman 

Flux Resource API Strawman helps design flux resource comms. module,
which will be a service to select the best-matching resources for
each job. Some of the data structures and APIs will be factored into
the comms. module code base.

It contains a `resource` command which uses a resource generation
recipe file (see GRUG below), and print resource information at
certain visit events of graph walks. Its options allow for using
a different matcher that uses a different set of subsystems
 on which to walk with distinct walking policies.

It also allows you to export the filtered graph of the used matcher
in a selected graph format. If you want to try this yourself to get a hang of it:

```
$ make
$ make graphs
```

`$ resource --help` explains the available options.

We have minimal support for doxygen documentation. It can be generated: 

```
$ cd doxy
$ doxygen doxy_conf.txt
$ cd ..
```
This will generate html, latex and man sub-directories under
the doc directory. Open doc/html/index.html using your favorite web browser. NOTE for LLNL developers: It doesn't build on TOSS2 systems because their compilers are old. Please use a TOSS3 machine or your own laptop (e.g. Mac OSX)


## Generating Resources Using GraphML (GRUG)

### Overview
GRUG is a GraphML-based language for specifying a resource-graph generation recipe.
The resource service strawman can read in a GRUG file and populate its store
of the resource graph data conforming to Flux’s resource model
([RFC4](https://github.com/flux-framework/rfc/blob/master/spec_4.adoc)).
The goal of GRUG is to help Flux scheduler plug-in developers easily determine
the representation of this resource graph data (e.g., granularity of resource pools,
relationships between resources, and subsystems/hierarchies to use to organize the resources)
that are best suited for their scheduling objectives and algorithms.  
Without having to modify the source code of the resource service strawman,
developers can quickly test various resource graph representations by
only modifying the GRUG text file.

GraphML is an easy-to-use, XML-based graph specification language. GRUG uses
the vanilla GraphML schema (http://graphml.graphdrawing.org) with no extension,
and thereby familiarity with GraphML is the only prerequisite for fluent uses
of GRUG. We find that the following on-line GraphML materials are particularly
useful:

- [The GraphML File Format](http://graphml.graphdrawing.org)
- [GraphML Primer](http://graphml.graphdrawing.org/primer/graphml-primer.html)
- [Graph Markup Language](https://cs.brown.edu/~rt/gdhandbook/chapters/graphml.pdf)

### GRUG 
GRUG describes a resource-generation recipe as a graph. A vertex prescribes
how the corresponding resource pool (or simply resource as a shorthand) should be generated;
an edge prescribes how the corresponding relationships between two resources
should be generated. The edge properties also allow a small recipe graph to generate a large and more complex resource graph store.
A multiplicative edge has a scaling factor that will generate the specified number of copies of the resources of the target type. An associative edge
allows a source resource to be associated with some of the already generated resources
in a specific manner.

The resource service strawman walks this recipe graph using
the depth first search traversal and emits and stores the corresponding
resource and their relationship data into its resource graph store.  
The recipe graph must be a forest of trees whereby each tree represents
a distinct resource hierarchy or subsystem. We use a hierarchy and subsystem interchangeably below.

A conforming GRUG file is composed of two sections: 1) recipe graph
definition and 2) recipe attributes declaration. We explain both in the following sections.

### Recipe Graph Definition

A recipe graph definition is expressed GraphML’s as `graph` elements
consisting of two nested elements: `node` and `edge`. A `node` element
prescribes ways to generate a resource pool (RFC4); and an edge
for generating relationships. For example, given the following
definition,

```xml
<node id="socket">
     <data key="type">socket</data>
     <data key="basename">socket</data>
     <data key="size">1</data>
     <data key="subsystem">containment</data>
</node>

<node id="core">
    <data key="type">core</data>
    <data key="basename">core</data>
    <data key="size">1</data>
    <data key="subsystem">containment</data>
</node>
```
these `node` elements are the generation recipes
for a socket and core resource (i.e., scalar), respectively.
And they belong to the containment hierarchy.


```xml
<edge id="socket2core" source="socket" target="core">
    <data key="e_subsystem">containment</data>
    <data key="relation">contains</data>
    <data key="rrelation">in</data>
    <data key="gen_method">MULTIPLY</data>
    <data key="multi_scale">2</data>
</edge>
```

Here, this `edge` element is the generation recipe for
the relationship between the socket and core resources. 
It specifies that for each socket resource, 2 new
core resources (i.e., MULTIPLY and 2) will be generated,
and the relationship is `contains` and the reverse relationship
is `in`.

A resource in one subsystem (e.g., power hierarchy) can be
associated with another subsystem (e.g., containment hierarchy),
and associative edges are used for this purpose.

```xml
<node id="pdu_power">
    <data key="type">pdu</data>
    <data key="basename">pdu</data>
    <data key="subsystem">power</data>
</node>

<edge id="powerpanel2pdu" source="powerpanel" target="pdu_power">
    <data key="e_subsystem">power</data>
    <data key="relation">drawn</data>
    <data key="rrelation">flows</data>
    <data key="gen_method">ASSOCIATE_IN</data>
    <data key="as_tgt_subsystem">containment</data>
</edge>
```

Here, this `edge` element is the generation recipe for
the relationship between powerpanel and `pdu` resource.
It specifies that a `powerpanel` resource will be associated
(i.e., `ASSOCIATE_IN`), all of the `pdu` resources
that have already generated in the `containment` subsystem. 
The forward relationship is `drawn` and the reverse
relationship is `flows`.

Oftentimes, association with all resources of a type is not
sufficient to make fine-grained association. For the case where the hierarchical paths of 
associating resources can be used to make associations, `ASSOCIATE\_BY\_PATH\_IN` generation
method can be used.

```xml
<edge id="pdu2node" source="pdu_power" target="node_power">
    <data key="e_subsystem">power</data>
    <data key="relation">drawn</data>
    <data key="rrelation">flows</data>
    <data key="gen_method">ASSOCIATE_BY_PATH_IN</data>
    <data key="as_tgt_uplvl">1</data>
    <data key="as_src_uplvl">1</data>
</edge>
```

Here, the method is similar to the previous one except that
the association is only made with the `node` resources whose
hierarchical path at its parent level (i.e., `as_tgt_uplvl`=1)
is matched with the hierarchical path of the source resource
(also at the parent level, `as_src_uplvl`=1).

### Recipe Attributes Declaration 

This section appears right after the GraphML header and
before the recipe graph section.
To be a valid GRUG, this section must declare all attributes for both `node`
and `edge` elements. Currently, here are 16 attributes must be 
declared. 5 for the `node` element and 11 for the `edge`
elements. You are encouraged to define default values for
each of the attributes, which then can lead to more concise
recipe definitions. If a graph element is supposed to have the default
value for an attribute, having the default in the attribute will save you specifying it in the definition. These 16 attributes are the following:

```xml
<-- attributes for the recipe node elements -->
<key id="root" for="node" attr.name="root" attr.type="int">
<key id="type" for="node" attr.name="type" attr.type="string"/>
<key id="basename" for="node" attr.name="basename" attr.type="string"/>
<key id="size" for="node" attr.name="size" attr.type="long"/>
<key id="subsystem" for="node" attr.name="subsystem" attr.type="string"/>

<-- attributes for the recipe edge elements -->
<key id="e_subsystem" for="edge" attr.name="e_subsystem" attr.type="string"/>
<key id="relation" for="edge" attr.name="relation" attr.type="string"/>
<key id="rrelation" for="edge" attr.name="rrelation" attr.type="string"/>
<key id="id_scope" for="edge" attr.name="id_scope" attr.type="int"/>
<key id="id_start" for="edge" attr.name="id_start" attr.type="int"/>
<key id="id_stride" for="edge" attr.name="id_stride" attr.type="int"/>
<key id="gen_method" for="edge" attr.name="gen_method" attr.type="string"/>
<key id="multi_scale" for="edge" attr.name="multi_scale" attr.type="int"/>
<key id="as_tgt_subsystem" for="edge" attr.name="as_tgt_subsystem" attr.type="string">
<key id="as_tgt_uplvl" for="edge" attr.name="as_tgt_uplvl" attr.type="int"/>
<key id="as_src_uplvl" for="edge" attr.name="as_src_uplvl" attr.type="int"/>
```

Only a few attributes have not been explained. The `root` attribute specifies if a
resource is a root of a subsystem. If root, 1 must be assigned.

`id_scope`, `id_start` and `id_stride` specifies how the id field of a
resource will be generated. The integer specified with `id_scope`
defines the scope in which the id should be generated. 
The scope is local to its ancestor level defined by `id_scope`.
If `id_scope` is higher than the most distant ancestor, then
the id space becomes global. 

For example,
if `id_scope`=0, id of the generating resource will be local to its parent.
If `id_scope`=1, id is local to its grand parent
For example, in `rack[1]->node[18]->socket[2]->core[8]` configuration,
if `id_scope` is 1, the id space of a core resource is local to
the node level instead of the socket level.
So, 16 cores in each node will have 0-15, instead of repeating
0-7 and 0-7, which will be the case if the `id_scope` is 0.


### Example GRUG Files
Example GRUG files can be found in `conf/` directory. `medium-1subsystem-coarse.graphml` shows how one can model a resource graph in a highly coarse manner with no additional subsystem-based organization. `mini-5subsystems-fine.graphml` shows one way to model a fairly complex resource graph with five distinct subsystems to support matchers of various types.

 
### GRUG Visualizer
`genspec-graphml2dot` utility can be used to generate a GraphViz dot file that can render the recipe graph. The dot file can be converted into svg format by typing in `dot -Tsvg output.dot -o output.svg`:

```
Usage: genspec-graphml2dot <genspec>.graphml
    Convert a resource-graph generator spec (<genspec>.graphml)
    to AT&T GraphViz format (<genspec>.dot). The output
    file only contains the basic information unless --more is given.

    OPTIONS:
    -h, --help
            Display this usage information

    -m, --more
            More information in the output file

```

