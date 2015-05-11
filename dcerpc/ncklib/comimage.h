#ifndef _COMIMAGE_H
#define _COMIMAGE_H 1

/* init funcs for modules */
PRIVATE void rpc__module_init_func(void);


PRIVATE void rpc__register_protseq(rpc_protseq_id_elt_p_t elt, int number);
PRIVATE void rpc__register_tower_prot_id(rpc_tower_prot_ids_p_t tower_prot, int number);
PRIVATE void rpc__register_protocol_id(rpc_protocol_id_elt_p_t prot, int number);
PRIVATE void rpc__register_naf_id(rpc_naf_id_elt_p_t naf, int number);
PRIVATE void rpc__register_authn_protocol(rpc_authn_protocol_id_elt_p_t auth, int number);

#endif
