/*
Copyright (C) 2004, The Perl Foundation.
$Id$

=head1 NAME

src/global.c - Access to global PMCs

=head1 DESCRIPTION

tdb

=head2 Functions

tdb

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "global.str"

#define DEBUG_GLOBAL 0

/*

=item C<PMC *
Parrot_find_global(Parrot_Interp interpreter, STRING *class, STRING *globalname)>

Search the symbol C<globalname> in the given namespace name.
If C<class> is NULL search current namespace.

Return NULL if the global isn't found or the global.

=item C<PMC *
Parrot_get_global(Parrot_Interp interpreter, STRING *class, STRING *globalname)>

If the global exists, return it. If not either throw an exception or return a
C<Null> PMC depending on the interpreter's error settings.

=cut

*/

PMC *
Parrot_find_global(Parrot_Interp interpreter, STRING *class, STRING *globalname)
{
    PMC *globals, *stash, *res;
    
#if DEBUG_GLOBAL
    PIO_printf(interpreter, "find_global class '%Ss' meth '%Ss'\n",
            class, globalname);
#endif
    if (!class)
        stash = CONTEXT(interpreter->ctx)->current_namespace;
    else {
        globals = Parrot_get_ctx_HLL_namespace(interpreter);
        stash = VTABLE_get_pmc_keyed_str(interpreter, globals, class);
        if (PMC_IS_NULL(stash))
            return NULL;
    }
    res = VTABLE_get_pointer_keyed_str(interpreter, stash, globalname);
    if (PMC_IS_NULL(res))
        return NULL;
    return res;

}

PMC *
Parrot_find_global_p(Parrot_Interp interpreter, PMC *ns, STRING *name)
{

    PMC *globals, *res;

    if (PMC_IS_NULL(ns))
        return Parrot_find_global(interpreter, NULL, name);
    
    switch (ns->vtable->base_type) {
        case enum_class_String:
            return Parrot_find_global(interpreter, PMC_str_val(ns), name);
        case enum_class_Key:
            globals = Parrot_get_ctx_HLL_namespace(interpreter);
            ns = VTABLE_get_pmc_keyed(interpreter, globals, ns);
            if (PMC_IS_NULL(ns))
                return NULL;
            /* fall through */
        case enum_class_NameSpace:
            res = VTABLE_get_pointer_keyed_str(interpreter, ns, name);
            if (PMC_IS_NULL(res))
                return NULL;
            return res;
    }
    return NULL;
}

PMC *
Parrot_get_global(Parrot_Interp interpreter, STRING *class,
        STRING *name, void *next)
{
    PMC *g = Parrot_find_global(interpreter, class, name);
    if (g)
        return g;
    if (PARROT_ERRORS_test(interpreter, PARROT_ERRORS_GLOBALS_FLAG))  {
        real_exception(interpreter, next, E_NameError,
                "Global '%Ss' not found",
                name);
    }
    return PMCNULL;
}

PMC *
Parrot_get_global_p(Parrot_Interp interpreter, PMC *ns, STRING *name)
{
    PMC *g = Parrot_find_global_p(interpreter, ns, name);
    if (g)
        return g;
    if (PARROT_ERRORS_test(interpreter, PARROT_ERRORS_GLOBALS_FLAG))  {
        real_exception(interpreter, NULL, E_NameError,
                "Global '%Ss' not found",
                name);
    }
    return PMCNULL;
}

/*

=item C<PMC* Parrot_get_name(Interp* interpreter, STRING *name)>

Find the name in lexicals, current namespace,  globals, and builtins. If the
name isn't found throw and exception or return the Null PMC, depending on the
interpreter's errors setting.

=cut

*/


PMC *
Parrot_get_name(Interp* interpreter, STRING *name)
{
    PMC *g, *lex_pad, *namespace;
    parrot_context_t *ctx = CONTEXT(interpreter->ctx);

    lex_pad = Parrot_find_pad(interpreter, name, ctx);
    g = NULL;
    if (!PMC_IS_NULL(lex_pad)) {
        g = VTABLE_get_pmc_keyed_str(interpreter, lex_pad, name);
    }
    if (!g) {
        namespace = CONTEXT(interpreter->ctx)->current_namespace;
        g = Parrot_find_global_p(interpreter, namespace, name);
    }
    if (!g) {
        namespace = Parrot_get_ctx_HLL_namespace(interpreter);
        g = Parrot_find_global_p(interpreter, namespace, name);
    }
    if (!g)
        g = Parrot_find_builtin(interpreter, name);
    if (g) {
        if (g->vtable->base_type == enum_class_MultiSub &&
                interpreter->current_args) {
            /*
             * inside a function call, we have an args signature
             */
            g = Parrot_MMD_search_default_func(interpreter, name);
            if (g)
                return g;
        }
        else
            return g;
    }
    if (PARROT_ERRORS_test(interpreter, PARROT_ERRORS_GLOBALS_FLAG))  {
        real_exception(interpreter, NULL, E_NameError,
                "Name '%Ss' not found", name);
    }

    return PMCNULL;
}

