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

#include "config.h"
#include "ctbase.h"
#include "cthash.h"

DWORD
CTStableHashTableInit(
    PSHASH_TABLE tab,
    LWGHashFunc hash_func,
    LWGEqualFunc equal_func,
    LWGDestroyNotify key_free,
    LWGDestroyNotify value_free
    )
{
    DWORD ceError = ERROR_SUCCESS;
    
    tab->table = lwg_hash_table_new(hash_func, equal_func);
    
    if (!tab->table)
    {
        ceError = ERROR_OUTOFMEMORY;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    tab->list = NULL;
    tab->key_free = key_free;
    tab->value_free = value_free;
    tab->dynamic = FALSE;
    
error:
    return ceError;
}

DWORD
CTStableHashTableNew(
    PSHASH_TABLE* tab,
    LWGHashFunc hash_func,
    LWGEqualFunc equal_func,
    LWGDestroyNotify key_free,
    LWGDestroyNotify value_free
    )
{
    DWORD ceError = ERROR_SUCCESS;
    
    ceError = CTAllocateMemory(sizeof(SHASH_TABLE), (PVOID*) tab);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = CTStableHashTableInit(*tab, hash_func, equal_func, key_free, value_free);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    (*tab)->dynamic = TRUE;
    
error:
    return ceError;
}

DWORD
CTStableHashTableInsert(
    PSHASH_TABLE tab,
    lwgpointer key,
    lwgpointer value
    )
{
    DWORD ceError = ERROR_SUCCESS;
    LWGList* old_link;
    if ((old_link = (LWGList*) lwg_hash_table_lookup(tab->table, key)) != NULL)
    {
        PSHASH_PAIR pair = (PSHASH_PAIR) old_link->data;
        lwg_hash_table_replace(tab->table, key, old_link);
        if (tab->value_free)
            tab->value_free(pair->value);
        if (tab->key_free)
            tab->key_free(pair->key);
        pair->value = value;
        pair->key = key;

    }
    else
    {
        PSHASH_PAIR pair = NULL;
        LWGList* link = NULL;

        ceError = CTAllocateMemory(sizeof(SHASH_PAIR), (PVOID*) &pair);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        pair->key = key;
        pair->value = value;
        
        tab->list = lwg_list_append(tab->list, pair);
        link = lwg_list_last(tab->list);
        lwg_hash_table_insert(tab->table, key, link);
    }

error:
    return ceError;
}

void
CTStableHashTableRemove(
    PSHASH_TABLE tab,
    lwgconstpointer key
    )
{
    LWGList* link = (LWGList*) lwg_hash_table_lookup(tab->table, key);
    
    if (link != NULL)
    {
        PSHASH_PAIR pair = (PSHASH_PAIR) link->data;
        if (tab->key_free)
            tab->key_free(pair->key);
        if (tab->value_free)
            tab->value_free(pair->value);
        CTFreeMemory(pair);
        tab->list = lwg_list_delete_link(tab->list, link);
        lwg_hash_table_remove(tab->table, key);
    }
}

lwgpointer
CTStableHashTableLookup(
    PSHASH_TABLE tab,
    lwgconstpointer key
    )
{
    LWGList* link = (LWGList*) lwg_hash_table_lookup(tab->table, key);
    
    if (link != NULL)
    {
        PSHASH_PAIR pair = (PSHASH_PAIR) link->data;
        return pair->value;
    }
    else
        return NULL;
}

void
CTStableHashTableFree(
    PSHASH_TABLE tab
    )
{
    LWGList* iter;
    
    for (iter = tab->list; iter != NULL; iter = iter->next)
    {
        PSHASH_PAIR pair = (PSHASH_PAIR) iter->data;
        if (pair) {
           if (tab->key_free)
              tab->key_free(pair->key);
           if (tab->value_free)
              tab->value_free(pair->value);
           CTFreeMemory(pair);
        }
    }
    
    lwg_list_free(tab->list);
    tab->list = NULL;

    lwg_hash_table_destroy(tab->table);
    tab->table = NULL;

    if (tab->dynamic)
        CTFreeMemory(tab);

}
