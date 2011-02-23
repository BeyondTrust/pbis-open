/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
/*
**  NAME:
**
**      ernodtbl.c
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      Node table (address <=> node number) maintenance
**
**  VERSION: DCE 1.0
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


/*
OVERALL OVERVIEW OF NODE MANAGEMENT ROUTINES AND STRUCTURES

The RPC IDL compiler terms chunks of memory pointed to by [ptr] pointer
parameters "nodes".  One of the primary datastructures used to manage these
nodes is a tree.  The elements of trees are commonly called "nodes".  
Therfore, whenever I think the possibility of confusion exists, I will call
the chunks passed in as parameters, "RPC nodes", and I will call the 
elements of the tree, "tree nodes".

This module provides routines and data structures for node management,
including alias detection and preservation, and deleted node
tracking (added for DCE V1.1).  In order to do this, some basic capabilities
are needed:
        Associating a node number with an address,
        Looking up a node by address,
        Looking up a node by node number,
        Tracking nodes which are deleted during a call, as opposed
        to simply being orphaned.

This code does not handle nodes which are partially overlapping.

The fundamental structure used in tracking nodes is named
"rpc_ss_private_node_table_t". It contains:
        "rpc_ss_ptr_array_p_t", a pointer to the root node in a tree
                used to convert from node numbers to node addresses,
        "rpc_ss_hash_entry_t", a hash table used to convert from node 
                addresses to node numbers
        "rpc_ss_deleted_nodes_t *", a pointer to a list of pointers
                to deleted nodes.
        several other support cells.



STRUCTURES USED TO MAP NODE NUMBERS TO NODE ADDRESSES

Historical note:

When I initially designed and wrote this module, I envisioned the 
structure used to map from node-number to node-address as a 
multi-dimensional array which added dimensions as it needed to store more.
In retrospect, I understand that it is simply an n-ary tree, which adds 
a new superior node and pushes down the previous tree as a child of the 
new (root) node.  Because people are more comfortable with trees that change 
topology than they are with arrays whose dimensionality changes, I will 
describe it as a tree in this commentary.  For maintainability, the module
might want to be updated at some point, changing the <mumble>_array names
into names that reflect the tree-ness of the structure (and changing the 
small amount of commentary already in the module).

End of historical note.

The defined constant "rpc_ss_node_array_size" defines n in n-ary for this 
tree.  In this discussion I will use rpc_ss_node_array_size == 3, which 
happens to be its value when using the internal debugging driver for this
module. 

Each node in the tree holds rpc_ss_node_array_size node addresses if a 
leaf node, or the same number of pointers to child nodes if not a leaf.
Each node is actually an array of unions of pointers to void and pointers
to tree nodes.  (Module type rpc_ss_ptr_array_t.)
When the primary node tracking structure (rpc_ss_pvt_node_table_t) is 
created in rpc_ss_node_table_init, it points to a tree consisting of a 
single node.

            +----------+
            |          |  rpc_ss_pvt_node_table_t
            +----------+
                    |
                    v
           +-----+-----+-----+
           |  1  |  2  |  3  |   rpc_ss_ptr_array_t
           +-----+-----+-----+

Each cell in the rpc_ss_ptr_array_t can point to a node, so this simple 
initial structure can suffice for the first 3 nodes that are marshalled.
(Actually, of course, the first rpc_ss_node_array_size nodes.)  When the
fourth (e.g.) node is marshalled, we are out of room, and need to expand
mapping structures.  We do this by inserting a new (root) node between the 
existing tree node and the rpc_ss_pvt_node_table_t structure.

            +----------+
            |          |  rpc_ss_pvt_node_table_t
            +----------+
                    |
                    v
           +-----+-----+-----+
           | 1-3 | 4-6 | 7-9 |   rpc_ss_ptr_array_t
           +-----+-----+-----+
              |
              v
     +-----+-----+-----+
     |  1  |  2  |  3  |   rpc_ss_ptr_array_t
     +-----+-----+-----+

Now, we can add a new node as a sibling to our original node, to map nodes
4 through 6, and eventually nodes 7 through 9.  Thus, with just one level
of indirection to the leaf nodes, we can handle (rpc_ss_node_array_size
squared) nodes.

It should be easy to see that when we need to process the 10th node in our
example we will again be out of room, and of course we expand the 
structure again by adding another level.  The structure now looks like this:


             +----------+
             |          |  rpc_ss_pvt_node_table_t
             +----------+
                     |
                     v
            +-----+-----+-----+
            | 1-9 |10-18|19-27|   rpc_ss_ptr_array_t
            +-----+-----+-----+
               |
               v
      +-----+-----+-----+
      | 1-3 | 4-6 | 7-9 |   rpc_ss_ptr_array_t
      +-----+-----+-----+
         |
         v
+-----+-----+-----+
|  1  |  2  |  3  |   rpc_ss_ptr_array_t
+-----+-----+-----+

It is important to note that this structure can be sparse.  Our discussion 
has used an example with node numbers incrementing from one, as indeed nodes
are numbered on an original call.  However, it is entirely possible in a 
chained call that the first node marshalled is node 27 (e.g.).  In this case,
only those tree nodes needed to map to node 27 are created.  The other cells 
point to NULL, until a node that requires another tree node is processed.


             +----------+
             |          |  rpc_ss_pvt_node_table_t
             +----------+
                     |
                     v
            +-----+-----+-----+
            | 1-9 |10-18|19-27|   rpc_ss_ptr_array_t
            +-----+-----+-----+
                           |
                           v
                  +-----+-----+-----+
                  |19-21|22-24|25-27|   rpc_ss_ptr_array_t
                  +-----+-----+-----+
                                 |
                                 v
                        +-----+-----+-----+
                        |  25 |  26 |  27 |   rpc_ss_ptr_array_t
                        +-----+-----+-----+

It is important to note that leaves are always at the same depth throughout
the tree, and that all intervening tree nodes at the same level map the same 
number of RPC nodes. (i.e., one level down, each tree node is capable of 
mapping 9 nodes.)


STRUCTURES USED TO MAP NODE ADDRESSES TO NODE NUMBERS

Well, after all that, we can map node numbers into addresses of nodes.
However, the RPC system is presented with addresses of user data to pass
to a remote system.  For that we need to map the other way.

We use a hash table that uses chaining to resolve conflicts.  The hash 
algorithm is: shift right 5 bits (to accommodate malloc, which generally 
returns chunks aligned to some boundary), and then masked to the size
of the hash table, currently 256 slots long.

The size of the table is arbitrary, but depends on two factors.  If it is 
smaller, we get more conflicts and realize less advantage from having
a hash table at all.  However, the hash table is simply an array of 
rpc_ss_hash_entry_t, whose size is 16 bytes on 32-bit address systems.
Since the entire array has to be nulled out during the init routine,
rpc_ss_init_node_table, we don't want it to be too large--some customers
(notably Sun Microsystems) have noticed the time it takes to zero this 
structure and asked us to reduce it.

The structure rpc_ss_hash_entry_t contains
        - a next pointer to implement a single-linked list for hash bucket
          conflict resolution
        - the node number
        - the node address
        - a flag that the node has been marshalled
        - a flag that the node has been unmarshalled
        - a pad to bring the actual size of the structure to the size
          returned by sizeof(rpc_ss_hash_entry_t) (=16)

COMMENTS FOR IMPROVING THE HASH TABLE

There are two fundamental problems with the current hash system:
        - it's slow to initialize
        - it's slow to use

This section describes changes to address these problems.

First and foremost, we need to implement a high speed hash table 
instrumentation system, so we can see the hash table utilization in 
practice.  Is the hash table too small?  Is the hash algorithm appropriate?
Are the chains too long?  We don't know!

Above, we talked about the tension between wanting to increase the size 
of the hash table, and wanting to decrease its size.  The hash table is 
currently an array of hash entries, allocating extra hash entries if it 
needs to chain as a result of a collision. So the first change:

Change the hash table contained in the rpc_ss_node_table_t into an array
of POINTERS to rpc_ss_hash_entry_t, rather than an array of the entries
themselves. This makes the array one fourth the size, and should cut
its initialization time by 4.  This also allows us to make the hash table
512 or 1024 entries if we find it necessary, without getting performance 
significantly worse than it is now (better, for the 512 case).

Second change: allocate 10, 50, 100... (choose appropriate n) hash entries 
at once from the allocator. Keep the address of the array of hash table 
entries and the index of the last one used in the rpc_ss_node_table_t.  Then
allocating a hash entry is simply a matter of bumping a count, until we need
another batch.  This does not need to happen at the creation of the node table,
and this array does not have to be zeroed at allocation. The node table can
be initialized such that the first hash entry allocation will result in a
chunk allocation with no extra code. These chunks do not have to be kept track 
of after all the entries in the chunk have been used, so they do not need to 
be chained.  Simply allocate a new one, point to it from the 
rpc_ss_node_table_t, and forget about the old one.

Third change: there are many times we need to scan the list of hash entries 
linked off the hash entry array.  Currently, this is a linear unordered list,
so the average walk length is 1/2 the length of the list if the entry is
found and the length of the list if the entry is not found.  Since every node
is looked up before it is inserted (to find out if it has to be inserted), we 
have a large miss rate (full length walks).  This code could be much faster
if the collision resolution were a [balanced] binary tree rather than a 
linked list.  There is currently binary tree code in compiler module 
nametbl.c, that could be adapted to this purpose.  The tree would have to be 
balanced occasionally, since there will likely be non-random insertion.
Note that if this change is made, the technique used to delete addresses
from the hash table needs to be changed.  Currently, there are several 
sites in the module that zero out the address field in the hash entry to
delete it.  That doesn't work in a binary tree, because the value is used
to navigate the tree.  I suggest that we use one of the two pad bytes to 
implement a "deleted" flag.  This means that the same value could be in the
tree several times, but that's OK.  Only one of them could be not "deleted".

END OF COMMENTS FOR IMPROVING THE HASH TABLE



ROUTINE DESCRIPTIONS FOR MANAGING THE NUMBER-TO-ADDRESS TREE

Routine rpc_ss_expand_array recursively adds enough superior tree nodes until
the tree is capable of mapping at least the node number passed in.  This is 
an internal support routine, called only by other routines in this module. 
It does not actually record any node information in the tree, it just makes 
the tree deep enough to accomodate the node with a specified number. 

The parameters are:
        array - actually a pointer to a pointer to the root node in the tree.
                In practice, this means that the address of a cell in the
                rpc_ss_pvt_node_table_t is passed in.  This has to be indirect
                because we will write the address of the new tree node in this
                cell.
        currently_mapped - The number of RPC nodes capable of being mapped
                by a tree of this depth. (Twenty seven in our overview
                example above.)  Note that this is NOT the number of nodes
                currently being managed by the structure.
        depth - The number of levels currently in the tree.
        num -   The number of the RPC node that we are preparing to map.
                When we are done, currently_mapped >= num.
        p_mem_h - a pointer to a memory handle for the run time's memory 
                management system.

The routine allocates a new tree node, pointed to by cell t_array, zeros
it out and points the zeroth element at the existing tree.  Then it places
the new node's address in the passed-in tree root pointer.  It updates the
depth and currently_mapped cells and finally checks that currently_mapped >=
num.  If not, it recurses. (Does that mean that the first call is a curse?)

This routine does not populate the subtree that will be needed to actually
manage the node about to be added.  That happens in the next routine, 
rpc_ss_register_node_num.


Routine rpc_ss_register_node_num is an internal support routine, called only 
by other routines in this module.  It actually places the node address 
information in the tree in a way that associates it with the nude's number.

The parameters are:
        root_array - A pointer to a pointer to the tree's root.
        currently_mapped - a pointer to the cell recording the number of
                RPC nodes mapped by the tree.
        root_depth - a pointer to the cell recording the depth of the tree.
        num - the numberof the node to be registered.
        ptr - a pointer to the RPC node to be registered.  This is the 
                value that will ultimately be stored in the tree.
        p_mem_h - a pointer to a memory handle for the run time's memory
                management system.

First, the routine checks that the tree is deep enough to
accommodate the node to be registered (=managed), and if not, it calls the 
previous routine, rpc_ss_expand_array.  One call is guaranteed to make
the tree deep enough, no matter what node number is passed in (unless an 
exception is raised due to lack of memory).

The routine then walks the tree to the leaf level in a loop, updating local
copies of array (tree root), depth and mapped (number of RPC nodes mapped by 
the (sub-) tree). It also uses the parameter num as a local, since it was 
passed by value resulting in a local copy already. Each time it drops a level
in the tree, it has to re-map the node number to be an offset in the range
of node numbers mapped by this portion of the tree.  (At the top level,
the node number is already a one-based offset into the range of node numbers
mapped, since at the top level, all the node numbers are mapped.) 
Let's look at an example.

Remember our early example of three levels, mapping node numbers from 1 - 27?
Here it is again:

             +----------+
             |          |  rpc_ss_pvt_node_table_t
             +----------+
                     |
                     v
            +-----+-----+-----+
            | 1-9 |10-18|19-27|   rpc_ss_ptr_array_t
            +-----+-----+-----+
               |
               v
      +-----+-----+-----+
      | 1-3 | 4-6 | 7-9 |   rpc_ss_ptr_array_t
      +-----+-----+-----+
         |
         v
+-----+-----+-----+
|  1  |  2  |  3  |   rpc_ss_ptr_array_t
+-----+-----+-----+

The address of the cell in the rpc_ss_pvt_node_table_t which points 
to the tree is passed in to the routine.  Let's say we are looking for node
six.

We have the following state:
        array = & of the root node
        depth = 3
        mapped = 27

First, we find the number of RPC nodes mapped by each tree node in the level 
below this one, so we can figure out which array cell in this level.

     mapped = mapped / rpc_ss_node_array_size; = 27 / 3 = 9

This lets us find the index to use, by using integer division:

     index = (num-1) / mapped; = 5 / 9 = 0

Then we update the node number (offset) to be an offset into the subtree we
are about to walk to, the subtree pointed to by the selected index at this 
level (0).

     num = num - (index * mapped); = 6 - ( 0 * 9 ) = 6;

OK, so in this example the first iteration didn't change the offset.  This 
will be true anytime we are walking the tree through the zeroth index.

Now we are ready to descend a level in the tree, but first we have to make 
sure it is there.  We check, and yes, the tree node mapping RPC nodes 1 - 9
is present. Therefore, we point our local cell "array" at the subtree, 
decrement depth and head back up to the top of the loop.

Current state:        
        array = & tree node that maps RPC nodes 1 - 9
        depth = 2
        mapped = 9
        num = 6

Go through the same machinations as before:
        mapped = mapped / rpc_ss_node_array_size; = 9 / 3 = 3
        index = (num-1) / mapped; = 5 / 3 = 1;
        num = num - (index * mapped); = 6 - (1*3) = 3

Again, we're ready to descend a level, and we check and find there is NO NODE!
So we allocate one, clear it out, and hook it into the node we are currently
working on.  The tree now looks like this:


                     +----------+
                     |          |  rpc_ss_pvt_node_table_t
                     +----------+
                             |
                             v
                    +-----+-----+-----+
                    | 1-9 |10-18|19-27|   rpc_ss_ptr_array_t
                    +-----+-----+-----+
                       |
                       v
              +-----+-----+-----+
              | 1-3 | 4-6 | 7-9 |   rpc_ss_ptr_array_t
              +-----+-----+-----+
                 |     |
                 v     v
+-----+-----+-----+   +-----+-----+-----+
|  1  |  2  |  3  |   |  4  |  5  |  6  |  rpc_ss_ptr_array_t (2 of 'em!)
+-----+-----+-----+   +-----+-----+-----+

After we hook in the new node, we point to it from our temp cell "array",
and once again decrement depth.

We pop back up to the top of the loop, with the current state:

        array = & of tree node mapping RPC nodes 4 - 6
        depth = 1
        mapped = 3
        num = 3

The loop terminates if depth <= 1 (it will never be < 1, unless there is a 
bug somewhere...), so we are done with the loop.

Now we simply index into our leaf tree node using the new value in num,
our offset into the node number range mapped in this node (3), and store
the parameter ptr, which is the address of the RPC node we wish to register.

While this description is rather long, it should be noted that in practice
this tree is very flat, therefore, we execute the loop a small number of 
times. The loop is executed (depth-1) times, and the tree stores 
rpc_ss_node_array_size to the depth power RPC nodes. At the time this is 
written (21-Mar-93), the production value for rpc_ss_node_array_size is 50,
so a tree with depth 3 stores 125,000 RPC nodes, and navigating this tree
requires just two trips through the loop.

These are the only two internal routines that manipulate the node tree.
The remaining routines that use the node tree are used by the RPC runtime
or stubs.

Routine rpc_ss_lookup_node_by_num takes a node number and returns the address
of the corresponding RPC node if the node has been registered, otherwise it 
returns NULL.  The parameters are:
        tab - an rpc_ss_node_table_t, really a pointer to void, which gets 
                mapped to a pointer to an rpc_ss_pvt_node_table_t, the real
                node table structure.
        num - the number of the node to be looked up.

First the routine makes two fast "no-brainer" checks--special conditions
that mean the pointer is NULL:

If the node number is zero, that indicates a NULL pointer was passed.
If the node number is greater than the tree currently maps, then the node
can't possibly be in the tree, so don't look for it.  Return NULL.

Now, we perform a tree walk exactly as described for routine 
rpc_ss_register_node_num, except that if we reach a child pointer that is 
NULL, we return NULL, rather than allocating a child node.
When we reach a leaf node (depth == 1), return the content of the leaf node
array indexed by the offset (number).

That is the last of the routines that ONLY manipulate the tree.  Now we
will look at the few routines that only manipulate the hash table.  Then we
will look at those that manipulate the two of them together.


ROUTINES THAT MANIPULATE THE HASH TABLE

Macro get_hash_chain_head implements the hash algorithm described above.


Routine rpc_ss_find_hash_entry takes two parameters:
        - str, the node table
        - ptr, the address whose hash entry we are trying to find

The routine finds the head of the hash chain for the address and then loops 
through the entries in the chain until it finds the match or the end of the 
chain. 

The interesting thing about this routine is that it always returns a hash
entry--but the returned entry may not match the input.  If this happens, it 
means the address is not registered in the hash table.

This routine is currently a significant performance bottleneck if there are 
a large (> 1000) number of nodes being passed in the interface, or if the 
hash distribution is not good (i.e., anytime there are a large number of 
hash conflicts).  This is because it performs a walk of a linear list, the 
chain used to resolve hash conflicts.  Currently we do not have any way
of gathering info about the length of the lists in actual use.  This is a
problem that needs to be resolved.


Well, that concludes the routines that manipulate ONLY the hash table.
The remainder of the routines in the module manipulate both.


Routine rpc_ss_register_node_by_num is the fundamental routine that registers
the correspondence between an RPC node's address and its node number.  Even 
though it is in the **EXTERNAL ROUTINES** section, it is an internal routine. 

MAINTENANCE NOTE***
Routine rpc_ss_register_node_by_num should be moved to the internal support
routines section.
END MAINTENANCE NOTE***

Its parameters are:
        tab -- The address of the node table
        num -- the node number, and
        ptr -- the node's address.

First, we make sure that the "high water mark" for node numbers is at least
as high as this one.  This assures us that we will not assign potentially 
overlapping node numbers during the call.  Then we place an entry in the 
hash table and in the node number tree.  The callers of this routine have
already made sure that the node is not already registered.

This is one of the places that would need to be changed if we were to make 
any of the changes to the hash table recommended above.


Routine rpc_ss_register_node is a marshalling assist routine.  It has four
parameters:
        tab -- the address of the node table
        ptr -- the address of the node being considered for marshalling
        marshalling -- a flag indicating that we are actually in the process
               of marshalling
        has_been_marshalled -- an [out] flag indicating that the node has
               already been marshalled in a previous call.

The routine returns as a long, the node number of the node, whether or not
it has been marshalled previously.

The first step is to find out if the node is already registered by calling 
rpc_ss_lookup_node_by_ptr.  If it has been, and if we are in the process 
of marshalling, then we look up whether it has already been marshalled.
Then we set the marshalled flag for this node to true.  (Currently, we only
do that if it is not already set to true, but it is more efficient to do it
unconditionally--should be changed in the future.)   Routine 
rpc_ss_lookup_node_by_ptr's description (below) describes a desireable 
change to make this routine more efficient.  Make sure you see it.
If the node was already registered, we're done... simply return the node 
number.

If it wasn't already registered, this is the first time we have seen this 
address (more accurately, this node...  we may have seen this address before 
when it represented another node that was deleted.  On deletion, its registry
was erased).  We increment the highest seen node number and use that as the  
number for this node, and call rpc_ss_register_node_by_num.  If we are in the 
process of marshalling, we mark the node as marshalled in the hash table 
entry, but as NOT already marshalled in the [out] parameter.  This means that
we will marshall it this time, but not subsequent times.  Then we return the
node number and exit.


Routine rpc_ss_unregister_node is called when a node is deleted to remove
the node number <-> address mapping.  It's parameters are:
        tab -- the address of the node table
        ptr -- the address of the node being deleted

The interesting fact for this routine is that we ONLY remove the address from
the hash table.  We do nothing with the node number tree. This is because we 
are  concerned about having a new node with the same address as a previous
(deleted) node NOT be a match.  The first time we see the new node, its 
address won't be in the hash table (since this routine deleted the former
node's registration), and therefore it will get a new node number.
If we add new capabilities (callbacks) in which we are concerned about 
getting a false match by node number, then we would have to revisit
this routine and also take care of the node number tree.

We remove the address from the hash table by simply NULLing our the address
field of the hash entry.  We don't bother unlinking and deallocating the 
entry.  This has an implication for the hash table if we make the chains
into a binary tree.  Since the tree is navigated by key value, we can't 
null out one of the key values.  We either have to really delete the node
from the tree, or we have to flag the tree node as being for a deleted RPC 
node.


Routine rpc_ss_lookup_node_by_ptr is used to map from a pointer to a node 
number. Its parameters are:
        tab -- the address of the node table
        ptr -- the address of the RPC node to look up

This routine operates  in a relatively straightforward way.  It performs a 
rpc_ss_find_hash_entry, and if the lookup succeeds, it pulls the node number
out of the hash entry.  If the lookup in the hash table fails, this routine 
returns node number 0, a reserved node number used to indicate the NULL 
pointer.

Here is the performance change mentioned above:  This routine is used by 
other routines in this module, and is followed by a call to 
rpc_ss_find_hash_entry--which this routine just did!  Therefore, a desired 
change would be to have an additional [out] parameter to pass back the
hash entry.  Before doing this, we need to find out if the stubs ever 
accessed this routine from generated code, or if it is used only by the 
stub support library.  If there is any possibility that there is stub code 
out there that references this routine, then we need to clone the routine
and modify the clone.


On to a group of three very similar routines.  They are 
        rpc_ss_return_pointer_to_node
        rpc_ss_lookup_pointer_to_node and
        rpc_ss_inquire_pointer_to_node

These routines are used in the unmarshalling process.  They take in a node
number and return a node address.  The distinction between the routines, and
particularly the usage of ...lookup_pointer_to_node are not well understood
by me.  Be VERY CAREFUL if you are mucking with these routines.

The last routine in this module, other than the stand alone test harness,
is rpc_ss_init_node_table.  Its parameters are:
        tab -- an [out] parameter, the address of the node table
        p_mem_h -- the address of the memory handle

This routine allocates a new node table and initializes it (mostly to 
zero/NULL).  It pre-expands the node number tree to be one level deep 
(assuming that we'll be marshalling at least one node--this doesn't really
have to be done now).
*/

