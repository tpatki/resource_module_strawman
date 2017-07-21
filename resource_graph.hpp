//
// Resource Graph API
//
// TODO:do not use bundled properties!!
//!
//! Resource Graph API Strawman
//!

#include <boost/config.hpp>
#include <set>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graphml.hpp>
#include "resource_data.hpp"

#ifndef RESOURCE_GRAPH_HPP
#define RESOURCE_GRAPH_HPP

namespace flux_resource_model {

    using namespace boost;

    struct gname_t {
        std::string graph_name;
    };

    typedef adjacency_list<
        vecS,
        vecS,
        directedS,
        resource_pool_t,
        resource_relation_t,
        gname_t
    > resource_graph_t;

    typedef property_map<
        resource_graph_t,
        multi_subsystems_t resource_relation_t::*
    >::type edg_subsystems_map_t;

    typedef property_map<
        resource_graph_t,
        multi_subsystems_t resource_pool_t::*
    >::type vtx_subsystems_map_t;

    typedef graph_traits<resource_graph_t>::vertex_descriptor vtx_t;
    typedef graph_traits<resource_graph_t>::edge_descriptor edg_t;
    typedef graph_traits<resource_graph_t>::vertex_iterator vtx_iterator;
    typedef graph_traits<resource_graph_t>::edge_iterator edg_iterator;
    typedef graph_traits<resource_graph_t>::out_edge_iterator out_edg_iterator;

    struct resource_graph_db_t {
        resource_graph_t resource_graph;
        std::map<std::string, vtx_t> roots;
        std::map<std::string, std::vector <vtx_t> > by_type;
        std::map<std::string, std::vector <vtx_t> > by_name;
        std::map<std::string, std::vector <vtx_t> > by_path;
    };

    template <typename EV, typename SubsystemMap>
    class subsystem_selector_t {
    public:
        subsystem_selector_t () {}
        ~subsystem_selector_t () {}
        subsystem_selector_t (SubsystemMap s, multi_subsystemsS sel)
        {
            m_s = s;
            m_sel = sel;
        }
        bool operator () (const EV& ev) const {
            multi_subsystems_t subs = get(m_s, ev);
            multi_subsystems_t::const_iterator si;
            multi_subsystemsS::const_iterator i;
            for (si = subs.begin(); si != subs.end(); si++) {
                i = m_sel.find (si->first);
                if (i != m_sel.end ()) {
                    if (si->second == "*")
                        return true;
                    else if (i->second.find (si->second) != i->second.end ()
                        || i->second.find ("*") != i->second.end ()) {
                        //std::cout << si->second << std::endl;
                        return true;
                    }
                }
            }
            return false;
        }
    private:
        // Subsystems selector
        multi_subsystemsS m_sel;
        // TODO: Can't use a pointer because property map is from get()
        SubsystemMap m_s;
    };

    typedef boost::filtered_graph<
        resource_graph_t,
        subsystem_selector_t<edg_t, edg_subsystems_map_t>,
        subsystem_selector_t<vtx_t, vtx_subsystems_map_t>
    > f_resource_graph_t;

    typedef property_map<
        f_resource_graph_t,
        std::map<single_subsystem_t, default_color_type> resource_pool_t::*
    >::type resource_color_map_t;

    typedef property_map<
        f_resource_graph_t,
        std::string resource_pool_t::*
    >::type resource_name_map_t;

    typedef boost::graph_traits<
        f_resource_graph_t
    >::vertex_iterator f_vtx_iterator;

    typedef boost::graph_traits<
        f_resource_graph_t
    >::edge_iterator f_edg_iterator;

    // TODO self-sorting accumulator
    struct scored_vtx_t {
        vtx_t v;
        int score;
    };
    typedef std::set<scored_vtx_t> ordered_accumulator_t;
    typedef ordered_accumulator_t::iterator out_iterator;

    //
    // Graph Output API
    //
    enum resource_graph_format_t {
        GRAPHVIZ_DOT,
        GRAPH_ML,
        NEO4J_CYPHER
    };

    template<class vtx_map_t>
    class vtx_label_writer_t {
    public:
        vtx_label_writer_t (vtx_map_t &_m)
            : m (_m) { }
        void operator()(std::ostream &out, const vtx_t u) const {
            out << "[label=\"" << m[u] << "\"]";
        }
    private:
        vtx_map_t m;
    };

    class edg_label_writer_t {
    public:
        edg_label_writer_t (edg_subsystems_map_t &_props, single_subsystem_t &s)
            : m_props (_props), m_s (s) {}
        void operator()(std::ostream& out, const edg_t &e) const {
            multi_subsystems_t::const_iterator i = m_props[e].find (m_s);
            if (i != m_props[e].end ()) {
                out << "[label=\"" << i->second << "\"]";
            } else {
                i = m_props[e].begin ();
                out << "[label=\"" << i->second << "\"]";
            }
        }
    private:
        edg_subsystems_map_t m_props;
        single_subsystem_t m_s;
    };
}

#endif // RESOURCE_GRAPH_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
