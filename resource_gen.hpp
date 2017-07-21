//!
//! Resource Generator API Strawman
//!

#include <string>
#include <boost/graph/depth_first_search.hpp>
#include "resource_graph.hpp"
#include "resource_gen_spec.hpp"

#ifndef RESOURCE_GEN_HPP
#define RESOURCE_GEN_HPP

namespace flux_resource_model {
    class resource_generator_t {
    public:
        resource_generator_t ();
        resource_generator_t (const resource_generator_t &o);
        const resource_generator_t &operator=(const resource_generator_t &o);
        ~resource_generator_t ();
        int read_graphml (const std::string &f, resource_graph_db_t &db);
        const std::string &get_err_message () const;
    private:
        resource_gen_spec_t gspec;
        std::string err_msg = "";
    };
}

#endif // RESOURCE_GEN_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
