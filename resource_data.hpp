//!
//! Resource Pool/Relationship Data API Strawman
//!
// TODO: This needs to be redesigned so that each different
// TODO: Determine where the planner should reside.
//! TODO: how to make sched_data can be plugged in

#include <string>
#include <map>

#ifndef RESOURCE_DATA_HPP
#define RESOURCE_DATA_HPP

namespace flux_resource_model {

    //!
    //! Used to keep track of the state of each resource pool.
    //!
    enum resource_state_t {
        RESOURCE_INVALID,
        RESOURCE_IDLE,
        RESOURCE_ALLOCATED,
        RESOURCE_RESERVED,
        RESOURCE_DOWN,
        RESOURCE_UNKNOWN,
        RESOURCE_END
    };
    
    typedef std::string single_subsystem_t;
    
    //!
    //! Multi-subsystem type --
    //! Key: subsystem name, Value: relationship type.
    //! Used to mark a resource pool or resource relation.
    //!
    typedef std::map<
        single_subsystem_t,
        std::string
    > multi_subsystems_t;
    
    //!
    //! Multi-subsystem selector type --
    //! Used to filter the graph based on the subsystem names
    //! and relationship types.
    //!
    typedef std::map<
        single_subsystem_t,
        std::set<std::string>
    > multi_subsystemsS;

    //!
    //! Scheduler state (strowman) type --
    //! TODO: This needs to be redesigned so that each different
    //! scheduler can maintain/update its state keyed off of
    //! vertex or edge.
    //! TODO: Determine where the planner should reside.
    //!
    struct sched_state_t {
        int64_t available;
        int64_t staged;
        std::map<std::string, int64_t> tags;
        std::map<int64_t, int64_t> allocs;
        std::map<int64_t, int64_t> reservtns;
        std::map<int64_t, int64_t> twindow;
    };
    
    //!
    //! Resource pool type --
    //! TODO: how to make sched_data can be plugged in
    //! by the resource match plug-in.
    //!
    struct resource_pool_t {
        std::string type;
        std::map<std::string, std::string> paths;
        std::string basename;
        std::string name;
        std::map<std::string, std::string> properties;
        int64_t id;
        int64_t size;
        int64_t count;
        int stride;
        std::string unit;
        resource_state_t state;
        sched_state_t sched_data;
        boost::default_color_type color_map;
        multi_subsystems_t member_of;
    };
    
    //!
    //! Resource relationship type --
    //! An edge can be annotated with a set of
    //!   {key: subsystem, val: relationship-type}.
    //! IOW, an edge can represent a relationship within
    //! a subsystem for many subsystems. But it cannot
    //! represent multiple relationship within
    //! the same subsystem.
    //!
    struct resource_relation_t {
        multi_subsystems_t member_of;
    };
}

#endif // RESOURCE_DATA_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