/*
 * Define HASH_STATS to monitor hash table efficiency.
 * Define UNIT_TEST_ERNODTBL to build stand-alone for unit testing.
 * Define DEBUG to create a file containing most significant pointer alias events.
 */
#ifdef UNIT_TEST_ERNODTBL
#   define  DCETHREAD_RAISE(x)                puts("Error")
#   define  rpc_ss_mem_alloc(h, x)  (char *)malloc(x)
#   define  rpc_void_p_t            char *
#   define  byte_p_t                char *
#   define  rpc_ss_node_table_t     byte_p_t
#   define  rpc_ss_mem_handle       long
#   define  NULL                    0
#   define  idl_true                1
#   define  idl_false               0
#else
#   include <dce/idlddefs.h>
#   include <ndrmi.h>
#   include <ndrui.h>
#   include <lsysdep.h>
#endif

#if defined UNIT_TEST_ERNODTBL || defined DEBUG
#   include <stdio.h>
#endif

#ifdef PERFMON
#include <dce/idl_log.h>
#endif

#ifdef DEBUG
    static  int ALIAS_DEBUG = idl_false;
#   define  DTOPEN          if (ALIAS_DEBUG && !trace_fid) trace_fid = fopen("alias_trace.dmp", "w")
#   define  DTRACE(ARGS)    if (!trace_fid); else fprintf ARGS
    static  FILE            *trace_fid = NULL;
