//!
//! Resource Generation API Strawman Implementation
//!

#include <boost/algorithm/string.hpp>
#include <vector>
#include <map>
#include <deque>
#include "resource_gen.hpp"

using namespace std;
using namespace boost;
using namespace flux_resource_model;

//
// Note that this class must be copy-constructible
// required by the concept of the depth first search
// visitor. It must be lightweight.
//
class dfs_emitter_t : public default_dfs_visitor {
public:
    dfs_emitter_t ();
    dfs_emitter_t (resource_graph_db_t *_db_p, resource_gen_spec_t *_gs);
    dfs_emitter_t (const dfs_emitter_t &o);
    dfs_emitter_t &operator=(const dfs_emitter_t &o);
    ~dfs_emitter_t();

    void tree_edge (gge_t e, const gg_t &recipe);
    void finish_vertex (ggv_t u, const gg_t &recipe);
    const string &get_err_message () const;

private:
    vtx_t emit_vertex (ggv_t u, gge_t e, const gg_t &recipe,
                       vtx_t src_v, int i, int sz, int j);
    edg_t raw_edge (vtx_t src_v, vtx_t tgt_v);
    void emit_edges (gge_t e, const gg_t &recipe,
                     vtx_t src_v, vtx_t tgt_v);
    int path_prefix (const std::string &pth,
                     int upl, std::string &pref);
    int gen_id (gge_t e, const gg_t &recipe,
                int i, int sz, int j);

    map<ggv_t, vector<vtx_t> > gen_src_vtx;
    deque<int> hier_scales;
    resource_graph_db_t *db_p = NULL;
    resource_gen_spec_t *gspec_p = NULL;
    string err_msg = "";
};


/*********************************************************************
 *             Private DFS Visitor Emitter API                       *
 *********************************************************************/

int dfs_emitter_t::path_prefix (const string &path, int uplevel, string &prefix)
{
    auto num_slashes = count (path.begin (), path.end (), '/');
    if (uplevel >= num_slashes)
      return -1;
    auto range = find_nth (path, "/", num_slashes - uplevel);
    string new_prefix (path.begin(), range.end());
    if (new_prefix.back () != '/')
        new_prefix.push_back ('/');
    prefix = std::move (new_prefix);
    return 0;
}

//
// This id is local to its ancestor level defined by "scope"
// scope=0: id is local to its parent
// scope=1: id is local to its grand parent
// For example, in rack[1]->node[18]->socket[2]->core[8] configuration,
// if scope is 1, the id space of a core resource is local to
// the node level instead of the socket level.
// So, 16 cores in each node will have 0-15, instead of repeating
// 0-7 and 0-7, which will be the case if the scope is 0.
//
int dfs_emitter_t::gen_id (gge_t e, const gg_t &recipe, int i, int sz, int j)
{
    int h = 0;
    int j_dim_wrap = 1;
    int scope = recipe[e].id_scope;

    if (scope < 0)
        return -1;
    else if (scope == 0)
        return recipe[e].id_start + i;

    if (scope > (int) hier_scales.size ())
        scope = hier_scales.size ();
    j_dim_wrap = 1;
    deque<int>::const_iterator iter;
    for (h = 0; h < scope; ++h)
        j_dim_wrap *= hier_scales[h];

    return recipe[e].id_start
              + (j % j_dim_wrap * sz * recipe[e].id_stride)
              + (i * recipe[e].id_stride);
}

edg_t dfs_emitter_t::raw_edge (vtx_t src_v, vtx_t tgt_v)
{
    edg_t e; // Unfortunately, BGL does not have null_edge ()
    bool inserted;
    out_edg_iterator ei, ee;
    resource_graph_db_t &db = *db_p;

    tie (ei, ee) = out_edges (src_v, db.resource_graph);
    for ( ; ei != ee; ++ei) {
        if (target (*ei, db.resource_graph) == tgt_v) {
            e = (*ei);
            return e;
        }
    }
    tie (e, inserted) = add_edge (src_v, tgt_v, db.resource_graph);
    if (!inserted) {
        err_msg += ": error inserting a new edge:"
                       + db.resource_graph[src_v].name
                       + " -> "
                       + db.resource_graph[tgt_v].name;
    }
    return e;
}

