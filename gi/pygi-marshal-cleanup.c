/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */
 
 #include "pygi-marshal-cleanup.h"
 #include <glib.h>

void 
pygi_marshal_cleanup_args (PyGIInvokeState   *state,
                           PyGICallableCache *cache)
{
    switch (state->stage) {
        case PYGI_INVOKE_STAGE_MARSHAL_IN_IDLE:
            /* current_arg has been marshalled so increment to start with
               next arg */
            state->current_arg++;
        case PYGI_INVOKE_STAGE_MARSHAL_IN_START:
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_FAILED:
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_DONE:
        {
            gsize i;
            /* we have not yet invoked so we only need to clean up 
               the in args and out caller allocates */

            /* FIXME: handle caller allocates */
            for (i = 0; i < state->current_arg; i++) {
                PyGIArgCache *arg_cache = cache->args_cache[i];
                PyGIMarshalCleanupFunc cleanup_func = arg_cache->cleanup;
                if (cleanup_func != NULL  &&
                    arg_cache->direction != GI_DIRECTION_OUT) {
                    cleanup_func (state, arg_cache, state->args[i]->v_pointer);
                }
            }
            break;
        }
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_START:
            /* we have not yet marshalled so decrement to end with previous
               arg */
            state->current_arg--;
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE:
        case PYGI_INVOKE_STAGE_DONE:
            /* In args should have already been cleaned up so only cleanup
               out args */
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_DONE:
            break;
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_START:
            break;
    }
}

 void 
 _pygi_marshal_cleanup_gvalue (PyGIInvokeState *state,
                               PyGIArgCache    *arg_cache,
                               gpointer         data)
{
    /*
    if (arg_cache->direction == GI_DIRECTION_IN)
        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
    g_slice_free (GValue, data);
    */
}

void 
_pygi_marshal_cleanup_closure_unref (PyGIInvokeState *state,
                                     PyGIArgCache    *arg_cache,
                                     gpointer         data)
{
    g_closure_unref ( (GClosure *)data);
}

void
_pygi_marshal_cleanup_object_unref (PyGIInvokeState *state,
                                    PyGIArgCache    *arg_cache,
                                    gpointer         data)
{
    g_object_unref ( (GObject *)data);
}


void
_pygi_marshal_cleanup_utf8 (PyGIInvokeState *state,
                            PyGIArgCache    *arg_cache,
                            gpointer         data)
{
    /* For in or inout values before invoke we need to free this,
     * but after invoke we we free only if transfer == GI_TRANSFER_NOTHING
     * and this is not an inout value
     *
     * For out and inout values before invoke we do nothing but after invoke
     * we free if transfer == GI_TRANSFER_EVERYTHING
     */
    switch (state->stage) {
        PYGI_INVOKE_STAGE_MARSHAL_IN_START:
        PYGI_INVOKE_STAGE_MARSHAL_IN_IDLE:
            g_free (data);
            break;
        PYGI_INVOKE_STAGE_NATIVE_INVOKE_FAILED:
        PYGI_INVOKE_STAGE_NATIVE_INVOKE_DONE:
            if (arg_cache->transfer == GI_TRANSFER_NOTHING &&
                  arg_cache->direction == GI_DIRECTION_IN)
                g_free (data);
            break;
        PYGI_INVOKE_STAGE_MARSHAL_RETURN_START:
        PYGI_INVOKE_STAGE_MARSHAL_RETURN_DONE:
        PYGI_INVOKE_STAGE_MARSHAL_OUT_START:
        PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE:
            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
                g_free (data);
            break;
        default:
            break;
     }
}