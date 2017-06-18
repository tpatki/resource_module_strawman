//
// Resource Specification API
//!
//! Resource Subsystem Specification API Strawman
//!

#include <string>
#include <vector>
#include "resource_graph.hpp"

#ifndef RESOURCE_SPEC_HPP
#define RESOURCE_SPEC_HPP

namespace flux_resource_model {

    enum id_meth_t {
        ID_GEN_USE_NONE,
        ID_GEN_USE_PARENT_INFO,
        ID_GEN_USE_MY_INFO
    };

    enum rs_meth_t {
        NEW,
        ASSOCIATE_IN,
        ASSOCIATE_BY_PATH_IN
    };

    struct sspec_gen_t {
        sspec_gen_t (
            id_meth_t im = ID_GEN_USE_NONE,
            rs_meth_t rm = NEW,
            int ulm = 0,
            int ulp = 0,
            std::string is = "",
            std::string p2me = "",
            std::string me2p = "")

        : imeth (im),
        rmeth (rm),
        uplevel_me (ulm),
        uplevel_parent (ulp),
        in_subsystem (is),
        p2me_type (p2me),
        me2p_type (me2p) { }

        id_meth_t imeth;
        rs_meth_t rmeth;
        int uplevel_me;
        int uplevel_parent;
        std::string in_subsystem;
        std::string p2me_type;
        std::string me2p_type;
    };

    struct sspec_t {
        sspec_t (
            sspec_gen_t &gi,
            const std::string &t,
            const std::string &b,
            int c,
            int s,
            int st,
            std::string ss)
        : gen_info (gi),
        type (t),
        basename (b),
        count (c),
        size (s),
        stride (st),
        ssys (ss) { }

        sspec_gen_t gen_info; // info to generate child resource pools
        std::string type;     // resource pool type
        std::string basename; // basename of the resource pool
        int count;            // how many resource pools
        int size;             // size of each resource pool
        int stride;           // stride to use for the id of resource pool
        std::string ssys;     // subsystem the resource pool belongs to
        std::vector <sspec_t *> children;
    };

    enum t_scale_t {
        TS_MINI     = 0,
        TS_SMALL    = 1,
        TS_MEDIUM   = 2,
        TS_MEDPLUS  = 3,
        TS_LARGE    = 4,
        TS_LARGEST  = 5
    };

    int test_spec_string_to_scale (std::string st, t_scale_t &sc);
    int test_spec_build (t_scale_t scale, std::vector <sspec_t *> &specs);
}

#endif // RESOURCE_SPEC_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
