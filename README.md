## Flux Resource API Strawman 

Flux Resource API Strawman helps design flux resource comms. module,
which will be a service to select the best-matching resources for
each job

Some of the data structures and APIs will be factored into
the comms. module code base.

It contains a `resource` command which builds a predefined
test resource graph containing five distinct subsystems
(a.k.a. hierarchies), and print resource information at
certain visit events of graph walks.

Its options allow for using a resource graph of varying sizes and
configurations, as well as a different matcher that uses a different
set of subsystems on which to walk with distinct walking policies.

It also allows you to export the filtered graph of the used matcher
in a selected graph format.

If you want to try this yourself to get a hang of it:

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
This will generate html, latex and man subdirectories under
the doc directory. Open doc/html/index.html using your favorate web browser 

