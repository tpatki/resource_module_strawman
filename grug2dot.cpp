#include <iostream>
#include <getopt.h>
#include "resource_gen_spec.hpp"


using namespace std;
using namespace boost;
using namespace flux_resource_model;

#define OPTIONS "fh"
static const struct option longopts[] = {
    {"more",      no_argument,  0, 'm'},
    {"help",      no_argument,  0, 'h'},
    { 0, 0, 0, 0 },
};

void usage (int code)
{
    cerr <<
"Usage: grug2dot <genspec>.graphml\n"
"    Convert a resource-graph generator spec (<genspec>.graphml)\n"
"    to AT&T GraphViz format (<genspec>.dot). The output\n"
"    file only contains the basic information unless --more is given.\n"
"\n"
"    OPTIONS:\n"
"    -h, --help\n"
"            Display this usage information\n"
"\n"
"    -m, --more\n"
"            More information in the output file\n"
"\n";
    exit (code);
}

int main (int argc, char *argv[])
{
    int ch;
    int rc = 0;
    bool simple = true;
    while ((ch = getopt_long (argc, argv, OPTIONS, longopts, NULL)) != -1) {
        switch (ch) {
            case 'h': /* --help */
                usage (0);
                break;
            case 'm': /* --more */
                simple = false;
                break;
            default:
                usage (1);
                break;
        }
    }

    if (optind != (argc - 1))
        usage (1);

    resource_gen_spec_t gspec;
    string fn (argv[optind]);
    filesystem::path path = fn;
    string base = path.stem ().string ();

    if (gspec.read_graphml (fn) != 0) {
        cerr << "Error in reading " << fn << endl;
        rc = -1;
    } else if (gspec.write_graphviz (base + ".dot", simple) != 0) {
        cerr << "Error in writing " << base + ".dot" << endl;
        rc = -1;
    }

    return rc; 
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
