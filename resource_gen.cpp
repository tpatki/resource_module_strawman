//!
//! Resource Generation API Strawman Implementation
//!

#include "resource_gen.hpp"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using namespace flux_resource_model;

int resource_generator_t::path_prefix (const string &path,
                               int uplevel, string &prefix)
{
    vector<string> tokens;
    split(tokens, path, is_any_of ("/"), token_compress_on);
    if (uplevel >= tokens.size () - 1)
        return -1;
    int i = 0;
    prefix = "/";
    for (i = 1; i < tokens.size () - uplevel; i++)
        prefix += tokens[i] + "/";
    prefix[prefix.size ()] = '\0';
    
    return 0;
    
}

int resource_generator_t::gen_id (const resource_graph_t &g, id_meth_t im,
                            const vtx_t &p, const vtx_t &v, int i)
{
    int id;
    switch (im) {
    case ID_GEN_USE_PARENT_INFO:
        id = (g[p].id * g[v].count + i) * g[v].stride;
        break;
    case ID_GEN_USE_MY_INFO:
        id = i * g[v].stride;
        break;
    case ID_GEN_USE_NONE:
    default:
        id = -1;
        break;
    }
    
    return id;
}
    
vtx_t resource_generator_t::gen_new (const vtx_t &p,
          const sspec_t &s, int i, resource_graph_db_t &db)
{
    vtx_t v;
    edg_t e;
    bool inserted;
    
    v = add_vertex (db.resource_graph);
    db.resource_graph[v].type = s.type;
    db.resource_graph[v].basename = s.basename;
    db.resource_graph[v].count = s.count;
    db.resource_graph[v].size = s.size;
    db.resource_graph[v].stride = s.stride;
    db.resource_graph[v].id
        = gen_id (db.resource_graph, s.gen_info.imeth, p, v, i);
    db.resource_graph[v].member_of[s.ssys] = "*";
    if (s.gen_info.p2me_type != "") {
        tie (e, inserted) = add_edge (p, v, db.resource_graph);
        db.resource_graph[e].member_of[s.ssys] = s.gen_info.p2me_type;
    }
    if (s.gen_info.me2p_type != "") {
        tie (e, inserted) = add_edge (v, p, db.resource_graph);
        db.resource_graph[e].member_of[s.ssys] = s.gen_info.me2p_type;
    }
    string idstr = (db.resource_graph[v].id != -1)
                    ? to_string (db.resource_graph[v].id)
                    : "";
    db.resource_graph[v].name = db.resource_graph[v].basename + idstr;
    db.resource_graph[v].paths[s.ssys]
        = db.resource_graph[p].paths[s.ssys]
          + "/" + db.resource_graph[v].name;
    db.by_type[db.resource_graph[v].type].push_back (v);
    db.by_name[db.resource_graph[v].name].push_back (v);
    db.by_path[db.resource_graph[v].paths[s.ssys]].push_back (v);
    
    return v;
}
    
