//!
//! Test Resource Generation Implementation
//!

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include "resource_spec.hpp"

using namespace std;
using namespace boost;
using namespace flux_resource_model;

struct t_scale_param_t {
    int nracks;
    int nnodes;
    int nesw;
    int npdus;
    int nnics;
    int nsocks;
    int ncores;
    int ngpus;
    int ncsws;
};

t_scale_param_t sparams[] = {
    { 1,  1,  1, 1, 1, 2, 1, 1, 1}, // mini
    { 4,  4,  1, 1, 1, 2, 2, 1, 1}, // small
    { 4,  18, 1, 1, 1, 2, 8, 1, 1}, // medium
    { 18, 18, 1, 1, 1, 2, 8, 1, 1}, // medium-plus
    { 36, 18, 1, 1, 1, 2, 8, 1, 1}, // large
    { 72, 18, 1, 1, 1, 2, 8, 1, 2}  // largest
};

//
// TODO: The following needs to be reduced down to resource spec reader code
//
static void spec_containment_subsystem (sspec_t *cluster, t_scale_param_t *s)
{
    sspec_gen_t use_id_pinfo (ID_GEN_USE_PARENT_INFO, NEW, 0, 0, "containment", "contains", "in");
    sspec_gen_t use_id_minfo (ID_GEN_USE_MY_INFO, NEW, 0, 0, "containment", "contains", "in");
    sspec_t *rack = new sspec_t (use_id_pinfo, "rack", "rack", s->nracks, 1, 1, "containment"); cluster->children.push_back (rack);
    sspec_t *node = new sspec_t (use_id_pinfo, "node", "node", s->nnodes, 1, 1, "containment"); rack->children.push_back (node);
    sspec_t *es = new sspec_t (use_id_pinfo, "edgeswitch", "edgeswitch", s->nesw, 1, 1, "containment"); rack->children.push_back (es);
    sspec_t *pdu = new sspec_t (use_id_pinfo, "pdu","pdu", s->npdus, 500, 1, "containment"); rack->children.push_back (pdu);
    sspec_t *esbw = new sspec_t (use_id_pinfo, "edgeswitchbw", "edgeswitchbw", 1, 40000 * s->nracks * s->nnodes, 1, "containment"); es->children.push_back (esbw);
    sspec_t *nic = new sspec_t (use_id_minfo, "nic", "nic", s->nnics, 1, 1, "containment"); node->children.push_back (nic);
    sspec_t *nicbw = new sspec_t (use_id_minfo, "nicbw", "nicbw", 1, 4, 1, "containment"); nic->children.push_back (nicbw);
    sspec_t *socket = new sspec_t (use_id_minfo, "socket", "socket", s->nsocks, 1, 1, "containment"); node->children.push_back (socket);
    sspec_t *core = new sspec_t (use_id_minfo, "core","core", s->ncores, 1, 1, "containment"); socket->children.push_back (core);
    sspec_t *gpu = new sspec_t (use_id_minfo, "gpu", "gpu", s->ngpus, 1, 1, "containment"); socket->children.push_back (gpu);
}

static void spec_ibnet_subsystem (sspec_t *ibnet, t_scale_param_t *s)
{
    sspec_gen_t use_id_pinfo (ID_GEN_USE_PARENT_INFO, NEW, 0, 0, "ibnet", "connected_down", "connected_up");
    sspec_gen_t use_id_minfo (ID_GEN_USE_MY_INFO, NEW, 0, 0, "ibnet", "contains", "in");
    sspec_gen_t use_assoc_in_cont (ID_GEN_USE_NONE, ASSOCIATE_IN, 0, 0, "containment", "connected_down", "connected_up");
    sspec_gen_t use_assoc_by_in_cont1 (ID_GEN_USE_NONE, ASSOCIATE_BY_PATH_IN, 2, 1, "containment", "connected_down", "connected_up");
    sspec_gen_t use_assoc_by_in_cont2 (ID_GEN_USE_NONE, ASSOCIATE_BY_PATH_IN, 0, 1, "containment", "connected_down", "connected_up");
    sspec_t *coreswitch = new sspec_t (use_id_pinfo, "coreswitch", "coreswitch", s->ncsws, 1, 1, "ibnet"); ibnet->children.push_back (coreswitch);
    sspec_t *csbw = new sspec_t (use_id_minfo, "coreswitchbw", "coreswitchbw", 1, 288000, 1, "ibnet"); coreswitch->children.push_back (csbw);
    sspec_t *es2 = new sspec_t (use_assoc_in_cont, "edgeswitch", "edgeswitch", 0, 0, 0, "ibnet"); coreswitch->children.push_back (es2);
    sspec_t *nic2 = new sspec_t (use_assoc_by_in_cont1, "nic", "nic", 0, 0, 0, "ibnet"); es2->children.push_back (nic2);
    sspec_t *node2 = new sspec_t (use_assoc_by_in_cont2, "node", "node", 0, 0, 0, "ibnet"); nic2->children.push_back (node2);
}