#else
#   define  DTOPEN          do {;} while (0) /* DTOPEN */
#   define  DTRACE(ARGS)    do {;} while (0) /* DTRACE */
#endif


/******************************************************************************/
/*                                                                            */
/*              Internal Data structures for node table                       */
/*                                                                            */
/******************************************************************************/

#ifdef UNIT_TEST_ERNODTBL
#define rpc_ss_node_array_size 3
#else
#define rpc_ss_node_array_size 50   /* The expansion factor.  With fewer    */
                                    /* than this nodes lookup is single     */
                                    /* index operation                      */
#endif

/*
 *  Definition of the chain of blocks of deleted nodes used for
 *  [reflect_deletions]
 */
#define RPC_SS_DELETED_NODES_SIZE 2048

typedef struct rpc_ss_deleted_nodes_t {
    struct rpc_ss_deleted_nodes_t *next;
    idl_ulong_int node_count;   /* Number of node numbers in this block */
    idl_ulong_int node_numbers[RPC_SS_DELETED_NODES_SIZE];
} rpc_ss_deleted_nodes_t;

/*
 * Entry that contains information about a node address.  These are hung in
 * a linked list off of the hash table.  The hash table is used to lookup a
 * node number give a node address.
 */
typedef struct rpc_ss_hash_entry_t {
    struct rpc_ss_hash_entry_t *next;   /* Single linked list of hash entries */
                                        /* rooted into the has array          */

    byte_p_t ptr;                       /* node address                       */

    idl_ulong_int node;                 /* node number for the address        */

    unsigned char marshalled;           /* True if node has been marshalled   */

    unsigned char unmarshalled;         /* True if node has been unmarshalled */

    short reserved;                     /* Pad out to an even number of bytes */
} rpc_ss_hash_entry_t;

