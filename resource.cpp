//!
//! Resource Comms Module Strawman Implementation
//!

// TODO: Perf profilng for graph setup and walk i
//        -- target of 1 sec of the full tree walk for largest configuration
// TODO: matcher/traverser plugin architecture

#include <cstdint>
#include <map>
#include <getopt.h>
#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include "resource_graph.hpp"
#include "resource_gen.hpp"
#include "resource_base_dfu_traverse.hpp"

using namespace std;
using namespace boost;
using namespace flux_resource_model;

#define OPTIONS "G:m:d:g:o:h"
static const struct option longopts[] = {
    {"grug",             required_argument,  0, 'G'},
    {"matcher",          required_argument,  0, 'm'},
    {"display-matchers", required_argument,  0, 'd'},
    {"graph-format",     required_argument,  0, 'g'},
    {"output",           required_argument,  0, 'o'},
    {"help",             no_argument,        0, 'h'},
    { 0, 0, 0, 0 },
};

struct test_params_t {
    string gengraph;
    string matcher_name;
    string o_fname;
    string o_fext;
    resource_graph_format_t o_format;
};

// Use a context structure to make it easy to move this to our module environment
struct resource_context_t {
    test_params_t params;
    resource_graph_db_t db;
    multi_subsystems_t subsystems;
    map<string, f_resource_graph_t *> resource_graph_views;
    resource_base_dfu_matcher_t matcher;
    resource_base_dfu_traverser_t<resource_base_dfu_matcher_t> traverser;
};

static void usage (int code)
{
    cerr <<
"usage: resource [OPTIONS…]\n"
"\n"
"Resource service strawman to help design flux resource comms. module,\n"
"which will be a service to select the best-matching resources for\n"
"each job.\n"
"\n"
"Some of the data structures and APIs used in this strawman will be \n"
"factored into the comms. module.\n"
"\n"
"Build a test resource-graph store based on the resource-graph generation\n"
"recipe written in GRUG format and print the resource information at\n"
"various visit events of the resource-graph walk."
"\n"
"OPTIONS allow for using a different matcher that uses a different\n"
"set of subsystems on which to walk with distinct walking policies.\n"
"\n"
"OPTIONS allow for exporting the filtered graph of the used matcher\n"
"in a selected graph format.\n"
"\n"
"\n"
"OPTIONS:\n"
"    -h, --help\n"
"            Display the usage information\n"
"\n"
"    -G, --grug=<genspec>.graphml\n"
"            GRUG resource graph generator specification file in graphml\n"
"            (default=conf/default)\n"
"\n"
"    -m, --matcher="
         "<CA|IBA|IBBA|PFS1BA|PA|C+IBA|C+PFS1BA|C+PA|IB+IBBA|"
              "C+P+IBA|VA|V+PFS1BA|ALL>\n"
"            Set the test matcher to use. Available matchers are:\n"
"                CA: Containment Aware\n"
"                IBA: InfiniBand connection-Aware\n"
"                IBBA: InfiniBand Bandwidth-Aware\n"
"                PFS1BA: Parallel File System 1 Bandwidth-aware\n"
"                PA: Power-Aware\n"
"                C+IBA: Containment and InfiniBand connection-Aware\n"
"                C+PFS1BA: Containment and PFS1 Bandwidth-Aware\n"
"                C+PA: Containment and Power-Aware\n"
"                IB+IBBA: InfiniBand connection and Bandwidth-Aware\n"
"                C+P+IBA: Containment, Power and InfiniBand connection-Aware\n"
"                VA: Virtual Hierarchy-Aware \n"
"                V+PFS1BA: Virtual Hierarchy and PFS1 Bandwidth-Aware \n"
"                ALL: Aware of everything.\n"
"            (default=CA).\n"
"\n"
"    -g, --graph-format=<dot|graphml|cypher>\n"
"            Specify the graph format of the output file\n"
"            (default=dot)\n"
"\n"
"    -o, --output=<basename>\n"
"            Set the basename of the output file\n"
"            For AT&T Graphviz dot, <basename>.dot\n"
"            For GraphML, <basename>.graphml\n"
"\n";
    exit (code);
}

static void set_default_params (test_params_t &params)
{
    params.gengraph = "conf/default";
    params.matcher_name = "CA";
    params.o_fname = "";
    params.o_fext = "dot";
    params.o_format = GRAPHVIZ_DOT;
}

static int string_to_graph_format (string st, resource_graph_format_t &format)
{
    int rc = 0;
    if (iequals (st, string ("dot")))
        format = GRAPHVIZ_DOT;
    else if (iequals (st, string ("graphml")))
        format = GRAPH_ML;
    else if (iequals (st, string ("cypher")))
        format = NEO4J_CYPHER;
    else
        rc = -1;

    return rc;
}

