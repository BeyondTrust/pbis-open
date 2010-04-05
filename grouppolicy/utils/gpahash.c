/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

CENTERROR
GPAStableHashTableInit(
    PGPASHASH_TABLE  tab,
    GPAHashFunc hash_func,
    GPAEqualFunc equal_func,
    GPADestroyNotify key_free,
    GPADestroyNotify value_free
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    tab->table = gpa_hash_table_new(hash_func, equal_func);
    
    if (!tab->table)
    {
        ceError = CENTERROR_OUT_OF_MEMORY;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    tab->list = NULL;
    tab->key_free = key_free;
    tab->value_free = value_free;
    tab->dynamic = FALSE;
    
error:
    return ceError;
}

CENTERROR
GPAStableHashTableNew(
    PGPASHASH_TABLE* tab,
    GPAHashFunc hash_func,
    GPAEqualFunc equal_func,
    GPADestroyNotify key_free,
    GPADestroyNotify value_free
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    ceError = LwAllocateMemory(sizeof(GPASHASH_TABLE), (PVOID*) tab);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAStableHashTableInit(*tab, hash_func, equal_func, key_free, value_free);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    (*tab)->dynamic = TRUE;
    
error:
    return ceError;
}

CENTERROR
GPAStableHashTableInsert(
    PGPASHASH_TABLE tab,
    gpapointer key,
    gpapointer value
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GPAList* old_link;
    if ((old_link = (GPAList*) gpa_hash_table_lookup(tab->table, key)) != NULL)
    {
        PGPASHASH_PAIR pair = (PGPASHASH_PAIR) old_link->data;
        gpa_hash_table_replace(tab->table, key, old_link);
        if (tab->value_free)
            tab->value_free(pair->value);
        if (tab->key_free)
            tab->key_free(pair->key);
        pair->value = value;
        pair->key = key;

    }
    else
    {
        PGPASHASH_PAIR pair = NULL;
        GPAList* link = NULL;

        ceError = LwAllocateMemory(sizeof(GPASHASH_PAIR), (PVOID*) &pair);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        pair->key = key;
        pair->value = value;
        
        tab->list = gpa_list_append(tab->list, pair);
        link = gpa_list_last(tab->list);
        gpa_hash_table_insert(tab->table, key, link);
    }

error:
    return ceError;
}

void
GPAStableHashTableRemove(
    PGPASHASH_TABLE tab,
    gpaconstpointer key
    )
{
    GPAList* link = (GPAList*) gpa_hash_table_lookup(tab->table, key);
    
    if (link != NULL)
    {
        PGPASHASH_PAIR pair = (PGPASHASH_PAIR) link->data;
        if (tab->key_free)
            tab->key_free(pair->key);
        if (tab->value_free)
            tab->value_free(pair->value);
        LwFreeMemory(pair);
        tab->list = gpa_list_delete_link(tab->list, link);
        gpa_hash_table_remove(tab->table, key);
    }
}

gpapointer
GPAStableHashTableLookup(
    PGPASHASH_TABLE tab,
    gpaconstpointer key
    )
{
    GPAList* link = (GPAList*) gpa_hash_table_lookup(tab->table, key);
    
    if (link != NULL)
    {
        PGPASHASH_PAIR pair = (PGPASHASH_PAIR) link->data;
        return pair->value;
    }
    else
        return NULL;
}

void
GPAStableHashTableFree(
    PGPASHASH_TABLE tab
    )
{
    GPAList* iter;
    
    for (iter = tab->list; iter != NULL; iter = iter->next)
    {
        PGPASHASH_PAIR pair = (PGPASHASH_PAIR) iter->data;
        if (pair) {
           if (tab->key_free)
              tab->key_free(pair->key);
           if (tab->value_free)
              tab->value_free(pair->value);
           LwFreeMemory(pair);
        }
    }
    
    gpa_list_free(tab->list);
    tab->list = NULL;

    gpa_hash_table_destroy(tab->table);
    tab->table = NULL;

    if (tab->dynamic)
        LwFreeMemory(tab);

}
