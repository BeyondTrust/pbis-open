/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */
/*
 */
#ifndef _RPCLIST_H
#define _RPCLIST_H
/*
**
**  NAME:
**
**      rpclist.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Various types and macros for list and queue processing.
**
**
*/


/***********************************************************************/
/*
 * List Processing.
 * 
 * The RPC services maintain a number of structures in the form of lists
 * and queues.  Lists are doubly linked with the list head containing both
 * a pointer to the first element of the list as well as a pointer to the
 * last element in the list.  The elements on these lists are maintained in
 * first-in first-out order. By maintaining the last element pointer,
 * elements can be easily added to the end of the list.
 *
 * Structures which are going to be used in lists should have a member of
 * the type rpc_list_t as their first member.
 *
 * List descriptors are used to maintain context for lists which are going
 * to take advantage of memory management optimizations (lookaside lists).
 * Memory for these lists is obtained in large "chunks", which is then
 * provided to individual list elements on an as-needed basis.
 *
 * ***CAVEAT***: 
 * 
 * The structures kept on lists which are processed by the
 * RPC_LIST* macros can be kept on only one list at a time.
 * This is because the macros *assume* the first field of the
 * structure given to them is a "link" field (an rpc_list_t)
 * structure. It casts the structure to to a be a pointer to an
 * rpc_list_t structure and manipulates the fields of it ("next" and
 * "last"). Enqueuing a structure on a second list would effectively
 * remove it from the first list since the "next" and "last" fields
 * would be overwritten. 
 */

#include <dce/dce.h>

/*int pthd4_get_expiration_np(struct timespec *delta, struct timespec *abstime);*/


/***********************************************************************/
/*
 * R P C _ L I S T _ T
 *
 * This is a list head (or a set of reference pointers in a list element).
 */

typedef struct
{
    pointer_t   next;   /* next element of list                     */
    pointer_t   last;   /* last element of list in a descriptor or  */
                        /* pointer to the prior element in an element */
} rpc_list_t, *rpc_list_p_t;



/***********************************************************************/
/*
 * R P C _ L I S T _ E L E M E N T _ A L L O C _ F N _ T
 *
 */

typedef void (*rpc_list_element_alloc_fn_t) (
        pointer_t   /*list_element*/
    );


/***********************************************************************/
/*
 * R P C _ L I S T _ E L E M E N T _ F R E E _ F N _ T
 *
 */

typedef void (*rpc_list_element_free_fn_t) (
        pointer_t   /*list_element*/
    );


/***********************************************************************/
/*
 * R P C _ L I S T _ D E S C _ T
 *
 */

typedef struct
{
    rpc_list_t                  list_head;
    unsigned32                  max_size;
    unsigned32                  cur_size;
    unsigned32                  element_size;
    unsigned32                  element_type;
    rpc_list_element_alloc_fn_t alloc_rtn;
    rpc_list_element_free_fn_t  free_rtn;
    rpc_mutex_t                 *mutex;
    rpc_cond_t                  *cond;
    boolean32                   use_global_mutex;
} rpc_list_desc_t, *rpc_list_desc_p_t;


/***********************************************************************/
/*
 * R P C _ L O O K A S I D E _ R C B _ T
 */

typedef struct
{
    unsigned16                  res_id;
    unsigned16                  waiter_cnt;
    unsigned16                  max_wait_times;
    unsigned16                  wait_time;
    rpc_mutex_t                 res_lock;
    rpc_cond_t                  wait_flg;
} rpc_lookaside_rcb_t, *rpc_lookaside_rcb_p_t;

#define RPC_C_LOOKASIDE_RES                     1
#define RPC_C_LOOKASIDE_RES_WAIT                1
#define RPC_C_LOOKASIDE_RES_MAX_WAIT            5

EXTERNAL rpc_lookaside_rcb_t rpc_g_lookaside_rcb;

/***********************************************************************/
/* 
 * R P C _ L I S T _ M U T E X _ I N I T
 * 
 *  Initialize the global lookaside list mutex.
 *
 * Sample usage:
 *
 *      RPC_LIST_MUTEX_INIT (0);
 */
#define RPC_LIST_MUTEX_INIT(junk) \
{ \
    RPC_MUTEX_INIT (rpc_g_lookaside_rcb.res_lock); \
    RPC_COND_INIT (rpc_g_lookaside_rcb.wait_flg, \
		   rpc_g_lookaside_rcb.res_lock); \
}

/***********************************************************************/
/* 
 * R P C _ L I S T _ I N I T
 * 
 *  Initialize a list head.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *
 *      RPC_LIST_INIT (list);
 */