static void spec_power_subsystem (sspec_t *powerpanel, t_scale_param_t *s)
{
    sspec_gen_t use_assoc_in_cont (ID_GEN_USE_NONE, ASSOCIATE_IN, 0, 0, "containment", "flows", "drawn");
    sspec_gen_t use_assoc_by_in_cont (ID_GEN_USE_NONE, ASSOCIATE_BY_PATH_IN, 1, 1, "containment", "flows", "drawn");
    sspec_t *pdu2 = new sspec_t (use_assoc_in_cont, "pdu", "pdu", 1, 1, 1, "power"); powerpanel->children.push_back (pdu2);
    sspec_t *node4 = new sspec_t (use_assoc_by_in_cont, "node", "node", 1, 1, 1, "power"); pdu2->children.push_back (node4);
}

static void spec_ibnetbw_subsystem (sspec_t *ibnetbw, t_scale_param_t *s)
{
    sspec_gen_t use_assoc_in_ibnet (ID_GEN_USE_NONE, ASSOCIATE_IN, 0, 0, "ibnet", "flows_down", "flows_up");
    sspec_gen_t use_assoc_in_cont (ID_GEN_USE_NONE, ASSOCIATE_IN, 0, 0, "containment", "flows_down", "flows_up");
    sspec_gen_t use_assoc_by_in_cont1 (ID_GEN_USE_NONE, ASSOCIATE_BY_PATH_IN, 3, 2, "containment", "flows_down", "flows_up");
    sspec_gen_t use_assoc_by_in_cont2 (ID_GEN_USE_NONE, ASSOCIATE_BY_PATH_IN, 0, 2, "containment", "flows_down", "flows_up");
    sspec_t *csbw = new sspec_t (use_assoc_in_ibnet, "coreswitchbw", "coreswitchbw", 1, 1, 1, "ibnetbw"); ibnetbw->children.push_back (csbw);
    sspec_t *esbw = new sspec_t (use_assoc_in_cont, "edgeswitchbw", "edgeswitchbw", 1, 72000, 1, "ibnetbw"); csbw->children.push_back (esbw);
    sspec_t *nicbw = new sspec_t (use_assoc_by_in_cont1, "nicbw", "nicbw", 1, 4, 1, "ibnetbw"); esbw->children.push_back (nicbw);
    sspec_t *node = new sspec_t (use_assoc_by_in_cont2, "node", "node", 1, 4, 1, "ibnetbw"); nicbw->children.push_back (node);
}

