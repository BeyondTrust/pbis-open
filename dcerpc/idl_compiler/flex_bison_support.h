/*****************************************************************
 *
 * flex_bison_support.h
 *
 * GNU Bison/Flex support for DCE IDL compiler
 *
 *     added 07-12-97, Jim Doyle, Boston University, <jrd@bu.edu>
 *
 * Helper routines were added to each parser and lexxer pair
 * (nidl.y, nidl.l, acf.y, acf.l). The helper functions are
 * built with each parser and token generator. It is necessary
 * to define the helper functions directly in the .l and .y
 * files because the helper functions need to be able to access
 * and reference the static and global variables of the 
 * parser/tokenizer state machines. This scheme allows us to
 * to manipulate the context of each state machine when we
 * need to save, activate a new instance, and then restore
 * the state of a parser/tokenizer state machine.
 *
 * This feature is used by the FE_import_parse() routine, which
 * needs to be able flip states when managing an IMPORT (i.e.
 * file include) of an IDL file.
 *
 * The helper functions pass void * opaque references to the
 * context types which are dynamically allocated on the heap.
 *
 *****************************************************************/

/*
 * Function prototypes for helper routines to
 * manipulate multiple NIDL lexxer activation contexts
 *
 */

void * new_nidl_flexxer_activation_record(void);
void init_new_nidl_flexxer_activation(void);
void init_new_acf_flexxer_activation(void);

void   delete_nidl_flexxer_activation_record(void *);

void * get_current_nidl_flexxer_activation(void);

void   set_current_nidl_flexxer_activation(void *);

void   init_nidl_flexxer_activation(void);


/*
 * Function prototypes for helper routines to
 * manipulate multiple ACF lexxer activation contexts
 *
 */

void * new_acf_flexxer_activation_record(void);
void init_new_acf_flexxer_activation_record(void);

void   delete_acf_flexxer_activation_record(void *);

void * get_current_acf_flexxer_activation(void);

void   set_current_acf_flexxer_activation(void *);

void   init_acf_flexxer_activation(void);

/*
 * Function prototypes for helper routines to
 * manipulate multiple NIDL Bison parser activation contexts
 *
 */

void * new_nidl_bisonparser_activation_record(void);
void init_new_nidl_bisonparser_activation_record(void);
void init_new_nidl_bisonparser_activation(void);

void   delete_nidl_bisonparser_activation_record(void *);

void * get_current_nidl_bisonparser_activation(void);

void   set_current_nidl_bisonparser_activation(void *);

void   init_nidl_bisonparser_activation(void);

/*
 * Function prototypes for helper routines to
 * manipulate multiple ACF Bison parser activation contexts
 *
 */

void * new_acf_bisonparser_activation_record(void);
void  init_new_acf_bisonparser_activation_record(void);
void  init_new_acf_bisonparser_activation(void);

void   delete_acf_bisonparser_activation_record(void *);

void * get_current_acf_bisonparser_activation(void);

void   set_current_acf_bisonparser_activation(void *);

void   init_acf_bisonparser_activation(void);