/*
 *  Multilevel array which enables quick indexed lookup of a node address give
 *  a node number.  At the leaf level it contains the node address, at higher
 *  levels it points to another array. Note that the union defined here
 *  consists of a single pointer. The type of the pointer's target is
 *  determined by the union arm selected
 */
typedef union rpc_ss_ptr_array_element_t {
    byte_p_t void_ptr;                              /* node address         */

    union rpc_ss_ptr_array_element_t *array_ptr;    /* next level in array  */
} rpc_ss_ptr_array_element_t;


typedef rpc_ss_ptr_array_element_t rpc_ss_ptr_array_t [rpc_ss_node_array_size];

typedef rpc_ss_ptr_array_t * rpc_ss_ptr_array_p_t;

/*
 * Actual node table structure which is completely a self-contained description of
 * a node number <=> node address mapping.
 */
#define RPC_SS_NODE_HASH_TABLE_SIZE 256
               /* This value must be a power of 2, because of the way the
                    hash function is defined */

typedef struct rpc_ss_pvt_node_table_t {
    rpc_ss_ptr_array_p_t array;             /* multi level array used to    */
                                            /* lookup an address give a     */
                                            /* node number                  */

    rpc_ss_hash_entry_t hash_table[RPC_SS_NODE_HASH_TABLE_SIZE];
                                            /* Hash table used to lookup    */
                                            /* the node number associated   */
                                            /* with an address and          */
                                            /* information on               */
                                            /* marshalling/unmarshalling    */
                                            /* status                       */

    idl_ulong_int depth;                             /* Number of levels in the      */
                                            /* multi-level array            */

    unsigned long long currently_mapped;                  /* Number of entry currently    */
                                            /* available in the multi-level */
                                            /* array for storing mappings   */

    idl_ulong_int high_num;                          /* Highest node number used     */

    rpc_ss_mem_handle *mem_h;               /* Pointer to the mem handle    */
                                            /* used to free up local        */
                                            /* storage at the end of a call */

    rpc_ss_deleted_nodes_t *deletes_list;   /* Pointer to a chain of blocks */
                                            /* containing idents of deleted */
                                            /* nodes                        */

    idl_boolean deletes_reflected;          /* TRUE => list of deleted      */
                                            /* nodes is being maintained    */
} rpc_ss_pvt_node_table_t;

/******************************************************************************/
/*                                                                            */
/*                        Internal Utility Macros                             */
/*                                                                            */
/******************************************************************************/

/*
**++
** Find the head of the hash chain for a pointer address.
**--
*/
#define get_hash_chain_head(ghch_str,ghch_ptr) \
&(ghch_str->hash_table[(((unsigned long)ghch_ptr>>5) & (RPC_SS_NODE_HASH_TABLE_SIZE-1))])