int resource_generator_t::gen_children (const vtx_t &p,
        const std::vector<sspec_t *> &c, resource_graph_db_t &db)
{
    int rc = 0;
    vtx_t v;
    edg_t e;
    bool inserted;
    vector<sspec_t *>::const_iterator iter;
    for (iter = c.begin (); iter != c.end (); iter++) {
        int i = 0;
        vector<vtx_t>::const_iterator ti;
        const sspec_t &cs = *(*iter);
        
        switch (cs.gen_info.rmeth) {
            case NEW:
                for (i = 0; i < cs.count; ++i) {
                    v = gen_new (p, cs, i, db);
                    rc += gen_children (v, cs.children, db);
                }
                break;
            case ASSOCIATE_IN:
                for (ti = db.by_type[cs.type].begin();
                     ti != db.by_type[cs.type].end(); ti++) {
                    v = (*ti);
                    db.resource_graph[v].paths[cs.ssys]
                        = db.resource_graph[p].paths[cs.ssys]
                          + "/" + db.resource_graph[v].name;
                    db.resource_graph[v].member_of[cs.ssys] = "*";
                    if (cs.gen_info.p2me_type != "") {
                        tie (e, inserted) = add_edge (p, v, db.resource_graph);
                        db.resource_graph[e].member_of[cs.ssys]
                            = cs.gen_info.p2me_type;
                    }
                    if (cs.gen_info.me2p_type != "") {
                        tie (e, inserted) = add_edge (v, p, db.resource_graph);
                        db.resource_graph[e].member_of[cs.ssys]
                            = cs.gen_info.me2p_type;
                    }
                    rc += gen_children (v, cs.children, db);
                }
                break;
            case ASSOCIATE_BY_PATH_IN:
                for (ti = db.by_type[cs.type].begin();
                     ti != db.by_type[cs.type].end(); ti++) {
                    v = (*ti);
                    const string &in = cs.gen_info.in_subsystem;
                    string &mpt = db.resource_graph[v].paths[in];
                    string mpr;
                    path_prefix (mpt, cs.gen_info.uplevel_me, mpr);
                    const string &ppt = db.resource_graph[p].paths[in];
                    string ppr;
                    path_prefix (ppt, cs.gen_info.uplevel_parent, ppr);
                    if (mpr != ppr)
                        continue;
                    db.resource_graph[v].paths[cs.ssys]
                        = db.resource_graph[p].paths[cs.ssys];
                          + "/" + db.resource_graph[v].name;
                    db.resource_graph[v].member_of[cs.ssys] = "*";
                    if (cs.gen_info.p2me_type != "") {
                        tie (e, inserted) = add_edge (p, v, db.resource_graph);
                        db.resource_graph[e].member_of[cs.ssys]
                            = cs.gen_info.p2me_type;
                    }
                    if (cs.gen_info.me2p_type != "") {
                        tie (e, inserted) = add_edge (v, p, db.resource_graph);
                        db.resource_graph[e].member_of[cs.ssys]
                            = cs.gen_info.me2p_type;
                    }
                    rc += gen_children (v, cs.children, db);
                }
                break;
            default:
                err_msg = "Unknown resource generation method";
                rc = 1;
                break;
        }
    }
    
    return rc;
}

int resource_generator_t::gen_subsystem (const sspec_t &root,
        resource_graph_db_t &db)
{
    vtx_t v = add_vertex (db.resource_graph);
    db.resource_graph[v].type = root.type;
    db.resource_graph[v].basename = root.basename;
    db.resource_graph[v].count = root.count;
    db.resource_graph[v].size = root.size;
    db.resource_graph[v].stride = root.stride;
    db.resource_graph[v].id = 0; 
    db.resource_graph[v].name = root.basename;
    db.resource_graph[v].paths[root.ssys] = "/" + db.resource_graph[v].name;
    db.resource_graph[v].member_of[root.ssys] = "*";
    db.by_path[db.resource_graph[v].paths[root.ssys]].push_back (v);
    db.by_type[db.resource_graph[v].type].push_back (v);
    db.by_name[db.resource_graph[v].name].push_back (v);
    db.by_path[db.resource_graph[v].paths[root.ssys]].push_back (v);
    db.roots[root.ssys] = v;
    
    return gen_children (v, root.children, db);
}

/*********************************************************************
 *             Public Resource Generator Interface                   *
 *********************************************************************/
               
resource_generator_t::resource_generator_t ()
{
    err_msg = "";
}

resource_generator_t::~resource_generator_t ()
{
}

const std::string &resource_generator_t::get_err_message ()
{
    return err_msg;
}

//!
//! Read a subsystem spec vector and generate resource database
//!
//! \param sspecs vector of subsystem specs
//! \param db     graph database consisting of resource graph and various indices
//! \return       0 on success; non-zero integer on an error
//!
int resource_generator_t::read_sspecs (const std::vector<sspec_t *> sspecs,
        resource_graph_db_t &db)
{
    int rc = 0;
    std::vector<sspec_t *>::const_iterator iter;
    for (iter = sspecs.begin (); iter != sspecs.end (); iter++)
        if ( (rc = gen_subsystem (*(*iter), db)) != 0)
            break;
    
    return rc;
}

int resource_generator_t::read_sspecs (const std::string &sfile,
        resource_graph_db_t &db)
{
    // NYI
    return -1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