/*

=item C<PMC* Parrot_global_namespace(Interp *, PMC *globals, STRING *ns)>

Return the stash hash of the given namespace in C<globals>.
If it doesn't exist yet, add it to the stash C<globals>.

=cut

*/

PMC*
Parrot_global_namespace(Interp *interpreter, PMC *globals, STRING *class)
{
    PMC *stash;

    stash = VTABLE_get_pmc_keyed_str(interpreter, globals, class);
    if (PMC_IS_NULL(stash) || 
            stash->vtable->base_type != enum_class_NameSpace) {
        stash = pmc_new(interpreter, enum_class_NameSpace);
        VTABLE_set_pmc_keyed_str(interpreter, globals, class, stash);
    }
    return stash;
}

/*

=item C<void
Parrot_store_global(Parrot_Interp, STRING *class, STRING *globalname, PMC *)>

Store the given PMC as global C<globalname> in the namespace C<class>. If
C<class> is NULL, the top-level global namespace is used.

=cut

*/

void
Parrot_store_global(Interp *interpreter, STRING *class,
        STRING *globalname, PMC *pmc)
{
    PMC *globals;
    PMC *stash;

    if (!class)
        stash = CONTEXT(interpreter->ctx)->current_namespace;
    else {
        globals = Parrot_get_ctx_HLL_namespace(interpreter);
        stash = Parrot_global_namespace(interpreter, globals, class);
    }
    VTABLE_set_pmc_keyed_str(interpreter, stash, globalname, pmc);
    Parrot_invalidate_method_cache(interpreter, class, globalname);
}

void
Parrot_store_global_p(Interp *interpreter, PMC *ns,
        STRING *globalname, PMC *pmc)
{
    PMC *globals;
    PMC *stash;

    if (PMC_IS_NULL(ns))
        Parrot_store_global(interpreter, NULL, globalname, pmc);
    
    switch (ns->vtable->base_type) {
        case enum_class_String:
            Parrot_store_global(interpreter, PMC_str_val(ns), globalname, pmc);
            break;
        case enum_class_Key:
            globals = Parrot_get_ctx_HLL_namespace(interpreter);
            stash = VTABLE_get_pmc_keyed(interpreter, globals, ns);
            if (PMC_IS_NULL(stash) || 
                stash->vtable->base_type != enum_class_NameSpace) {
                stash = pmc_new(interpreter, enum_class_NameSpace);
                VTABLE_set_pmc_keyed(interpreter, globals, ns, stash);
            }
            VTABLE_set_pmc_keyed_str(interpreter, stash, globalname, pmc);
            break;
        case enum_class_NameSpace:
            VTABLE_set_pmc_keyed_str(interpreter, ns, globalname, pmc);
    }
}

static void
store_sub(Interp *interpreter, STRING *class,
        STRING *globalname, PMC *pmc)
{
    PMC *stash;

    if (pmc->vtable->base_type == enum_class_MultiSub) {
        PMC *one_sub;

        one_sub = VTABLE_get_pmc_keyed_int(interpreter, pmc, 0);
        stash = Parrot_get_HLL_namespace(interpreter, PMC_sub(one_sub)->HLL_id);
    }
    else
        stash = Parrot_get_HLL_namespace(interpreter, PMC_sub(pmc)->HLL_id);

    if (class)
        stash = Parrot_global_namespace(interpreter, stash, class);

    VTABLE_set_pmc_keyed_str(interpreter, stash, globalname, pmc);
    Parrot_invalidate_method_cache(interpreter, class, globalname);
    /* MultiSub isa R*PMCArray and doesn't have a PMC_sub structure 
     * MultiSub could also contain subs from various namespaces,
     * so it doesn't make much sense to associate a namespace
     * with a multi.
     *
     */
    if (pmc->vtable->base_type != enum_class_MultiSub)
        PMC_sub(pmc)->namespace_stash = stash;
}

static void
store_sub_p(Interp *interpreter, PMC *namespace,
        STRING *globalname, PMC *pmc)
{
    PMC *globals;
    PMC *stash;

    globals = Parrot_get_ctx_HLL_namespace(interpreter);
    stash = VTABLE_get_pmc_keyed(interpreter, globals, namespace);
    if (PMC_IS_NULL(stash) || 
            stash->vtable->base_type != enum_class_NameSpace) {
        stash = pmc_new(interpreter, enum_class_NameSpace);
        VTABLE_set_pmc_keyed(interpreter, globals, namespace, stash);
    }
    VTABLE_set_pmc_keyed_str(interpreter, stash, globalname, pmc);
    
    /* MultiSub isa R*PMCArray and doesn't have a PMC_sub structure */
    if (pmc->vtable->base_type != enum_class_MultiSub)
        PMC_sub(pmc)->namespace_stash = stash;
}