void dfs_emitter_t::emit_edges (gge_t ge, const gg_t &recipe,
                                vtx_t src_v, vtx_t tgt_v)
{
    resource_graph_db_t &db = *db_p;
    edg_t e = raw_edge (src_v, tgt_v);
    if (err_msg != "")
        return;
    db.resource_graph[e].member_of[recipe[ge].e_subsystem] = recipe[ge].relation;
    db.resource_graph[e].name += ":" + recipe[ge].e_subsystem
                                     + "." + recipe[ge].relation;
    e = raw_edge (tgt_v, src_v);
    if (err_msg != "")
        return;
    db.resource_graph[e].member_of[recipe[ge].e_subsystem] = recipe[ge].rrelation;
    db.resource_graph[e].name += ":" + recipe[ge].e_subsystem
                                     + "." + recipe[ge].rrelation;
}

vtx_t dfs_emitter_t::emit_vertex (ggv_t u, gge_t e, const gg_t &recipe,
                                  vtx_t src_v, int i, int sz, int j)
{
    resource_graph_db_t &db = *db_p;
    vtx_t v = add_vertex (db.resource_graph);
    string pref = "";
    string ssys = recipe[u].subsystem;
    int id = 0;

    if (src_v == graph_traits<resource_graph_t>::null_vertex()) {
        // ROOT!!
        db.roots[recipe[u].subsystem] = v;
        id = -1;
    } else {
        id = gen_id (e, recipe, i, sz, j);
        pref = db.resource_graph[src_v].paths[ssys];
    }

    string istr = (id != -1)? to_string (id) : "";
    db.resource_graph[v].type = recipe[u].type;
    db.resource_graph[v].basename = recipe[u].basename;
    db.resource_graph[v].size = recipe[u].size;
    db.resource_graph[v].id = id;
    db.resource_graph[v].name = recipe[u].basename + istr;
    db.resource_graph[v].paths[ssys] = pref + "/" + db.resource_graph[v].name;
    db.resource_graph[v].member_of[ssys] = "*";

    //
    // Indexing for fast look-up...
    //
    db.by_path[db.resource_graph[v].paths[ssys]].push_back (v);
    db.by_type[db.resource_graph[v].type].push_back (v);
    db.by_name[db.resource_graph[v].name].push_back (v);
    return v;
}



/*********************************************************************
 *             Public DFS Visitor Emitter                            *
 *********************************************************************/

dfs_emitter_t::dfs_emitter_t ()
{

}

dfs_emitter_t::dfs_emitter_t (resource_graph_db_t *_d, resource_gen_spec_t *_g)
{
    db_p = _d;
    gspec_p = _g;
}

dfs_emitter_t::dfs_emitter_t (const dfs_emitter_t &o)
{
    db_p = o.db_p;
    gspec_p = o.gspec_p;
    err_msg = o.err_msg;
}

dfs_emitter_t::~dfs_emitter_t()
{
    gen_src_vtx.clear ();
    hier_scales.clear ();
}

dfs_emitter_t &dfs_emitter_t::operator=(const dfs_emitter_t &o)
{
    db_p = o.db_p;
    gspec_p = o.gspec_p;
    err_msg = o.err_msg;
    return *this;
}

