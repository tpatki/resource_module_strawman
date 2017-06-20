//!
//! Resource Base DFU Matcher API Strawman
//!

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include "resource_graph.hpp"
#include "resource_base_match.hpp"

#ifndef RESOURCE_BASE_DFU_MATCH_HPP
#define RESOURCE_BASE_DFU_MATCH_HPP

namespace flux_resource_model {

    //!
    //! Base DFU matcher class.
    //!  Define the set of visitor methods that are called
    //! back by a resource graph traverser.
    //!
    class resource_base_dfu_matcher_t : public resource_base_matcher_t
    {
    public:
        resource_base_dfu_matcher_t () : m_trav_level (0) { }
        resource_base_dfu_matcher_t (const std::string &name)
            : resource_base_matcher_t (name), m_trav_level (0) { }
        ~resource_base_dfu_matcher_t () { }

        void incr () { m_trav_level++; }
        void decr () { m_trav_level--; }

        //!
        //! Called back on each preorder visit of the dominant subsystem.
        //! \param u     descriptor of the visiting vertex
        //! \param g     filtered resource graph object
        //! \return      return MATCHER_WALK_PRUNED when further searching
        //!                  needs to be pruned;
        //!                  otherwise MATCHER_SCORE_BASELINE
        //!
        int dom_discover_vtx (vtx_t u, f_resource_graph_t &g)
        {
            incr ();
            std::cout << level_prefix ()
                      << "dom_discover_vtx: "
                      << g[u].name
                      << " " <<std::endl;
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on each postorder visit of the dominant subsystem. Should
        //! return a score calculated based on the subtree and up walks. Any score
        //! aboved MATCHER_SCORE_BASELINE is qualified to be a match.
        //!
        //! \param u     descriptor of the visiting vertex
        //! \param score_map   vector of integer scores evaluated as part
        //!              of auxiliary subsystem up-walks
        //! \param g     filtered resource graph object
        //! \return      return a score calculated based on the subtree and up walks.
        //!              any score aboved MATCHER_SCORE_BASELINE is qualified to be a match
        //!
        int dom_finish_vtx (vtx_t u, std::map<single_subsystem_t,
                std::vector<int> > &score_map, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "dom_finish_vtx: "
                      << g[u].name
                      << " " << std::endl;
            decr ();
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on traversing a tree edge of the dominant subsystem
        //!
        //! \param e     descriptor of the traversing edge
        //! \param g     filtered resource graph object
        //!
        int dom_tree_edge (edg_t e, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "dom_tree_edge: "
                      << std::endl;
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on traversing a back edge of the dominant subsystem.
        //! This means a cycle, and default action is not to traverse this
        //! edge further.
        //!
        //! \param e     descriptor of the traversing edge
        //! \param g     filtered resource graph object
        //!
        int dom_back_edge (edg_t e, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "dom_back_edge: CYCLE! CYCLE! in the graph"
                      << std::endl;
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on traversing a forward or cross edge
        //! of the dominant subsystem.
        //!
        //! \param e     descriptor of the traversing edge
        //! \param g     filtered resource graph object
        //!
        int dom_forward_or_cross_edge (edg_t e, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "dom_forward_or_cross_edge: PROBLEM! PROBLEM!"
                      << std::endl;
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on each pre-up visit of an auxiliary subsystem.
        //! \param u     descriptor of the visiting vertex
        //! \param g     filtered resource graph object
        //! \return      return MATCHER_WALK_PRUNED when further searching
        //!              needs to be pruned; otherwise MATCHER_SCORE_BASELINE
        //!
        int aux_discover_vtx (vtx_t u, f_resource_graph_t &g)
        {
            incr ();
            std::cout << level_prefix ()
                      << "aux_discover_vtx: "
                      << g[u].name
                      << " " <<std::endl;
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on each post-up visit of the auxiliary subsystem. Should
        //! return a score calculated based on the subtree and up walks. Any score
        //! aboved MATCHER_SCORE_BASELINE is qualified to be a match.
        //!
        //! \param u     descriptor of the visiting vertex
        //! \param score_map   vector of integer scores evaluated as part
        //!              of auxiliary subsystem up-walks
        //! \param g     filtered resource graph object
        //! \return      return a score calculated based on the subtree
        //!              and up walks. Any score aboved MATCHER_SCORE_BASELINE
        //!              is qualified to be a match
        //!
        int aux_finish_vtx (vtx_t u, std::map<single_subsystem_t,
                std::vector<int> > &score_map, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "aux_finish_vtx: "
                      << g[u].name
                      << " " <<std::endl;
            decr ();
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on traversing an up edge of an auxiliary subsystem
        //! The target vertex has not been visited by dominant depth first visit
        //!
        //! \param e     descriptor of the traversing edge
        //! \param g     filtered resource graph object
        //!
        int aux_up_edge (edg_t e, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "aux_up_edge: "
                      << std::endl;
            return MATCHER_SCORE_BASELINE;
        }

        //!
        //! Called back on traversing an back edge of an auxiliary subsystem
        //! The target vertex is being visited by the upwalk on the same subsystem 
        //!
        //! \param e     descriptor of the traversing edge
        //! \param g     filtered resource graph object
        //!
        int aux_up_back_edge (edg_t e, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "aux_up_back_edge: CYCLE! CYCLE! "
                      << std::endl;
            return MATCHER_SCORE_BASELINE;
        }

        //
        //! Called back on traversing an up edge of an auxiliary subsystem
        //! The target vertex has been visited by the upwalk on the same subsystem
        //!
        //! \param e     descriptor of the traversing edge
        //! \param g     filtered resource graph object
        //!
        int aux_up_forward_edge (edg_t e, f_resource_graph_t &g)
        {
            std::cout << level_prefix ()
                      << "aux_up_forward_edge: MAKE SURE THIS IS OK"
                      << std::endl;
            return MATCHER_SCORE_BASELINE;
        }

    private:

        std::string level_prefix ()
        {
            int i;
            std::string prefix = "";
            for (i = 0; i < m_trav_level; ++i)
                prefix += "----";
            return prefix;
        }

        int m_trav_level;
    };
}

#endif // RESOURCE_BASE_DFU_MATCH_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