static int graph_format_to_ext (resource_graph_format_t &format, string &st)
{
    int rc = 0;
    switch (format) {
    case GRAPHVIZ_DOT:
        st = "dot";
        break;
    case GRAPH_ML:
        st = "graphml";
        break;
    case NEO4J_CYPHER:
        st = "cypher";
        break;
    default:
        rc = -1;
    }

    return rc;
}

static int subsystem_exist (resource_context_t *ctx, string n)
{
    int rc = 0;
    if (ctx->db.roots.find (n) == ctx->db.roots.end ())
        rc = -1;
    return rc;
}

static int set_subsystems_use (resource_context_t *ctx, string n)
{
    int rc = 0;
    ctx->matcher.set_matcher_name (n);
    resource_base_dfu_matcher_t &matcher = ctx->matcher;
    const string &matcher_type = matcher.get_matcher_name ();

    if (iequals (matcher_type, string ("CA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
    }
    else if (iequals (matcher_type, string ("IBA"))) {
        if ( (rc = subsystem_exist (ctx, "ibnet")) == 0)
            matcher.add_subsystem ("ibnet", "*");
    }
    else if (iequals (matcher_type, string ("IBBA"))) {
        if ( (rc = subsystem_exist (ctx, "ibnetbw")) == 0)
            matcher.add_subsystem ("ibnetbw", "*");
    }
    else if (iequals (matcher_type, string ("PFS1BA"))) {
        if ( (rc = subsystem_exist (ctx, "pfs1bw")) == 0)
            matcher.add_subsystem ("pfs1bw", "*");
    }
    else if (iequals (matcher_type, string ("PA"))) {
        if ( (rc = subsystem_exist (ctx, "power")) == 0)
            matcher.add_subsystem ("power", "*");
    }
    else if (iequals (matcher_type, string ("C+PFS1BA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( !rc && (rc = subsystem_exist (ctx, "pfs1bw")) == 0)
            matcher.add_subsystem ("pfs1bw", "*");
    }
    else if (iequals (matcher_type, string ("C+IBA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( !rc && (rc = subsystem_exist (ctx, "ibnet")) == 0)
            matcher.add_subsystem ("ibnet", "connected_up");
    }
    else if (iequals (matcher_type, string ("C+PA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( !rc && (rc = subsystem_exist (ctx, "power")) == 0)
            matcher.add_subsystem ("power", "drawn");
    }
    else if (iequals (matcher_type, string ("IB+IBBA"))) {
        if ( (rc = subsystem_exist (ctx, "ibnet")) == 0)
            matcher.add_subsystem ("ibnet", "connected_down");
        if ( !rc && (rc = subsystem_exist (ctx, "ibnetbw")) == 0)
            matcher.add_subsystem ("ibnetbw", "*");
    }
    else if (iequals (matcher_type, string ("C+P+IBA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( (rc = subsystem_exist (ctx, "power")) == 0)
            matcher.add_subsystem ("power", "drawn");
        if ( !rc && (rc = subsystem_exist (ctx, "ibnet")) == 0)
            matcher.add_subsystem ("ibnet", "connected_up");
    } else if (iequals (matcher_type, string ("V+PFS1BA"))) {
        if ( (rc = subsystem_exist (ctx, "virtual1")) == 0)
            matcher.add_subsystem ("virtual1", "*");
        if ( !rc && (rc = subsystem_exist (ctx, "pfs1bw")) == 0)
            matcher.add_subsystem ("pfs1bw", "*");
    } else if (iequals (matcher_type, string ("VA"))) {
        if ( (rc = subsystem_exist (ctx, "virtual1")) == 0)
            matcher.add_subsystem ("virtual1", "*");
    } else if (iequals (matcher_type, string ("ALL"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "*");
        if ( !rc && (rc = subsystem_exist (ctx, "ibnet")) == 0)
            matcher.add_subsystem ("ibnet", "*");
        if ( !rc && (rc = subsystem_exist (ctx, "ibnetbw")) == 0)
            matcher.add_subsystem ("ibnetbw", "*");
        if ( !rc && (rc = subsystem_exist (ctx, "pfs1bw")) == 0)
            matcher.add_subsystem ("pfs1bw", "*");
        if ( (rc = subsystem_exist (ctx, "power")) == 0)
            matcher.add_subsystem ("power", "*");
    } else
        rc = -1;

    return rc;
}

static void write_to_graphviz (f_resource_graph_t &fg,
                single_subsystem_t ss, fstream &o)
{
    resource_name_map_t vmap = get(&resource_pool_t::name, fg);
    edg_subsystems_map_t emap = get(&resource_relation_t::member_of, fg);
    vtx_label_writer_t<resource_name_map_t> vwr (vmap);
    edg_label_writer_t ewr (emap, ss);
    write_graphviz (o, fg, vwr, ewr);
}

static void write_to_graphml (f_resource_graph_t &fg, fstream &o)
{
    dynamic_properties dp;
    dp.property ("name", get (&resource_pool_t::name, fg));
    dp.property ("name", get (&resource_relation_t::name, fg));
    write_graphml(o, fg, dp, true);
}

static void write_to_graph (resource_context_t *ctx)
{
    fstream o;
    string fn, mn;
    mn = ctx->matcher.get_matcher_name ();
    f_resource_graph_t &fg = *(ctx->resource_graph_views[mn]);
    fn = ctx->params.o_fname + "." + ctx->params.o_fext;
    o.open (fn, fstream::out);
    cout << "[INFO] Write the target graph of the matcher..." << endl;
    switch (ctx->params.o_format) {
    case GRAPHVIZ_DOT:
        write_to_graphviz (fg, ctx->matcher.get_dom_subsystem (), o);
        break;
    case GRAPH_ML:
        write_to_graphml (fg, o);
        break;
    case NEO4J_CYPHER:
    default:
        cout << "[ERROR] Graph format is not yet implemented:"
             << endl;
        break;
    }
    o.close ();
}

static double elapse_time (timeval &st, timeval &et)
{
    double ts1 = (double) st.tv_sec + (double) st.tv_usec/1000000.0f;
    double ts2 = (double) et.tv_sec + (double) et.tv_usec/1000000.0f;
    return ts2 - ts1;
}

int main (int argc, char *argv[])
{
    int rc;
    int ch;
    resource_context_t *ctx = new resource_context_t ();
    set_default_params (ctx->params);

    while ((ch = getopt_long (argc, argv, OPTIONS, longopts, NULL)) != -1) {
        switch (ch) {
            case 'h': /* --help */
                usage (0);
                break;
            case 'G': /* --grug*/
                ctx->params.gengraph = optarg;
                rc = 0;
                break;
            case 'm': /* --matcher */
                ctx->params.matcher_name = optarg;
                break;
            case 'g': /* --graph-format */
                rc = string_to_graph_format (optarg, ctx->params.o_format);
                if ( rc != 0) {
                    cerr << "[ERROR] unknown output format for --graph-format: ";
                    cerr << optarg << endl;
                    usage (1);
                }
                graph_format_to_ext (ctx->params.o_format, ctx->params.o_fext);
                break;
            case 'o': /* --output */
                ctx->params.o_fname = optarg;
                break;
            default:
                usage (1);
                break;
        }
    }

    if (optind != argc)
        usage (1);

    //
    // Generate a resource graph db
    //
    resource_generator_t rgen;
    if ( (rc = rgen.read_graphml (ctx->params.gengraph, ctx->db)) != 0) {
        cerr << "[ERROR] error in generating resources" << endl;
        cerr << "[ERROR] " << rgen.get_err_message () << endl;
        return EXIT_FAILURE;
    }
    resource_graph_t &g = ctx->db.resource_graph;

    //
    // Configure the matcher and its subsystem selector
    //
    cout << "[INFO] Load the matcher ..." << endl;
    if (set_subsystems_use (ctx, ctx->params.matcher_name) != 0) {
        cerr << "[ERROR] couldn't find all subsystems mapped to "
             << ctx->params.matcher_name << endl;
        return EXIT_FAILURE;
    }
    subsystem_selector_t<edg_t, edg_subsystems_map_t> edgsel (
        get (&resource_relation_t::member_of, g),
        ctx->matcher.get_subsystemsS ());
    subsystem_selector_t<vtx_t, vtx_subsystems_map_t> vtxsel (
        get (&resource_pool_t::member_of, g),
        ctx->matcher.get_subsystemsS ());
    f_resource_graph_t *fg = new f_resource_graph_t (g, edgsel, vtxsel);
    ctx->resource_graph_views[ctx->params.matcher_name] = fg;

    //
    // Traverse (only DFU now)
    //
    struct timeval st, et;
    gettimeofday (&st, NULL);
    ctx->traverser.begin_walk (*fg, ctx->db.roots, ctx->matcher);
    gettimeofday (&et, NULL);

    //
    // Elapse time
    //
    double elapse = elapse_time (st, et);
    cout << "*********************************************************" << endl;
    cout << "* Elapse time "   << to_string (elapse) << endl;
    cout << "*   Start Time: " << to_string (st.tv_sec)  << "."
                               << to_string (st.tv_usec) << endl;
    cout << "*   End Time: "   << to_string (et.tv_sec)  << "."
                               << to_string (et.tv_usec) << endl;
    cout << "*********************************************************" << endl;

    //
    // Output the filtered resource graph
    //
    if (ctx->params.o_fname != "")
        write_to_graph (ctx);

    return EXIT_SUCCESS;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
