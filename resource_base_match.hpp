//!
//! Resource Base Matcher API Strawman
//!
// TODO: richer ways to specify matching subsystem(s) and type(s)
//          e.g., wildcard, filter, substring match ...

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>

#ifndef RESOURCE_BASE_MATCH_HPP
#define RESOURCE_BASE_MATCH_HPP

namespace flux_resource_model {

    enum matcher_rc_t {
        MATCHER_WALK_PRUNED = 0,
        MATCHER_WALK_NOT_TREE_EDGE,
        MATCHER_WALK_ERROR,
        MATCHER_SCORE_BASELINE,
    };
    
    //!
    //! Base matcher class.
    //! Provides idiom to specify the target subsystems and
    //! resource relation types which then allows resource
    //! module to filter the graph DB for this matcher
    //!
    class resource_base_matcher_t
    {
    public:
        resource_base_matcher_t ()
        {
            m_name = "anonymous";
        }
        resource_base_matcher_t (const std::string &name)
        {
            m_name = name;
        }
        ~resource_base_matcher_t ()
        {
            m_subsystems.clear ();
            m_subsystems_map.clear ();
        }

        //!
        //! Add a subsystem and the relationship type that this resource base matcher
        //! will use. Vertices and edges of the resource graph are filtered in based
        //! on this information. Each vertex and edge that is a part of this subsystem
        //! and relationship type will be selected.
        //!
        //! This method must be called at least once to set the dominant subsystem
        //! to use. This method can be called multiple times with a distinct subsystem,
        //! each becomes an auxiliary subsystem. The queuing order can be reconstructed
        //! get_subsystems.
        //!
        //! \param s     a subsystem to select
        //! \param tf    edge (or relation type) to select. pass * for selecting all types
        //! \param m     matcher_t
        //! \return      0 on success; -1 on an error
        //!
        int add_subsystem (const single_subsystem_t s, const std::string tf = "*")
        {
            if (m_subsystems_map.find (s) == m_subsystems_map.end ()) {
                m_subsystems.push_back (s);
                m_subsystems_map[s].insert (tf);
                return 0;
            }
            return -1;
        }

        const std::string &get_matcher_name ()
        {
            return m_name;
        }

        void set_matcher_name (const std::string &name)
        {
            m_name = name;
        }
        
        const std::vector<single_subsystem_t> &get_subsystems ()
        {
            return m_subsystems;
        }
        
        //!
        //! \return      return the dominant subsystem this matcher has selected to use
        //!
        const single_subsystem_t &get_dom_subsystem ()
        {
            return *(m_subsystems.begin());
        }
        
        //!
        //! \return      return the subsystem selector to be used in graph filtering
        //!
        const multi_subsystemsS &get_subsystemsS ()
        {
            return m_subsystems_map;
        }

    private:
        std::string m_name;
        std::vector<single_subsystem_t> m_subsystems;
        multi_subsystemsS m_subsystems_map;
    };
}
#endif // RESOURCE_BASE_MATCH_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

