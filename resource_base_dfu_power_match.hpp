//!
//! Resource Base DFU Matcher API Strawman
//!

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include "resource_graph.hpp"
#include "resource_base_match.hpp"
#include "resource_base_dfu_match.hpp"

#ifndef RESOURCE_BASE_DFU_POWER_MATCH_HPP
#define RESOURCE_BASE_DFU_POWER_MATCH_HPP

namespace flux_resource_model {

    //!
    //! Base DFU matcher class.
    //!  Define the set of visitor methods that are called
    //! back by a resource graph traverser.
    //!
    class resource_base_dfu_power_dom_matcher_t : public resource_base_dfu_matcher_t
    {
    public:
        resource_base_dfu_power_dom_matcher_t () = default;
        resource_base_dfu_power_dom_matcher_t (const std::string &name)
            : resource_base_dfu_matcher_t (name) { }

        //!
        //! Called back on each preorder visit of the dominant subsystem.
        //! \param u     descriptor of the visiting vertex
        //! \param g     filtered resource graph object
        //! \return      return MATCHER_WALK_PRUNED when further searching
        //!                  needs to be pruned;
        //!                  otherwise MATCHER_SCORE_BASELINE
        //!
        virtual int dom_discover_vtx (vtx_t u, f_resource_graph_t &g)
        { 
            incr ();
            if (g[u].type == "node") {
                std::cout << level_prefix ()
                          << "dom_discover_vtx: "
                          << g[u].name
                          << " (ensure node is free with enough power) " << std::endl;
            } else if (g[u].type == "pdu") {
                std::cout << level_prefix ()
                          << "dom_discover_vtx: "
                          << g[u].name
                          << " (ensure that req_nodes * power_per_node < pdu && req_nodes <= free_nodes) " << std::endl;
            } else {
                std::cout << level_prefix ()
                          << "dom_discover_vtx: "
                          << g[u].name
                          << " (found a vertex we haven't accounted for in power calculation) " << std::endl;

           }
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on each postorder visit of the dominant subsystem.        //! Should return a score calculated based on the subtree and up walks. Any score
        //! aboved MATCHER_SCORE_BASELINE is qualified to be a match.
        //!
        //! \param u     descriptor of the visiting vertex
        //! \param score_map   vector of integer scores evaluated as part
        //!              of auxiliary subsystem up-walks
        //! \param g     filtered resource graph object
        //! \return      return a score calculated based on the subtree and up walks.
        //!              any score aboved MATCHER_SCORE_BASELINE is qualified to be a match
        //!
        virtual int dom_finish_vtx (vtx_t u, std::map<single_subsystem_t,
                std::vector<int> > &score_map, f_resource_graph_t &g)
        {

            if (g[u].type == "node") {
                std::cout << level_prefix ()
                      << "dom_finish_vtx: "              
                      << g[u].name
                      << "Allocate node with desired power." << std::endl;
            } else {
                std::cout << level_prefix ()
                          << "dom_finish_vtx: "              
                          << g[u].name
                          << " " << std::endl;
            }
            decr ();
            return MATCHER_SCORE_BASELINE;
        }

    }; //end of class

class resource_base_dfu_power_aux_matcher_t: public resource_base_dfu_matcher_t
{
    
    public:
        resource_base_dfu_power_aux_matcher_t () = default;
        resource_base_dfu_power_aux_matcher_t (const std::string &name)
            : resource_base_dfu_matcher_t (name) { }
    
    virtual int aux_discover_vtx (vtx_t u, f_resource_graph_t &g)
        {
            incr ();
                std::cout << level_prefix ()
                          << "dom_discover_vtx: "
                          << g[u].name
                          << "(check if enough power is available?) " <<std::endl;

            return MATCHER_SCORE_BASELINE;
        }

        virtual int aux_finish_vtx (vtx_t u, std::map<single_subsystem_t,
                std::vector<int> > &score_map, f_resource_graph_t &g)
        {
                std::cout << level_prefix ()
                          << "dom_finish_vtx: "              
                          << g[u].name
                          << " " << std::endl;
            decr ();
            return MATCHER_SCORE_BASELINE;
        }
}; // end of class
} //end of namespace

#endif // RESOURCE_BASE_DFU_MATCH_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
