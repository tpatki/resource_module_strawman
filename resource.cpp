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
#include "resource_spec.hpp"
#include "resource_graph.hpp"
#include "resource_gen.hpp"
#include "resource_base_dfu_traverse.hpp"

using namespace std;
using namespace boost;
using namespace flux_resource_model;

#define OPTIONS "s:m:l:d:g:o:h"
static const struct option longopts[] = {
    {"graph-scale",      required_argument,  0, 's'},
    {"matcher",          required_argument,  0, 'm'},
    {"list-subsystems",  required_argument,  0, 'l'},
    {"display-matchers", required_argument,  0, 'd'},
    {"graph-format",     required_argument,  0, 'g'},
    {"output",           required_argument,  0, 'o'},
    {"help",             no_argument,        0, 'h'},
    { 0, 0, 0, 0 },
};

struct test_params_t {
    t_scale_t scale;
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
"usage: resource-proto [OPTIONS…]\n"
"\n"
"Resource prototype v1.0 to help design flux resource comms. module,\n"
"which will be a service to select the best-matching resources for\n"
"each job.\n"
"\n"
"Some of the data structures and APIs will be factored into\n"
"the comms. module.\n"
"\n"
"Build a predefined test resource graph containing five distinct\n"
"subsystems (a.k.a. hierarchies), and print resource information at\n"
"certain visit events of graph walks."
"\n"
"OPTIONS allow for using a resource graph of varying sizes and\n"
"configurations, as well as a different matcher that uses a different\n"
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
"    -s, --graph-scale=<mini|small|medium|medplus|large|largest>\n"
"            Set the scale of the test resource graph\n"
"            (default=mini)\n"
"\n"
"    -m, --matcher="
         "<CA|IBA|IBBA|PFS1BA|PA|C+IBA|C+PFS1BA|C+PA|IB+IBBA|C+P+IBA|ALL>\n"
"            Set the matcher to use. Available matchers are:\n"
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
"                ALL: Aware of everything. \n"
"            (default=CA).\n"
"\n"
"    -l, --list-subsystems\n"
"            List all available subsystems (a.k.a. hierarchies)\n"
"            in the resource graph\n"
"\n"
"    -g, --graph-format=<dot|graphml|cypher>\n"
"            Specify the graph format of the output file\n"
"            (default=dot)\n"
"\n"
"    -o, --output=<basename>\n"
"            Set the basename of the output file\n"
"            For AT&T Graphviz dot, <basename>.dot\n"
"            For GraphML, <basename>.graphml\n"
"            For Neo4j, <basename>.cypher\n"
"\n";
    exit (code);
}

