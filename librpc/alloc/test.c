/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include <moonunit/moonunit.h>
#include <lwrpc/allocate.h>

#include <string.h>

/* Test that basic allocations work */
MU_TEST(Alloc, basic)
{
    void* mem = talloc(NULL, 123, NULL);
    unsigned long addr = (unsigned long) mem;
   
    MU_ASSERT(addr % 8 == 0);

    memset(mem, 0xFF, 123);

    tfree(mem);
}

/* Destructor that increments an integer to
   count total number of objects freed */
static void
count_destruct(void* data)
{
    int** d = (int**) data;

    (**d)++;

    tfree_children(data);
}

/* Calculates an exponent */
static int
int_pow(int base, int exp)
{
    int res = 1;

    for (; exp; exp--)
        res = res * base;

    return res;
}

/* Recursively builds and tears down a talloc tree:
 * parent: the root node
 * size: the size of each node
 * destruct: destructor
 * ply: number of children created at each level
 * depth: maximum depth of the tree
 * level_mod: if 0, nothing is freed, else children are freed when the current level % level_mod is 0
 * child_mod: only free children if their index % child_mod is 0
 */
static void
build_and_destroy_tree_mod(void* parent, int size, void (*destruct) (void*), int ply, int depth, int level_mod, int child_mod)
{
    if (depth > 0)
    {
        int i;
        void** children = malloc(sizeof (void*) * ply);
        for (i = 0; i < ply; i++)
        {
            children[i] = tdup(parent, parent, size, destruct);
            
            build_and_destroy_tree_mod(children[i], size, destruct, ply, depth-1, level_mod, child_mod);
        }

        if (level_mod && depth % level_mod == 0)
        {
            for (i = 0; i < ply; i++)
            {
                if (i % child_mod == 0)
                    tfree(children[i]);
            }
        }

        free(children);
    }
}

static void
generic_test(int ply, int depth, int level_mod, int child_mod)
{
    int count = ply == 1 ? depth + 1 : (int_pow(ply, depth + 1) - 1) / (ply - 1);
    int freed = 0;
    int** root = talloc(NULL, sizeof(int*), count_destruct);

    *root = &freed;

    build_and_destroy_tree_mod(root, sizeof(int*), count_destruct, ply, depth, 0, 0);
    tfree(root);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, freed, count);
}

MU_TEST(Alloc, stress)
{
    int ply, depth, level_mod, child_mod;

    for (ply = 1; ply <= 4; ply++)
        for (depth = 1; depth <= 8; depth++)
            for (level_mod = 0; level_mod <= depth; level_mod++)
                for (child_mod = 1; child_mod <= ply; child_mod++)
                {
                    MU_TRACE("Ply: %i Depth: %i Level mod: %i Child mod: %i",
                             ply, depth, level_mod, child_mod);
                    generic_test(ply, depth, level_mod, child_mod);
                }
}

MU_TEST(Alloc, unlink_children)
{
    int freed = 0;
    int** parent = talloc(NULL, sizeof(int*), count_destruct);
    int** child = talloc(parent, sizeof(int*), count_destruct);
    
    *parent = *child = &freed;

    tunlink_children(parent);

    tfree(parent);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, freed, 1);

    tfree(child);
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, freed, 2);
}
    
MU_TEST(Alloc, link)
{
    int freed = 0;

    int** parent = talloc(NULL, sizeof(int*), count_destruct);
    int** child = talloc(NULL, sizeof(int*), count_destruct);
    
    *parent = *child = &freed;

    tlink(parent, child);

    tfree(parent);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, freed, 2);
}
