#include <string>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/filesystem.hpp>

#ifndef RESOURCE_GEN_SPEC_HPP 
#define RESOURCE_GEN_SPEC_HPP 1

namespace flux_resource_model {

    using namespace boost;

    enum gen_meth_t {
        MULTIPLY,
        ASSOCIATE_IN,
        ASSOCIATE_BY_PATH_IN,
        GEN_UNKNOWN
    };
    
    struct resource_pool_gen_t {
        int root;
        std::string type;
        std::string basename;
        long size;
        std::string subsystem;
    };
    
    struct relation_gen_t {
        std::string e_subsystem;
        std::string relation;
        std::string rrelation;
        int id_scope;
        int id_start;
        int id_stride;
        std::string gen_method;
        int multi_scale;
        std::string as_tgt_subsystem;
        int as_tgt_uplvl;
        int as_src_uplvl;
    };
    
    typedef adjacency_list<
        vecS,
        vecS,
        directedS,
        resource_pool_gen_t,
        relation_gen_t
    > gg_t;
    
    typedef graph_traits<gg_t>::vertex_descriptor ggv_t;
    typedef graph_traits<gg_t>::edge_descriptor gge_t;
    typedef property_map<gg_t, std::string resource_pool_gen_t::* >::type
                vtx_type_map_t;
    typedef property_map<gg_t, std::string resource_pool_gen_t::* >::type
                vtx_basename_map_t;
    typedef property_map<gg_t, long resource_pool_gen_t::* >::type
                vtx_size_map_t;
    typedef property_map<gg_t, std::string resource_pool_gen_t::* >::type
                vtx_subsystem_map_t;
    typedef property_map<gg_t, std::string relation_gen_t::* >::type
                edg_e_subsystem_map_t;
    typedef property_map<gg_t, std::string relation_gen_t::* >::type
                edg_relation_map_t;
    typedef property_map<gg_t, std::string relation_gen_t::* >::type
                edg_rrelation_map_t;
    typedef property_map<gg_t, std::string relation_gen_t::* >::type
                edg_gen_method_map_t;
    typedef property_map<gg_t, std::string relation_gen_t::* >::type
                edg_id_method_map_t;
    typedef property_map<gg_t, int relation_gen_t::* >::type
                edg_multi_scale_map_t;
    
    class resource_gen_spec_t {
    public:
        resource_gen_spec_t ();
        resource_gen_spec_t (const resource_gen_spec_t &o);
        const gg_t &get_gen_graph ();
        const gen_meth_t to_gen_method_t (const std::string &s) const; 
        int read_graphml (const std::string &ifn);
        int write_graphviz (const std::string &ofn, bool simple=false);

    private:
        void setup_dynamic_property (dynamic_properties &dp, gg_t &g);
        gg_t g;
        dynamic_properties dp;
    };
}

#endif // RESOURCE_GEN_SPEC_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