static void spec_pfs1bw_subsystem (sspec_t *pfs1bw, t_scale_param_t *s)
{
    sspec_gen_t use_id_pinfo (ID_GEN_USE_PARENT_INFO, NEW, 0, 0, "pfs1bw", "flows_down", "flows_up");
    sspec_gen_t use_assoc_in_ibnet (ID_GEN_USE_NONE, ASSOCIATE_IN, 0, 0, "ibnet", "flows_down", "flows_up");
    sspec_gen_t use_assoc_in_cont (ID_GEN_USE_NONE, ASSOCIATE_IN, 0, 0, "containment", "flows_down", "flows_up");
    sspec_gen_t use_assoc_by_in_cont1 (ID_GEN_USE_NONE, ASSOCIATE_BY_PATH_IN, 3, 2, "containment", "flows_down", "flows_up");
    sspec_gen_t use_assoc_by_in_cont2 (ID_GEN_USE_NONE, ASSOCIATE_BY_PATH_IN, 0, 2, "containment", "flows_down", "flows_up");
    sspec_t *lnetbw = new sspec_t (use_id_pinfo, "lnetbw", "lnetbw", 1, 40000, 1, "pfs1bw"); pfs1bw->children.push_back (lnetbw);
    sspec_t *csbw = new sspec_t (use_assoc_in_ibnet, "coreswitchbw", "coreswitchbw", 1, 1, 1, "pfs1bw"); lnetbw->children.push_back (csbw);
    sspec_t *esbw = new sspec_t (use_assoc_in_cont, "edgeswitchbw", "edgeswitchbw", 1, 1, 1, "pfs1bw");csbw->children.push_back (esbw);
    sspec_t *nicbw = new sspec_t (use_assoc_by_in_cont1, "nicbw", "nicbw", 1, 1, 1, "pfs1bw"); esbw->children.push_back (nicbw);
    sspec_t *node = new sspec_t (use_assoc_by_in_cont2, "node", "node", 1, 1, 1, "pfs1bw"); nicbw->children.push_back (node);
}

//
// Public Test Resource Generation API Implementation
//

//!
//! Build a test subsystem specification ranging from mini- to large-size cluster
//!
//! \param scale scale of the test subsystem
//! \param specs output specification represented as a vector of sspec_t objects
//! \return      0 on success; non-zero integer on an error
//!
int flux_resource_model::test_spec_build (t_scale_t scale, std::vector <sspec_t *> &specs)
{
    sspec_gen_t use_id_minfo (ID_GEN_USE_MY_INFO);
    sspec_t *cluster = new sspec_t (use_id_minfo, "cluster", "cluster", 1, 1, 1, "containment");
    sspec_t *ibnet = new sspec_t (use_id_minfo, "ibnet", "ibnet", 1, 1, 1, "ibnet");
    sspec_t *ibnetbw = new sspec_t (use_id_minfo, "ibnetbw", "ibnetbw", 1, 1, 1, "ibnetbw");
    sspec_t *pfs1bw = new sspec_t (use_id_minfo, "pfs1bw", "pfs1bw", 1, 1, 1, "pfs1bw");
    sspec_t *powerpanel = new sspec_t (use_id_minfo, "powerpanel", "powerpanel", 1, 1, 1, "power");

    spec_containment_subsystem (cluster, &sparams[scale]);
    specs.push_back (cluster);
    spec_ibnet_subsystem (ibnet, &sparams[scale]);
    specs.push_back (ibnet);
    spec_ibnetbw_subsystem (ibnetbw, &sparams[scale]);
    specs.push_back (ibnetbw);
    spec_power_subsystem (powerpanel, &sparams[scale]);
    specs.push_back (powerpanel);
    spec_pfs1bw_subsystem (pfs1bw, &sparams[scale]);
    specs.push_back (pfs1bw);

    return 0;
}

//!
//! Return a scale enum value corresponding to the string
//!
//! \param st    scale string
//! \param scale scale enum value
//! \return      0 on success; non-zero integer on an error
//!
int flux_resource_model::test_spec_string_to_scale (string st, t_scale_t &scale)
{
    int rc = 0;
    if (iequals (st, string ("MINI")))
        scale = TS_MINI;
    else if (iequals (st, string ("SMALL")))
        scale = TS_SMALL;
    else if (iequals (st, string ("MEDIUM")))
        scale = TS_MEDIUM;
    else if (iequals (st, string ("MEDPLUS")))
        scale = TS_MEDPLUS;
    else if (iequals (st, string ("LARGE")))
        scale = TS_LARGE;
    else if (iequals (st, string ("LARGEST")))
        scale = TS_LARGEST;
    else
        rc = -1;

    return rc;
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