//!
//! Visitor method that is invoked on a tree-edge event
//! generated by depth_first_walk ()
//!
//! \param e      resource generator graph edge descriptor
//! \param recipe resource generator recipe graph
//!
void dfs_emitter_t::tree_edge (gge_t e, const gg_t &recipe)
{
    vtx_t src_vtx, tgt_vtx;
    ggv_t src_ggv = source (e, recipe);
    ggv_t tgt_ggv = target (e, recipe);
    vector<vtx_t>::iterator src_it, tgt_it;
    resource_graph_db_t &db = *db_p;
    string in;
    int i = 0, j = 0;;

    if (recipe[src_ggv].root) {
        //! ROOT
        vtx_t null_v = graph_traits<resource_graph_t>::null_vertex();
        gen_src_vtx[src_ggv].push_back (emit_vertex (src_ggv, e, recipe,
                                                     null_v, 0, 1, 0));
    }

    gen_src_vtx[tgt_ggv] = vector<vtx_t>();

    switch (gspec_p->to_gen_method_t (recipe[e].gen_method)) {
    case MULTIPLY:
        for (src_it = gen_src_vtx[src_ggv].begin ();
             src_it != gen_src_vtx[src_ggv].end (); src_it++, j++) {

            src_vtx = *src_it;
            for (i = 0; i < recipe[e].multi_scale; ++i) {
                tgt_vtx = emit_vertex (tgt_ggv, e, recipe, src_vtx, i,
                                       recipe[e].multi_scale, j);
                emit_edges (e, recipe, src_vtx, tgt_vtx);
                // TODO: Next gen src vertex; where do you clear them?
                gen_src_vtx[tgt_ggv].push_back (tgt_vtx);
            }
        }
        hier_scales.push_front (recipe[e].multi_scale);
        break;

    case ASSOCIATE_IN:
        for (src_it = gen_src_vtx[src_ggv].begin ();
             src_it != gen_src_vtx[src_ggv].end (); src_it++) {

            src_vtx = *src_it;
            for (tgt_it = db.by_type[recipe[tgt_ggv].type].begin();
                 tgt_it != db.by_type[recipe[tgt_ggv].type].end(); tgt_it++) {
                tgt_vtx = (*tgt_it);
                db.resource_graph[tgt_vtx].paths[recipe[e].e_subsystem]
                    = db.resource_graph[src_vtx].paths[recipe[e].e_subsystem]
                          + "/" + db.resource_graph[tgt_vtx].name;
                db.resource_graph[tgt_vtx].member_of[recipe[e].e_subsystem] = "*";
                emit_edges (e, recipe, src_vtx, tgt_vtx);
                gen_src_vtx[tgt_ggv].push_back (tgt_vtx);
            }
        }
        break;

    case ASSOCIATE_BY_PATH_IN:
        in = recipe[e].as_tgt_subsystem;
        for (src_it = gen_src_vtx[src_ggv].begin ();
             src_it != gen_src_vtx[src_ggv].end (); src_it++) {

            src_vtx = *src_it;
            for (tgt_it = db.by_type[recipe[tgt_ggv].type].begin();
                 tgt_it != db.by_type[recipe[tgt_ggv].type].end(); tgt_it++) {
                string comp_pth1, comp_pth2;
                tgt_vtx = (*tgt_it);
                path_prefix (db.resource_graph[tgt_vtx].paths[in],
                             recipe[e].as_tgt_uplvl, comp_pth1);
                path_prefix (db.resource_graph[src_vtx].paths[in],
                             recipe[e].as_src_uplvl, comp_pth2);

                if (comp_pth1 != comp_pth2)
                    continue;

                db.resource_graph[tgt_vtx].paths[recipe[e].e_subsystem]
                    = db.resource_graph[src_vtx].paths[recipe[e].e_subsystem]
                          + "/" + db.resource_graph[tgt_vtx].name;
                db.resource_graph[tgt_vtx].member_of[recipe[e].e_subsystem] = "*";
                emit_edges (e, recipe, src_vtx, tgt_vtx);
                gen_src_vtx[tgt_ggv].push_back (tgt_vtx);
            }
        }
        break;

    case GEN_UNKNOWN:
    default:
        err_msg += ": unknown generation method";
        break;
    }
}

//!
//! Visitor method that is invoked on a finish vertex by DFS visitor
//!
//! \param e      resource generator graph edge descriptor
//! \param recipe resource generator recipe graph
//!
void dfs_emitter_t::finish_vertex (ggv_t u, const gg_t &recipe)
{
    if (hier_scales.size())
        hier_scales.pop_front ();
}

//!
//! Return the error message. All error messages that
//! encountered have been concatenated.
//!
//! \return       error message
//!
const string &dfs_emitter_t::get_err_message () const
{
    return err_msg;
}


/*********************************************************************
 *             Public Resource Generator Interface                   *
 *********************************************************************/

resource_generator_t::resource_generator_t ()
{

}

resource_generator_t::~resource_generator_t ()
{

}

resource_generator_t::resource_generator_t (const resource_generator_t &o)
{
    gspec = o.gspec;
    err_msg = o.err_msg;
}

const resource_generator_t &resource_generator_t::operator=(
          const resource_generator_t &o)
{
    gspec = o.gspec;
    err_msg = o.err_msg;
    return *this;
}

//!
//! Return an error message string. All error messages that
//! encountered have been concatenated.
//!
//! \return       an error message string
//!
const std::string &resource_generator_t::get_err_message () const
{
    return err_msg;
}

//!
//! Read a subsystem spec graphml file and generate resource database
//!
//! \param sfile  generator spec file in graphml
//! \param db     graph database consisting of resource graph and various indices
//! \return       0 on success; non-zero integer on an error
//!
int resource_generator_t::read_graphml (const string &fn, resource_graph_db_t &db)
{
    int rc = 0;
    if (gspec.read_graphml (fn) != 0) {
        err_msg += ": error in reading " + fn;
        return -1;
    }

    //
    // depth_first_search on the generator recipe graph
    // with emitter visitor.
    //
    dfs_emitter_t emitter (&db, &gspec);
    depth_first_search (gspec.get_gen_graph (), visitor (emitter));
    err_msg += emitter.get_err_message ();

    return (err_msg == "")? rc : -1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