/******************************************************************************/
/*                                                                            */
/*                        Internal Support Routines                           */
/*                                                                            */
/******************************************************************************/

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_expand_array
**
**          This internal support routine will take the specified array and
**          push it a level down in the multilevel array and allocate a new
**          higher level in the array.
**
**  FORMAL PARAMETERS:
**
**      array -- A segment of the multilevel array
**      currently_mapped -- Number of nodes mapped into the multilevel array
**      depth -- Number of level in the multilevel array
**      num -- Node number that will be inserted after the expand
**      p_mem_h -- Local memory management handle
**
**  RETURN VALUE:
**
**      None
**
**--
*/
static void rpc_ss_expand_array
(
    rpc_ss_ptr_array_p_t * array,
    unsigned long long *currently_mapped,
    idl_ulong_int *depth,
    idl_ulong_int num,
    rpc_ss_mem_handle * p_mem_h
)
{
    /*
    ** This routine is called when we need to make the node number to pointer
    ** mapping array another level (dimension) deeper. It allocates a new
    ** top-level array, and points to the previous top-level from the zeroth
    ** element.
    */
    rpc_ss_ptr_array_p_t t_array;

#ifdef PERFMON
    RPC_SS_EXPAND_ARRAY_N;
#endif

    t_array = (rpc_ss_ptr_array_p_t) rpc_ss_mem_alloc (
              p_mem_h, sizeof(rpc_ss_ptr_array_t));

    /*
    ** Clear out the new storage
    */
    memset( *t_array, 0, sizeof(rpc_ss_ptr_array_t));

    /*
    ** Add another level of indirection.
    */
    (*t_array)[0].array_ptr = (union rpc_ss_ptr_array_element_t*)*array;
    *array = t_array;

    /*
    ** We're now more deeply nested, and map more node numbers.
    */
    (*depth)++;
    *currently_mapped = *currently_mapped * rpc_ss_node_array_size;

    DTRACE((
        trace_fid, "Expanding: array=%p depth=%lu, mapped=%lu\n",
        array, (unsigned long) *depth, (unsigned long) *currently_mapped
    ));

    if (*currently_mapped < num)
        rpc_ss_expand_array (array, currently_mapped, depth, num, p_mem_h);
#ifdef PERFMON
    RPC_SS_EXPAND_ARRAY_X;
#endif

}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_register_node_num
**
**          This internal support routine expands the multilevel array to the
**          proper size to contain the specified node number and then 
**          inserts the specified node number and node address pair into the
**          node table.
**
**  FORMAL PARAMETERS:
**
**      root_array -- A segment of the multilevel array
**      currently_mapped -- Number of nodes mapped into the multilevel array
**      root_depth -- Number of level in the multilevel array
**      num -- Node number to be registered
**      ptr -- Node address to be registered
**      p_mem_h -- Memory handle to use for memory allocation when needed
**
**  RETURN VALUE:
**
**      None
**
**--
*/
static void rpc_ss_register_node_num
(
    rpc_ss_ptr_array_p_t *root_array,
    unsigned long long *currently_mapped,
    idl_ulong_int *root_depth,
    idl_ulong_int num,
    byte_p_t ptr,
    rpc_ss_mem_handle * p_mem_h
)
{
    rpc_ss_ptr_array_p_t array;
    rpc_ss_ptr_array_p_t t_array;
    unsigned long long mapped;
    idl_ulong_int index;
    idl_ulong_int depth;

#ifdef PERFMON
    RPC_SS_REGISTER_NODE_NUM_N;
#endif

    if (*currently_mapped < num)
        rpc_ss_expand_array (root_array, currently_mapped,
                             root_depth, num, p_mem_h);

    array = *root_array;
    depth = *root_depth;
    mapped = *currently_mapped;

    /* The logic in the following loop must parallel the logic of the
        corresponding loop in rpc_ss_lookup_node_by_num */
    while (depth > 1)
    {
        mapped = mapped / rpc_ss_node_array_size;
        index = (num-1) / mapped;
        num = num - (index * mapped);

        if (((*array)[index]).array_ptr == 0)
        {
            t_array = (rpc_ss_ptr_array_p_t) rpc_ss_mem_alloc (
                       p_mem_h, sizeof(rpc_ss_ptr_array_t));

            memset( *t_array, 0, sizeof(rpc_ss_ptr_array_t));

            ((*array)[index]).array_ptr =
                     (union rpc_ss_ptr_array_element_t*)t_array;
        }

        array = (rpc_ss_ptr_array_p_t)((*array)[index]).array_ptr;
        depth = depth - 1;
    }
    ((*array)[num-1]).void_ptr = ptr;
#ifdef PERFMON
    RPC_SS_REGISTER_NODE_NUM_X;
#endif

}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_find_hash_entry
**
**          This internal support routine returns the hash entry associated
**          with a node address.  If the specified node address is found in the
**          hash table, its hash entry is returned otherwise the last hash
**          entry in the list is returned.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      ptr -- Node address to lookup
**
**  RETURN VALUE:
**
**      pointer to hash entry (matching or last in list)
**
**--
*/
static rpc_ss_hash_entry_t *rpc_ss_find_hash_entry
(
    rpc_ss_pvt_node_table_t *str,
    byte_p_t ptr
)
{
    rpc_ss_hash_entry_t *hash_entry;

#ifdef PERFMON
    RPC_SS_FIND_HASH_ENTRY_N;
#endif

    hash_entry = get_hash_chain_head (str, ptr);

    while ((hash_entry->ptr != ptr) && (hash_entry->next))
        hash_entry = hash_entry->next;

#ifdef PERFMON
    RPC_SS_FIND_HASH_ENTRY_X;
#endif

    return hash_entry;
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_register_node_by_num
**
**          This routine registers the association between the specified node
**          number and node address.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      num -- Node number to be registerd
**      ptr -- Node address to be registered
**
**  RETURN VALUE:
**
**      None
**
**--
*/
static void rpc_ss_register_node_by_num
(
    rpc_ss_node_table_t tab,
    idl_ulong_int num,
    byte_p_t ptr
)
{
    rpc_ss_hash_entry_t *temp;
    rpc_ss_hash_entry_t *hash_entry;
    rpc_ss_pvt_node_table_t *str;

#ifdef PERFMON
    RPC_SS_REGISTER_NODE_BY_NUM_N;
#endif

    str = (rpc_ss_pvt_node_table_t*) tab;

    /*
    ** Keep track of the highest node number seen.
    */
    if (num > str->high_num) str->high_num = num;

    /*
    ** Place an entry in the pointer to node number table, a hash table.
    */
    hash_entry = get_hash_chain_head (str, ptr);

    if (hash_entry->node != 0)
    {
        temp = hash_entry;
        hash_entry = (rpc_ss_hash_entry_t*) rpc_ss_mem_alloc (str->mem_h, sizeof(rpc_ss_hash_entry_t));

        hash_entry->next = temp->next;
        temp->next = hash_entry;
    }

    hash_entry->node = num;
    hash_entry->ptr = ptr;
    hash_entry->marshalled = idl_false;
    hash_entry->unmarshalled = idl_false;


#ifdef HASH_STATS
    printf ("Hash value: %03d Hash chain:", (((unsigned long)ptr>>5) & 0xff));
    for (temp = get_hash_chain_head (str, ptr); temp; temp=temp->next)
        printf (" %lx", temp->ptr);
    printf ("\n");
#endif

    /*
    ** Place an entry in the node number to pointer table, a
    ** varying depth, multi-level array.
    */
    rpc_ss_register_node_num ( &str->array,
                               &str->currently_mapped,
                               &str->depth, num, ptr,
                               str->mem_h );
#ifdef PERFMON
    RPC_SS_REGISTER_NODE_BY_NUM_X;
#endif

}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_lookup_node_by_ptr
**
**          This routine returns the node number associated with a node
**          address.  If the specfied node address is not registered it returns
**          node number 0.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      ptr -- Node address to lookup
**      p_p_hash_entry -- [out] pointer to the hash entry in which the node
**                          number was found
**
**  RETURN VALUE:
**
**      Node number associated with address or 0 if no association
**
**--
*/
static idl_ulong_int rpc_ss_lookup_node_by_ptr
(
    rpc_ss_node_table_t tab,
    byte_p_t ptr,
    rpc_ss_hash_entry_t **p_p_hash_entry
)
{
    rpc_ss_hash_entry_t *hash_entry;
    idl_ulong_int node;
    rpc_ss_pvt_node_table_t * str;

#ifdef PERFMON
    RPC_SS_LOOKUP_NODE_BY_PTR_N;
#endif

    str = (rpc_ss_pvt_node_table_t *) tab;

    hash_entry = rpc_ss_find_hash_entry (str, ptr);

    if (hash_entry->ptr == ptr)
        node = hash_entry->node;
    else
        node = 0;

#ifdef PERFMON
    RPC_SS_LOOKUP_NODE_BY_PTR_X;
#endif

    *p_p_hash_entry = hash_entry;
    return node;
}

/******************************************************************************/
/*                                                                            */
/*  Add a node number to the list of deleted nodes                            */
/*  New routine for DCE V1.1                                                  */
/*                                                                            */
/******************************************************************************/
static void rpc_ss_add_delete_to_list
(
    idl_ulong_int node_number,
    rpc_ss_pvt_node_table_t *p_node_table
)
{
    rpc_ss_deleted_nodes_t *p_delete_block;

    if (p_node_table->deletes_list == NULL)
    {
        /* No delete list exists. Start the first block */
        p_delete_block = (rpc_ss_deleted_nodes_t *)rpc_ss_mem_alloc(
                                    p_node_table->mem_h,
                                    sizeof(rpc_ss_deleted_nodes_t));
        p_delete_block->next = NULL;
        p_delete_block->node_count = 0;
        p_node_table->deletes_list = p_delete_block;
    }
    else if (p_node_table->deletes_list->node_count==RPC_SS_DELETED_NODES_SIZE)
    {
        /* All existing blocks in the delete list are full.
            Add a new one to the head of the chain */
        p_delete_block = (rpc_ss_deleted_nodes_t *)rpc_ss_mem_alloc(
                                    p_node_table->mem_h,
                                    sizeof(rpc_ss_deleted_nodes_t));
        p_delete_block->next = p_node_table->deletes_list;
        p_delete_block->node_count = 0;
        p_node_table->deletes_list = p_delete_block;
    }
    else
    {
        /* Use the partly-filled block at the head of the list */
        p_delete_block = p_node_table->deletes_list;
    }

    p_delete_block->node_numbers[p_delete_block->node_count] = node_number;
    (p_delete_block->node_count)++;
}

/******************************************************************************/
/*                                                                            */
/*                            External Routines                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*  Enable [reflect_deletions] in the node table                              */
/*  New routine for DCE V1.1                                                  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_enable_reflect_deletes
(
    rpc_ss_node_table_t tab
)
{
    rpc_ss_pvt_node_table_t * str;

    str = (rpc_ss_pvt_node_table_t *) tab;
    str->deletes_reflected = idl_true;
}

/******************************************************************************/
/*                                                                            */
/*  Marshall a list of deleted nodes                                          */
/*  New routine for DCE V1.1                                                  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_marsh_deletes
(
    IDL_msp_t IDL_msp
)
{
    rpc_ss_pvt_node_table_t *p_node_table;
    idl_ulong_int delete_count = 0;
    rpc_ss_deleted_nodes_t *p_delete_block;

    p_node_table = (rpc_ss_pvt_node_table_t *)
                                            IDL_msp->IDL_mem_handle.node_table;

    if (p_node_table != NULL)
    {
        for (p_delete_block = p_node_table->deletes_list;
             p_delete_block != NULL;
             p_delete_block = p_delete_block->next)
        {
            delete_count += p_delete_block->node_count;
        }
    }

    /* Marshall the Z-value for the conformant array */
    IDL_MARSH_ULONG(&delete_count);
    if (delete_count == 0)
        return;

    /* Marshall the blocks of node numbers */
    for (p_delete_block = p_node_table->deletes_list;
         p_delete_block != NULL;
         p_delete_block = p_delete_block->next)
    {
#ifdef PACKED_SCALAR_ARRAYS
        rpc_ss_ndr_marsh_by_pointing(p_delete_block->node_count, 4,
                                     (rpc_void_p_t)p_delete_block->node_numbers,
                                     IDL_msp);
#else
        rpc_ss_ndr_marsh_by_looping(p_delete_block->node_count, IDL_DT_ULONG,
                                    (rpc_void_p_t)p_delete_block->node_numbers,
                                    4, 0, IDL_msp);
#endif
    }
}