static void
store_sub_in_namespace(Parrot_Interp interpreter, PMC* sub_pmc,
        PMC *namespace, STRING *sub_name)
{
    /*
     * namespace is either a String, or a Key, or NULL
     */
    if (PMC_IS_NULL(namespace))
        store_sub(interpreter, NULL, sub_name, sub_pmc);
    else {
        STRING *names;
        INTVAL type = namespace->vtable->base_type;
        switch (type) {
            case enum_class_String:
                names = PMC_str_val(namespace);
                store_sub(interpreter, names, sub_name, sub_pmc);
                break;
            case enum_class_Key:
                store_sub_p(interpreter, namespace, sub_name, sub_pmc);
                break;
            default:
                internal_exception(1, "Namespace constant is neither String nor Key");
        }

    }
}

/* XXX in mmd.c ? */
STRING* Parrot_multi_long_name(Parrot_Interp interpreter, PMC* sub_pmc);

STRING*
Parrot_multi_long_name(Parrot_Interp interpreter, PMC* sub_pmc)
{
    PMC *multi_sig;
    STRING* sub_name, *sig;
    INTVAL i, n;

    sub_name = PMC_sub(sub_pmc)->name;
    multi_sig = PMC_sub(sub_pmc)->multi_signature;
    n = VTABLE_elements(interpreter, multi_sig);
    /*
     * foo :multi(STRING, Integer) =>
     *
     * foo_@STRING_@Integer
     */
    for (i = 0; i < n; ++i) {
        sig = VTABLE_get_string_keyed_int(interpreter, multi_sig, i);
        sub_name = string_concat(interpreter, sub_name,
                const_string(interpreter, "_@"), 0);
        sub_name = string_concat(interpreter, sub_name, sig, 0);
    }
    return sub_name;
}

static void
store_named_in_namespace(Parrot_Interp interpreter, PMC* sub_pmc)
{
    STRING* sub_name;
    PMC *multi_sig;
    PMC *namespace;
    INTVAL func_nr;
    char *c_meth;

    sub_name  = PMC_sub(sub_pmc)->name;
    namespace = PMC_sub(sub_pmc)->namespace;
    multi_sig = PMC_sub(sub_pmc)->multi_signature;
    
    if (PMC_IS_NULL(multi_sig)) {
        store_sub_in_namespace(interpreter, sub_pmc, namespace, sub_name);
    }
    else {
        STRING *long_name;
        PMC *multi_sub;
        PMC *stash;
        
        /* If namespace is NULL, we need to look in the root HLL namespace. But
           since we haven't actually run code yet, the context hasn't been set
           to include the HLL, so we have to do the work ourselves. */
        if (PMC_IS_NULL(namespace))
        {
            stash = Parrot_get_HLL_namespace(interpreter, PMC_sub(sub_pmc)->HLL_id);
            multi_sub = VTABLE_get_pmc_keyed_str(interpreter, stash, sub_name);
        }
        else
            multi_sub = Parrot_find_global_p(interpreter, namespace, sub_name);
        
        /* is there an existing MultiSub PMC? or do we need to create one? */
        if (PMC_IS_NULL(multi_sub)) {
            multi_sub = pmc_new(interpreter, enum_class_MultiSub);
            /* we have to push the sub onto the MultiSub before we try to store
               it because storing requires information from the sub */
            VTABLE_push_pmc(interpreter, multi_sub, sub_pmc);
            store_sub_in_namespace(interpreter, multi_sub,
                    namespace, sub_name);
        }
        else
            VTABLE_push_pmc(interpreter, multi_sub, sub_pmc);
        
        long_name = Parrot_multi_long_name(interpreter, sub_pmc);
        store_sub_in_namespace(interpreter, sub_pmc, namespace, long_name);

        c_meth = string_to_cstring(interpreter, sub_name);
        if ( (func_nr = Parrot_MMD_method_idx(interpreter, c_meth))  >= 0) {
            Parrot_mmd_rebuild_table(interpreter, -1, func_nr);
        }
        string_cstring_free(c_meth);
    }
}

void
Parrot_store_sub_in_namespace(Parrot_Interp interpreter, PMC *sub)
{
    if (!(PObj_get_FLAGS(sub) & SUB_FLAG_PF_ANON)) {
        INTVAL cur_id = CONTEXT(interpreter->ctx)->current_HLL;
        /* PF structures aren't fully constructed yet */
        Parrot_block_DOD(interpreter);
        /* store relative to HLL namespace */
        CONTEXT(interpreter->ctx)->current_HLL = PMC_sub(sub)->HLL_id;
        
        store_named_in_namespace(interpreter, sub);
        
        /* restore HLL_id */
        CONTEXT(interpreter->ctx)->current_HLL = cur_id;
        Parrot_unblock_DOD(interpreter);
    }
    else {
        PMC *stash = Parrot_get_HLL_namespace(interpreter, PMC_sub(sub)->HLL_id);
        PMC_sub(sub)->namespace_stash = stash;
    }
}
/*

=back

=head1 SEE ALSO

F<include/parrot/global.h>

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