static void set_default_params (test_params_t &params)
{
    params.scale = TS_MINI;
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
    if (ctx->subsystems.find ("containment")
        == ctx->subsystems.end ())    
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
            matcher.add_subsystem ("pfs1bw", "flows_up");
    }
    else if (iequals (matcher_type, string ("PA"))) {
        if ( (rc = subsystem_exist (ctx, "power")) == 0)
            matcher.add_subsystem ("power", "*");
    }
    else if (iequals (matcher_type, string ("C+PFS1BA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( !rc && (rc = subsystem_exist (ctx, "PFS1BA")) == 0)
            matcher.add_subsystem ("pfs1bw", "flows_up");
    }
    else if (iequals (matcher_type, string ("C+IBA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( !rc && (rc = subsystem_exist (ctx, "PFS1BA")) == 0)
            matcher.add_subsystem ("ibnet", "connected_up");
    }
    else if (iequals (matcher_type, string ("C+PA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( !rc && (rc = subsystem_exist (ctx, "PA")) == 0)
            matcher.add_subsystem ("power", "drawn");
    }
    else if (iequals (matcher_type, string ("IB+IBBA"))) {
        if ( (rc = subsystem_exist (ctx, "ibnet")) == 0)
            matcher.add_subsystem ("ibnet", "connected_down");
        if ( !rc && (rc = subsystem_exist (ctx, "IBBA")) == 0)
            matcher.add_subsystem ("ibnetbw", "*");
    }
    else if (iequals (matcher_type, string ("C+P+IBA"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "contains");
        if ( (rc = subsystem_exist (ctx, "power")) == 0)
            matcher.add_subsystem ("power", "drawn");
        if ( !rc && (rc = subsystem_exist (ctx, "IBA")) == 0)
            matcher.add_subsystem ("ibnet", "connected_up");
    } else if (iequals (matcher_type, string ("ALL"))) {
        if ( (rc = subsystem_exist (ctx, "containment")) == 0)
            matcher.add_subsystem ("containment", "*");
        if ( !rc && (rc = subsystem_exist (ctx, "IBA")) == 0)
            matcher.add_subsystem ("ibnet", "*");
        if ( !rc && (rc = subsystem_exist (ctx, "IBBA")) == 0)
            matcher.add_subsystem ("ibnetbw", "*");
        if ( !rc && (rc = subsystem_exist (ctx, "PFS1BW")) == 0)
            matcher.add_subsystem ("pfs1bw", "*");
        if ( (rc = subsystem_exist (ctx, "power")) == 0)
            matcher.add_subsystem ("power", "*");
    }
    else
        rc = -1;
    
    return rc;
}

static void write_to_graph (resource_context_t *ctx)
{
    if (ctx->params.o_format != GRAPHVIZ_DOT) {
        cout << "[ERROR] Graph format is not yet implemented:" 
             << endl;
        return;
    }

    fstream o;
    string fn, mn;
    mn = ctx->matcher.get_matcher_name ();
    f_resource_graph_t &fg = *(ctx->resource_graph_views[mn]);
    fn = ctx->params.o_fname + "." + ctx->params.o_fext;
    o.open (fn, fstream::out);

    cout << "[INFO] Write the target graph of the matcher..." << endl;
    edg_subsystems_map_t emap = get(&resource_relation_t::member_of, fg);
    edge_label_writer_t ewr (emap);
    write_graphviz (o, fg,
        make_label_writer (get(&resource_pool_t::name, fg)), ewr);
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
            case 's': /* --graph-scale */
                rc = test_spec_string_to_scale (optarg, ctx->params.scale);
                if ( rc != 0) {
                    cerr << "[ERROR] unknown scale for --graph-scale: ";
                    cerr << optarg << endl;
                    usage (1);
                }
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
    // Build a test resource specification
    //
    vector <sspec_t *> spec_vect;
    test_spec_build (ctx->params.scale, spec_vect);
    vector <sspec_t *>::iterator iter;
    for (iter = spec_vect.begin (); iter != spec_vect.end (); iter++)
        ctx->subsystems[(*iter)->ssys] = "";
    
    //
    // Generate a resource graph db
    //
    resource_generator_t r_gen;
    if ( (rc = r_gen.read_sspecs (spec_vect, ctx->db)) != 0) {
        cerr << "[ERROR] error in generating resources" << endl;
        cerr << "[ERROR] " << r_gen.get_err_message () << endl;
        return EXIT_FAILURE;
    }
    resource_graph_t &g = ctx->db.resource_graph;

    //
    // Configure the matcher and its subsystem selector
    //
    cout << "[INFO] Load the matcher ..." << endl;
    set_subsystems_use (ctx, ctx->params.matcher_name);
    subsystem_selector_t<edg_t, edg_subsystems_map_t>
        edgsel (get (&resource_relation_t::member_of, g),
                ctx->matcher.get_subsystemsS ());
    subsystem_selector_t<vtx_t, vtx_subsystems_map_t>
        vtxsel (get (&resource_pool_t::member_of, g),
                ctx->matcher.get_subsystemsS ());
    f_resource_graph_t *fg = new f_resource_graph_t (g, edgsel, vtxsel);
    ctx->resource_graph_views[ctx->params.matcher_name] = fg;
    
    //
    // Traverse 
    //
    struct timeval st, et;
    gettimeofday (&st, NULL);
    ctx->traverser.begin_walk (*fg, ctx->db.roots, ctx->matcher);
    gettimeofday (&et, NULL);
    
    //
    // Walk elapse time 
    //
    cout << "*********************************************************" << endl;
    cout << "* Elapse time "   << to_string (elapse_time (st, et)) << endl;
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