/******************************************************************************/
/*                                                                            */
/*  Unmarshall a list of deleted nodes and delete the nodes                   */
/*  New routine for DCE V1.1                                                  */
/*                                                                            */
/******************************************************************************/
void rpc_ss_ndr_unmar_deletes
(
    IDL_msp_t IDL_msp
)
{
    idl_ulong_int delete_count;
    idl_ulong_int *delete_list;
    unsigned32 i;
    rpc_void_p_t node_to_delete;

    /* Get size of delete list */
    IDL_UNMAR_ULONG(&delete_count);
    if (delete_count == 0)
        return;

    /* Get the delete list */
    delete_list = (idl_ulong_int *)rpc_ss_mem_alloc(&IDL_msp->IDL_mem_handle,
                                    delete_count * sizeof(idl_ulong_int));
#ifdef PACKED_SCALAR_ARRAYS
    rpc_ss_ndr_unmar_by_copying(delete_count, 4, (rpc_void_p_t)delete_list,
                                IDL_msp);
#else
    rpc_ss_ndr_unmar_by_looping(delete_count, IDL_DT_ULONG, 
                                (rpc_void_p_t)delete_list, 4, 0, IDL_msp);
#endif

    /* Apply the deletes */
    for (i=0; i<delete_count; i++)
    {
        node_to_delete = (rpc_void_p_t)rpc_ss_lookup_node_by_num(
                                        IDL_msp->IDL_mem_handle.node_table,
                                        delete_list[i]);
        (*(IDL_msp->IDL_p_free))(node_to_delete);
    }
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_lookup_node_by_num
**
**          This routine returns the address of a node associated with the
**          specified node number.  If the node number is not yet registered, then
**          NULL is returned.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      num -- Node number to be looked up
**
**  RETURN VALUE:
**
**      node address or NULL if unknown
**
**--
*/
byte_p_t rpc_ss_lookup_node_by_num
(
    rpc_ss_node_table_t tab,
    idl_ulong_int num
)
{
    idl_ulong_int mapped;
    idl_ulong_int depth;
    idl_ulong_int index;
    rpc_ss_ptr_array_p_t array;
    rpc_ss_pvt_node_table_t * str;

#ifdef PERFMON
    RPC_SS_LOOKUP_NODE_BY_NUM_N;
#endif


    /* Node number 0 is reserved for NULL pointers */
    if (num == 0) 
    {

#ifdef PERFMON
        RPC_SS_LOOKUP_NODE_BY_NUM_X;
#endif

        return (byte_p_t) NULL;
    }

    str = (rpc_ss_pvt_node_table_t *) tab;
    mapped = str->currently_mapped;

    /* Make sure the table is large enough to do a lookup */
    if (mapped < num)
    {

#ifdef PERFMON
        RPC_SS_LOOKUP_NODE_BY_NUM_X;
#endif

        return (byte_p_t) NULL;
    }

    array = (str->array);
    depth = str->depth;

    /* The logic in the following loop must parallel the logic of the
        corresponding loop in rpc_ss_register_node_by_num */
    while (depth > 1)
    {
        mapped = mapped / rpc_ss_node_array_size;
        index = (num-1) / mapped;

        if (((*array)[index]).array_ptr == 0)
	{

#ifdef PERFMON
            RPC_SS_LOOKUP_NODE_BY_NUM_X;
#endif

            return (byte_p_t) NULL;
	}
        num = num - (index * mapped);
        array = (rpc_ss_ptr_array_p_t)((*array)[index]).array_ptr;
        depth = depth - 1;
    }

#ifdef PERFMON
    RPC_SS_LOOKUP_NODE_BY_NUM_X;
#endif

    return ((*array)[num-1]).void_ptr;
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_register_node
**
**          This routine returns the node number associated with the specified
**          node address.  If the node address was not yet registered, then the
**          next highest node number will be associated with the specified
**          address and returned.
**
**          The intended usage for this routine is during marshalling.  If the
**          marshalling flag is set then the routine will either return the
**          node number associated with this address (if it had previously been
**          registered) and set the has_been_marshalled flag true, or assign
**          a node number to this address and mark it as having been marshalled.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      ptr -- Node address
**      marshalling -- Flag that indicates that the node will be marshalled if necessary
**      has_been_marshalled -- [out] true if node has already been marshalled
**
**  RETURN VALUE:
**
**      node number associated with the specified address
**
**--
*/
idl_ulong_int rpc_ss_register_node
(
    rpc_ss_node_table_t tab,
    byte_p_t ptr,
    long marshalling,
    long *has_been_marshalled
)
{
    idl_ulong_int num;
    rpc_ss_pvt_node_table_t * str;
    rpc_ss_hash_entry_t *hash_entry;

#ifdef PERFMON
    RPC_SS_REGISTER_NODE_N;
#endif

    /*
    ** Return node number 0 for NULL
    */
    if (ptr == NULL)
    {

#ifdef PERFMON
        RPC_SS_REGISTER_NODE_X;
#endif

        return 0;
    }

    str = (rpc_ss_pvt_node_table_t *) tab;

    /*
    ** Find out if this node is already registered.  If so return its number.
    */
    if ((num = rpc_ss_lookup_node_by_ptr (tab, ptr, &hash_entry)) != 0)
    {
        if (marshalling)
        {
            *has_been_marshalled = hash_entry->marshalled;
            hash_entry->marshalled = idl_true;
        }


#ifdef PERFMON
    RPC_SS_REGISTER_NODE_X;
#endif

        return num;
    }

    /*
    ** Not already registered.  Bump the high count and use it.
    */
    num = ++(str->high_num);

    rpc_ss_register_node_by_num (tab, num, ptr);
    if (marshalling)
    {
        hash_entry = rpc_ss_find_hash_entry (str, ptr);
        hash_entry->marshalled = idl_true;
        *has_been_marshalled = idl_false;
    }

    DTRACE((
               trace_fid, "Register(%p): num=%lu, ptr=%p\n", str, (unsigned long) num, ptr
    ));

#ifdef PERFMON
    RPC_SS_REGISTER_NODE_X;
#endif

    return num;
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_unregister_node
**
**          This routine removes a node address to node number mapping from the
**          node table.  It is intended to be called when an address is freed
**          from a memory allocator.  If the addr is later returned in a future
**          call to the allocator, it will not be matched as an existing node
**          and thus will be mapped to a new node on the client.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      ptr -- Node address
**
**  RETURN VALUE:
**
**      None
**
**--
*/
void rpc_ss_unregister_node
(
    rpc_ss_node_table_t tab,
    byte_p_t ptr
)
{
    rpc_ss_pvt_node_table_t * str;
    rpc_ss_hash_entry_t *hash_entry;

#ifdef PERFMON
    RPC_SS_UNREGISTER_NODE_N;
#endif

    /*
    ** Return if NULL
    */
    if (ptr == NULL)
    {

#ifdef PERFMON
        RPC_SS_UNREGISTER_NODE_X;
#endif

        return;
    }

    str = (rpc_ss_pvt_node_table_t *) tab;

    /*
    ** Find the hash entry for the pointer, and delete it.
    */
    hash_entry = rpc_ss_find_hash_entry (str, ptr);
    if (hash_entry->ptr == ptr)
    {
        if (str->deletes_reflected)
        {
            /* Add the node to the list of deleted nodes */
            rpc_ss_add_delete_to_list(hash_entry->node, str);
        }
        /* Remove its entry from the hash table */
        hash_entry->ptr = NULL;
    }

    /*
     * No else clause; if they were not equal, it means that the pointer
     * was not registered.  No action.
     */

#ifdef PERFMON
    RPC_SS_UNREGISTER_NODE_X;
#endif

    return;
}

#if 0
This code is not currently needed, but is preserved in case we need
it in the future.
/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_replace_address
**
**          This routine replaces the address associated with a node number.
**          It is used to propagate the node number across the multiple
**          instances of memory associated with transmit_as and represent_as.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      oldptr -- Old Node address
**      newptr -- New Node address
**
**  RETURN VALUE:
**
**      None
**
**--
*/
void rpc_ss_replace_address
(
    rpc_ss_node_table_t tab,
    byte_p_t oldptr,
    byte_p_t newptr
)
{
    rpc_ss_pvt_node_table_t * str;
    rpc_ss_hash_entry_t *hash_entry;
    rpc_ss_hash_entry_t *new_hash_entry;

#ifdef PERFMON
    RPC_SS_REPLACE_ADDRESS_N;
#endif

    str = (rpc_ss_pvt_node_table_t *) tab;

    /*
    ** Return if the old address was NULL, a cheap test against a coding error.
    */
    if (oldptr == NULL)
    {

#ifdef PERFMON
        RPC_SS_REPLACE_ADDRESS_X;
#endif

        return;
    }

    /*
    ** Find the hash entry for the pointer, and delete it.
    */
    hash_entry = rpc_ss_find_hash_entry (str, oldptr);
    if (hash_entry->ptr == oldptr)
        hash_entry->ptr = NULL;
    else
    /*
    ** If the node lookup failed, this node was not registered.
    ** Don't do anything.
    */
    {
#ifdef PERFMON
    RPC_SS_REPLACE_ADDRESS_X;
#endif
        return;
    }

    /*
    ** Register the new mapping.
    */
    rpc_ss_register_node_by_num ( tab, hash_entry->node, newptr );

    /*
    ** Propagate the marshalled/unmarshalled flags.
    */
    new_hash_entry = rpc_ss_find_hash_entry (str, newptr);
    new_hash_entry->marshalled = hash_entry->marshalled;
    new_hash_entry->unmarshalled = hash_entry->unmarshalled;

#ifdef PERFMON
    RPC_SS_REPLACE_ADDRESS_X;
#endif

    return;
}
/*End of disabled code.*/
#endif


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_return_pointer_to_node
**
**          This routine is intended for use during unmarshalling.  It accepts
**          a node number and information regarding the size and required
**          allocator for the node.  If the node address associated with this
**          node number is already register, the node address is returned.  If
**          there is not yet an association with this node number, one is
**          created and the newly allocated node is returned.
**
**          Also returned is a flag indicating if the node has been
**          unmarshalled.  If the node has previously been unmarshalled, then
**          this indicates that the stub that it need not do it again.  Upon the
**          next call to this routine for this node number, the unmarshalled 
**          flag will be set to
**          true, thus after the first call to rpc_ss_return_pointer_to_node for
**          a specific node, the stub must unmarshall its value.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      num -- Node number to lookup
**      size -- Size of the node
**      p_allocate -- Address of allocation routine with the signiture of malloc
**      has_been_unmarshalled -- [out] indicates if unmarshalled yet
**      new_node -- Optional [out] parameter. When used indicates whether memory
**                  was allocated
**
**  RETURN VALUE:
**
**      Node address associated the specified node number
**
**--
*/
byte_p_t rpc_ss_return_pointer_to_node
(
    rpc_ss_node_table_t tab,
    idl_ulong_int       num,
    idl_ulong_int                size,
    rpc_void_p_t        (*p_allocate)(idl_size_t size),
    long                *has_been_unmarshalled,
    long                *new_node               /* NULL or addr of return flag */
)
{
    byte_p_t p;
    rpc_ss_pvt_node_table_t * str;
    rpc_ss_hash_entry_t * hash_entry;

#ifdef PERFMON
    RPC_SS_RETURN_POINTER_TO_NODE_N;
#endif

    str = (rpc_ss_pvt_node_table_t *)tab;
    p = rpc_ss_lookup_node_by_num (tab, num);

    if (p == NULL)
    {
        if (new_node != NULL) *new_node = (long)idl_true;
        if (!p_allocate)
        {
            p = rpc_ss_mem_alloc (str->mem_h, size);
        }
        else
        {
            if (size == 0) size = 1;
            p = (byte_p_t)(*p_allocate)(size);
        }
        if (p ==NULL)
            DCETHREAD_RAISE(rpc_x_no_memory);
        rpc_ss_register_node_by_num (tab, num, p);
    }
    else
        if (new_node != NULL) *new_node = (long)idl_false;


    /* Return unmarshalled flag */
    hash_entry = rpc_ss_find_hash_entry (str, p);
    *has_been_unmarshalled = hash_entry->unmarshalled;

    /* Mark as already unmarshalled to be returned in next call */
    hash_entry->unmarshalled = idl_true;

#ifdef PERFMON
    RPC_SS_RETURN_POINTER_TO_NODE_X;
#endif

    return p;
}


/*
**++
**  WARNING:
**      This routine is used only by stubs generated by the old (OSF) compiler.
**      Do not modify this routine unless you understand how old-style stubs
**      work
**
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_lookup_pointer_to_node
**
**          This routine is intended for use during unmarshalling.  If the node
**          address associated with specified node number is already
**          registered, the node address is returned.  This routine is required
**          for conformant objects because the size information can be
**          unmarshalled only the first time the object is encountered.  Thus
**          this routine is called before rpc_ss_return_pointer_to_node to
**          determine if a conformant type has been already unmarshalled.  If it
**          hasn't then the stub can unmarshall the size information and then
**          call rpc_ss_return_pointer_to_node to actually allocate and register
**          the node.
**
**          Also returned is a flag indicating if the node has been
**          unmarshalled.  If the node has previously been unmarshalled, then
**          this indicates that the stub that it need not do it again.  Upon
**          the next call to this routine, the unmarshalled flag will be set to
**          true, thus after the first call to rpc_ss_return_pointer_to_node
**          for a specific node, the stub must unmarshall its value.
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      num -- Node number to lookup
**      has_been_unmarshalled -- [out] indicates if unmarshalled yet
**
**  RETURN VALUE:
**
**      Node address associated the specified node number
**
**--
*/
byte_p_t rpc_ss_lookup_pointer_to_node
(
    rpc_ss_node_table_t tab,
    idl_ulong_int num,
    long *has_been_unmarshalled
)
{
    byte_p_t p;
    rpc_ss_pvt_node_table_t * str;
    rpc_ss_hash_entry_t * hash_entry;

#ifdef PERFMON
    RPC_SS_LOOKUP_POINTER_TO_NODE_N;
#endif

    p = rpc_ss_lookup_node_by_num (tab, num);

    if (p == NULL)
    {

#ifdef PERFMON
    RPC_SS_LOOKUP_POINTER_TO_NODE_X;
#endif

        *has_been_unmarshalled = false;
        return p;
    }

    /* Return unmarshalled flag */
    str = (rpc_ss_pvt_node_table_t *)tab;
    hash_entry = rpc_ss_find_hash_entry (str, p);
    *has_been_unmarshalled = hash_entry->unmarshalled;

    /* Mark as already unmarshalled to be returned in next call */
    hash_entry->unmarshalled = idl_true;

#ifdef PERFMON
    RPC_SS_LOOKUP_POINTER_TO_NODE_X;
#endif

    return p;
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_inquire_pointer_to_node
**
**          Same functionality as rpc_ss_lookup_pointer_to_node except that
**          the "unmarshalled" flag is not set on the table entry for the
**          node
**
**  FORMAL PARAMETERS:
**
**      tab -- Node table
**      num -- Node number to lookup
**      has_been_unmarshalled -- [out] indicates if unmarshalled yet
**
**  RETURN VALUE:
**
**      Node address associated the specified node number
**
**--
*/
byte_p_t rpc_ss_inquire_pointer_to_node
(
    rpc_ss_node_table_t tab,
    idl_ulong_int num,
    long *has_been_unmarshalled
)
{
    byte_p_t p;
    rpc_ss_pvt_node_table_t * str;
    rpc_ss_hash_entry_t * hash_entry;

#ifdef PERFMON
    RPC_SS_INQUIRE_POINTER_TO_NODE_N;
#endif

    p = rpc_ss_lookup_node_by_num (tab, num);

    if (p == NULL)
    {

#ifdef PERFMON
    RPC_SS_INQUIRE_POINTER_TO_NODE_X;
#endif

        *has_been_unmarshalled = false;
        return p;
    }

    /* Return unmarshalled flag */
    str = (rpc_ss_pvt_node_table_t *)tab;
    hash_entry = rpc_ss_find_hash_entry (str, p);
    *has_been_unmarshalled = hash_entry->unmarshalled;


#ifdef PERFMON
    RPC_SS_INQUIRE_POINTER_TO_NODE_X;
#endif

    return p;
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      rpc_ss_init_node_table
**
**          This routine initializes the specified node table to be empty.
**
**  FORMAL PARAMETERS:
**
**      tab -- [out] Node table
**      p_mem_h -- Address of the mem handle for local memory management
**
**--
*/
void rpc_ss_init_node_table
(
    volatile rpc_ss_node_table_t *p_node_str,
    rpc_ss_mem_handle *p_mem_h
)
{
/*
** Allocate a node table and initialize it.
*/
    rpc_ss_pvt_node_table_t *str;

#ifdef PERFMON
    RPC_SS_INIT_NODE_TABLE_N;
#endif

#ifdef VERBOSE
    printf ("Init'ing new node table.\n");
#endif
    str = (rpc_ss_pvt_node_table_t*) rpc_ss_mem_alloc (
        p_mem_h, sizeof (rpc_ss_pvt_node_table_t));

    memset (str, 0, sizeof (rpc_ss_pvt_node_table_t));

    str->mem_h = p_mem_h;
    str->currently_mapped = 1;
    rpc_ss_expand_array (&(str->array), &(str->currently_mapped),
        &(str->depth), 1, p_mem_h);
    /* The preceding memset includes the effect of
     *  str->deletes_list = NULL;
     *  str->deletes_reflected = idl_false;
     */
    p_mem_h->node_table = (rpc_ss_node_table_t) str;
    *p_node_str = (rpc_ss_node_table_t) str;

    DTOPEN;
    DTRACE((trace_fid,"\n\nTable (%p) Initialized\n",str));

#ifdef PERFMON
    RPC_SS_INIT_NODE_TABLE_X;
#endif

}


/****************************************************************************/
/*                                                                          */
/*                        Unit Testing support                              */
/*                                                                          */
/*     This section of code provides a main routine that can be used to     */
/*     test the major functions of this module independant of the rest IDL  */
/*     support code.                                                        */
/****************************************************************************/
#ifdef UNIT_TEST_ERNODTBL
rpc_ss_node_table_t  str;

main ()
{
idl_ulong_int node;
byte_p_t ptr;
idl_ulong_int high_node;
long has_been_marshalled;

rpc_ss_init_node_table( &str, 0);

printf ("Correct results are in parentheses.\n\n");

has_been_marshalled = 0;
node=rpc_ss_register_node (str, (char*)10, 1, &has_been_marshalled);
printf ("%d (1)", node);
if (has_been_marshalled) printf ("  Erroneously flagged as marshalled!");
printf ("\n");

has_been_marshalled = 0;
node=rpc_ss_register_node (str, (char*)40, 1, &has_been_marshalled);
printf ("%d (2)", node);
if (has_been_marshalled) printf ("  Erroneously flagged as marshalled!");
printf ("\n");

has_been_marshalled = 0;
node=rpc_ss_register_node (str, (char*)10, 1, &has_been_marshalled);
printf ("%d (1)", node);
if (!has_been_marshalled) printf ("  Erroneously flagged as not marshalled!");
printf ("\n");

has_been_marshalled = 0;
node=rpc_ss_register_node (str, (char*)20, 0, &has_been_marshalled);
printf ("%d (3)", node);
if (has_been_marshalled) printf ("  Erroneously flagged as marshalled!");
printf ("\n");

node=rpc_ss_register_node (str, (char*)30, 0, 0);
printf ("%d (4)\n", node);

rpc_ss_register_node_by_num (str, 17, 50);

node=rpc_ss_register_node (str, (char*)1064, 0, 0);
printf ("%d (18)\n", node);



node=rpc_ss_lookup_node_by_ptr (str, (char*)50);
printf ("%d (17)\n", node);
node=rpc_ss_lookup_node_by_ptr (str, (char*)10);
printf ("%d (1)\n", node);
node=rpc_ss_lookup_node_by_ptr (str, (char*)30);
printf ("%d (4)\n", node);
node=rpc_ss_lookup_node_by_ptr (str, (char*)20);
printf ("%d (3)\n", node);
node=rpc_ss_lookup_node_by_ptr (str, (char*)1064);
printf ("%d (18)\n", node);
node=rpc_ss_lookup_node_by_ptr (str, (char*)40);
printf ("%d (2)\n", node);

ptr=rpc_ss_lookup_node_by_num(str, 1);
printf ("%d (10)\n", ptr);
ptr=rpc_ss_lookup_node_by_num(str, 2);
printf ("%d (40)\n", ptr);
ptr=rpc_ss_lookup_node_by_num(str, 3);
printf ("%d (20)\n", ptr);
ptr=rpc_ss_lookup_node_by_num(str, 4);
printf ("%d (30)\n", ptr);
ptr=rpc_ss_lookup_node_by_num(str, 17);
printf ("%d (50)\n", ptr);
ptr=rpc_ss_lookup_node_by_num(str, 18);
printf ("%d (1064)\n", ptr);
ptr=rpc_ss_lookup_node_by_num(str, 99);
printf ("%d (0)\n", ptr);
ptr=rpc_ss_lookup_node_by_num(str, 0);
printf ("%d (0)\n", ptr);

ptr = rpc_ss_return_pointer_to_node (str, 2, 3, 0, &has_been_marshalled,
                                     (long *)NULL);
printf ("%d (40)", ptr);
if (has_been_marshalled) printf ("  Erroneously flagged as unmarshalled!");
printf ("\n");
ptr = rpc_ss_return_pointer_to_node (str, 2, 3, 0, &has_been_marshalled,
                                     (long *)NULL);
printf ("%d (40)", ptr);
if (!has_been_marshalled) printf ("  Erroneously flagged as not unmarshalled!");
printf ("\n");
ptr = rpc_ss_return_pointer_to_node (str, 12, 3, 0, &has_been_marshalled,
                                     (long *)NULL);
printf ("%08X (some reasonable heap pointer value)", ptr);
if (has_been_marshalled) printf ("  Erroneously flagged as marshalled!");
printf ("\n");

}
#endif