#define RPC_LIST_INIT(list) \
{ \
    list.next = NULL; \
    list.last = NULL; \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ E M P T Y
 *
 *  Determine whether or not a list has any elements left.
 *
 *  RETURNS: true   - list has more elements
 *           false  - list has no more elements
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *
 *      if (RPC_LIST_EMPTY (list)) ...
 */

#define RPC_LIST_EMPTY(list)    (list.next == NULL)


/***********************************************************************/
/*
 * R P C _ L I S T _ A D D
 *
 * Insert an element after a specified entry in a list.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;               (listhead)
 *      any_p_t             list_element;       (location to insert)
 *      any_p_t             insert_element;     (element to be inserted)
 *
 *      RPC_LIST_ADD (list, list_element, insert_element, any_p_t);
 */

#define RPC_LIST_ADD(list, list_element, insert_element, list_element_type) \
{ \
    ((rpc_list_p_t) (insert_element))->next = \
        ((rpc_list_p_t) (list_element))->next;  \
    ((rpc_list_p_t) (insert_element))->last = (pointer_t) (list_element); \
    if (((rpc_list_p_t) (list_element))->next == NULL) \
        { \
            (list).last = (pointer_t) (insert_element); \
        } \
    else \
        { \
            ((rpc_list_p_t) (((rpc_list_p_t) (list_element))->next))->last = \
                    (pointer_t) (insert_element); \
        } \
    ((rpc_list_p_t) (list_element))->next = (pointer_t) (insert_element); \
}


/***********************************************************************/
/*
 * R P C _ L I S T _ A D D _ H E A D
 *
 * Add an entry to the beginning of a list.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_ADD_HEAD (list, list_element, any_p_t);
 */

#define RPC_LIST_ADD_HEAD(list, list_element, list_element_type) \
{ \
    if (RPC_LIST_EMPTY (list)) \
    { \
        (list).next = (list).last = (pointer_t) (list_element); \
        ((rpc_list_p_t) (list_element))->next = NULL; \
        ((rpc_list_p_t) (list_element))->last = (pointer_t) &(list); \
    } \
    else \
    { \
        ((rpc_list_p_t) (list_element))->next = (pointer_t) ((list).next); \
        ((rpc_list_p_t) (list_element))->last = (pointer_t) &(list); \
        ((rpc_list_p_t)((list).next))->last = (pointer_t) (list_element); \
        (list).next = (pointer_t) (list_element); \
    } \
}


/***********************************************************************/
/*
 * R P C _ L I S T _ A D D _ T A I L
 *
 * Add an entry to the end of a list.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_ADD_TAIL (list, list_element, any_p_t);
 */

#define RPC_LIST_ADD_TAIL(list, list_element, list_element_type) \
{ \
    if (RPC_LIST_EMPTY (list)) \
    { \
        list.next = (pointer_t) (list_element); \
        ((rpc_list_p_t) (list_element))->last = (pointer_t) &(list); \
    } \
    else \
    { \
        ((rpc_list_p_t) (list.last))->next = (pointer_t) (list_element); \
        ((rpc_list_p_t) (list_element))->last = list.last; \
    } \
    list.last = (pointer_t) (list_element); \
    ((rpc_list_p_t) (list_element))->next = NULL; \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ R E M O V E
 *
 * Remove an element (pointed to by list_element) from a list.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_REMOVE (list, list_element);
 */

#define RPC_LIST_REMOVE(list, list_element) \
{ \
    if (list.last == list.next) \
    { \
        RPC_LIST_INIT (list); \
    } \
    else \
    { \
        if (((rpc_list_p_t) (list_element))->next == NULL) \
        { \
            list.last = ((rpc_list_p_t) (list_element))->last; \
        } \
        else \
        { \
            ((rpc_list_p_t) ((rpc_list_p_t) (list_element))->next)->last = \
                ((rpc_list_p_t) (list_element))->last; \
        } \
        ((rpc_list_p_t) ((rpc_list_p_t) (list_element))->last)->next = \
            ((rpc_list_p_t) (list_element))->next; \
    } \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ R E M O V E _ H E A D
 *
 * Remove the first entry from a list and return it. If the list is
 * empty a NULL pointer is returned. Note: This is functionally
 * equivalent to doing RPC_LIST_EXTRACT (,,1), but is slightly more
 * efficient.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_REMOVE_HEAD (list, list_element, any_p_t);
 */

#define RPC_LIST_REMOVE_HEAD(list, list_element, list_element_type) \
{ \
    if (RPC_LIST_EMPTY (list)) \
    { \
        list_element = NULL; \
    } \
    else \
    { \
        RPC_LIST_FIRST (list, list_element, list_element_type); \
        RPC_LIST_REMOVE (list, list_element); \
    } \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ R E M O V E _ T A I L
 *
 * Remove the last entry from a list and return it. If the list is
 * empty a NULL pointer is returned.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_REMOVE_TAIL (list, list_element, any_p_t);
 */

#define RPC_LIST_REMOVE_TAIL(list, list_element, list_element_type) \
{ \
    if (RPC_LIST_EMPTY (list)) \
    { \
        list_element = NULL; \
    } \
    else \
    { \
        RPC_LIST_LAST (list, list_element, list_element_type); \
        RPC_LIST_REMOVE (list, list_element); \
    } \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ E X T R A C T
 * 
 * Remove the nth entry on a list and return it.
 * If n exceeds the length of the list a NULL pointer is returned.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_EXTRACT (list, list_element, any_p_t, n);
 */

#define RPC_LIST_EXTRACT(list, list_element, list_element_type, n) \
{ \
    RPC_LIST_LOOKUP (list, list_element, list_element_type, n); \
    if (list_element != NULL) \
    { \
        RPC_LIST_REMOVE (list, list_element); \
    } \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ F I R S T
 * 
 * Returns the first element in a list (without removing it).
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_FIRST (list, list_element, any_p_t);
 */

#define RPC_LIST_FIRST(list, list_element, list_element_type) \
    list_element = (list_element_type) (list.next);
    

/***********************************************************************/
/* 
 * R P C _ L I S T _ L A S T
 * 
 * Return the last element in a list (without removing it).
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *
 *      RPC_LIST_LAST (list, list_element, any_p_t);
 */

#define RPC_LIST_LAST(list, list_element, list_element_type) \
    list_element = (list_element_type) (list.last);
    

/***********************************************************************/
/* 
 * R P C _ L I S T _ N E X T
 * 
 * Can be used iteratively to walk a list and read all entries.
 * When there are no more entries a NULL pointer is returned.
 *
 * Sample usage:
 *
 *      any_p_t             this_element;
 *      any_p_t             next_element;
 *
 *      RPC_LIST_NEXT (this_element, next_element, any_p_t);
 */

#define RPC_LIST_NEXT(this_element, next_element, list_element_type) \
{ \
    if (((rpc_list_p_t) (this_element))->next == NULL) \
    { \
        next_element = NULL; \
    } \
    else \
    { \
        next_element = \
            (list_element_type) (((rpc_list_p_t) (this_element))->next); \
    } \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ L O O K U P
 * 
 * Get the nth entry on a list (without removing it).
 * If n exceeds the length of the list a NULL pointer is returned.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      any_p_t             list_element;
 *      any_int             n;
 *
 *      RPC_LIST_LOOKUP (list, list_element, any_p_t, n);
 */

#define RPC_LIST_LOOKUP(list, list_element, list_element_type, n) \
{ \
    int                 _count; \
\
    for (_count = (int) n, \
            list_element = (list_element_type) (list.next); \
        (_count > 1) && (list_element != NULL); \
        list_element = \
            (list_element_type) (((rpc_list_p_t) (list_element))->next), \
        _count--); \
}


/***********************************************************************/
/* 
 * R P C _ L I S T _ C O U N T
 * 
 * Return the number of entries on the list.
 *
 * Sample usage:
 *
 *      rpc_list_t          list;
 *      unsigned32          count;
 *
 *      RPC_LIST_COUNT (list, count);
 */

#define RPC_LIST_COUNT(list, count) \
{ \
    rpc_list_p_t    _next_element;\
\
    for (count = 0, _next_element = ((rpc_list_p_t)(list.next));\
         _next_element != NULL;\
         count++, _next_element = ((rpc_list_p_t)(_next_element->next)));\
}


/***********************************************************************/
/*
 * R P C _ _ L I S T _ D E S C _ I N I T
 *
 */

PRIVATE void rpc__list_desc_init (
        rpc_list_desc_p_t            /*list_desc*/,
        unsigned32                   /*max_size*/,
        unsigned32                   /*element_size*/,
        unsigned32                   /*element_type*/,
        rpc_list_element_alloc_fn_t  /*alloc_rtn*/,
        rpc_list_element_free_fn_t   /*free_rtn*/,
        rpc_mutex_p_t                /*mutex*/,
        rpc_cond_p_t                 /*cond*/
    );


/***********************************************************************/
/*
 * R P C _ _ L I S T _ E L E M E N T _ A L L O C
 *
 */

PRIVATE pointer_t rpc__list_element_alloc (
        rpc_list_desc_p_t            /*list_desc*/,
        boolean32                    /*block*/
    );

  
/***********************************************************************/
/*
 * R P C _ _ L I S T _ E L E M E N T _ F R E E
 *
 */

PRIVATE void rpc__list_element_free (
        rpc_list_desc_p_t        /*list_desc*/,
        pointer_t                /*list_element*/
    );

/***********************************************************************/
/*
 * R P C _ _ L I S T _ F O R K _ H A N D L E R
 *
 */

PRIVATE void rpc__list_fork_handler (
        rpc_fork_stage_id_t      /*stage*/
    );


#endif /* _RPCLIST_H */
