/* GObject introspection: Function implementation
 *
 * Copyright (C) 2005 Matthias Clasen
 * Copyright (C) 2008,2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include <glib.h>

#include <girepository.h>
#include "girepository-private.h"
#include "gitypelib-internal.h"
#include "girffi.h"

/**
 * SECTION:gifunctioninfo
 * @Short_description: Struct representing a function
 * @Title: GIFunctionInfo
 *
 * GIFunctionInfo represents a function, method or constructor.
 * To find out what kind of entity a #GIFunctionInfo represents, call
 * g_function_info_get_flags().
 *
 * See also #GICallableInfo for information on how to retreive arguments and
 * other metadata.
 *
 * <refsect1 id="gi-gifunctioninfo.struct-hierarchy" role="struct_hierarchy">
 * <title role="struct_hierarchy.title">Struct hierarchy</title>
 * <synopsis>
 *   <link linkend="gi-GIBaseInfo">GIBaseInfo</link>
 *    +----<link linkend="gi-GICallableInfo">GICallableInfo</link>
 *          +----GIFunctionInfo
 *          +----<link linkend="gi-GISignalInfo">GISignalInfo</link>
 *          +----<link linkend="gi-GIVFuncInfo">GIVFuncInfo</link>
 * </synopsis>
 * </refsect1>
 */

GIFunctionInfo *
_g_base_info_find_method (GIBaseInfo   *base,
			  guint32       offset,
			  gint          n_methods,
			  const gchar  *name)
{
  /* FIXME hash */
  GIRealInfo *rinfo = (GIRealInfo*)base;
  Header *header = (Header *)rinfo->typelib->data;
  gint i;

  for (i = 0; i < n_methods; i++)
    {
      FunctionBlob *fblob = (FunctionBlob *)&rinfo->typelib->data[offset];
      const gchar *fname = (const gchar *)&rinfo->typelib->data[fblob->name];

      if (strcmp (name, fname) == 0)
        return (GIFunctionInfo *) g_info_new (GI_INFO_TYPE_FUNCTION, base,
			                      rinfo->typelib, offset);

      offset += header->function_blob_size;
    }

  return NULL;
}

/**
 * g_function_info_get_symbol:
 * @info: a #GIFunctionInfo
 *
 * Obtain the symbol of the function. The symbol is the name of the
 * exported function, suitable to be used as an argument to
 * g_module_symbol().
 *
 * Returns: the symbol
 */
const gchar *
g_function_info_get_symbol (GIFunctionInfo *info)
{
  GIRealInfo *rinfo;
  FunctionBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), NULL);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];

  return g_typelib_get_string (rinfo->typelib, blob->symbol);
}

/**
 * g_function_info_get_flags:
 * @info: a #GIFunctionInfo
 *
 * Obtain the #GIFunctionInfoFlags for the @info.
 *
 * Returns: the flags
 */
GIFunctionInfoFlags
g_function_info_get_flags (GIFunctionInfo *info)
{
  GIFunctionInfoFlags flags;
  GIRealInfo *rinfo;
  FunctionBlob *blob;

  g_return_val_if_fail (info != NULL, -1);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), -1);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];

  flags = 0;

  /* Make sure we don't flag Constructors as methods */
  if (!blob->constructor && !blob->is_static)
    flags = flags | GI_FUNCTION_IS_METHOD;

  if (blob->constructor)
    flags = flags | GI_FUNCTION_IS_CONSTRUCTOR;

  if (blob->getter)
    flags = flags | GI_FUNCTION_IS_GETTER;

  if (blob->setter)
    flags = flags | GI_FUNCTION_IS_SETTER;

  if (blob->wraps_vfunc)
    flags = flags | GI_FUNCTION_WRAPS_VFUNC;

  if (blob->throws)
    flags = flags | GI_FUNCTION_THROWS;

  return flags;
}

/**
 * g_function_info_get_property:
 * @info: a #GIFunctionInfo
 *
 * Obtain the property associated with this #GIFunctionInfo.
 * Only #GIFunctionInfo with the flag %GI_FUNCTION_IS_GETTER or
 * %GI_FUNCTION_IS_SETTER have a property set. For other cases,
 * %NULL will be returned.
 *
 * Returns: (transfer full): the property or %NULL if not set. Free it with
 * g_base_info_unref() when done.
 */
GIPropertyInfo *
g_function_info_get_property (GIFunctionInfo *info)
{
  GIRealInfo *rinfo;
  FunctionBlob *blob;
  GIInterfaceInfo *container;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), NULL);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];
  container = (GIInterfaceInfo *)rinfo->container;

  return g_interface_info_get_property (container, blob->index);
}

/**
 * g_function_info_get_vfunc:
 * @info: a #GIFunctionInfo
 *
 * Obtain the virtual function associated with this #GIFunctionInfo.
 * Only #GIFunctionInfo with the flag %GI_FUNCTION_WRAPS_VFUNC has
 * a virtual function set. For other cases, %NULL will be returned.
 *
 * Returns: (transfer full): the virtual function or %NULL if not set.
 * Free it by calling g_base_info_unref() when done.
 */
GIVFuncInfo *
g_function_info_get_vfunc (GIFunctionInfo *info)
{
  GIRealInfo *rinfo;
  FunctionBlob *blob;
  GIInterfaceInfo *container;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), NULL);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];
  container = (GIInterfaceInfo *)rinfo->container;

  return g_interface_info_get_vfunc (container, blob->index);
}

GQuark
g_invoke_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("g-invoke-error-quark");
  return quark;
}

/**
 * g_function_info_invoke: (skip)
 * @info: a #GIFunctionInfo describing the function to invoke
 * @in_args: an array of #GIArgument<!-- -->s, one for each in
 *    parameter of @info. If there are no in parameter, @in_args
 *    can be %NULL
 * @n_in_args: the length of the @in_args array
 * @out_args: an array of #GIArgument<!-- -->s, one for each out
 *    parameter of @info. If there are no out parameters, @out_args
 *    may be %NULL
 * @n_out_args: the length of the @out_args array
 * @return_value: return location for the return value of the
 *    function. If the function returns void, @return_value may be
 *    %NULL
 * @error: return location for detailed error information, or %NULL
 *
 * Invokes the function described in @info with the given
 * arguments. Note that inout parameters must appear in both
 * argument lists. This function uses dlsym() to obtain a pointer
 * to the function, so the library or shared object containing the
 * described function must either be linked to the caller, or must
 * have been g_module_symbol()<!-- -->ed before calling this function.
 *
 * Returns: %TRUE if the function has been invoked, %FALSE if an
 *   error occurred.
 */
gboolean
g_function_info_invoke (GIFunctionInfo *info,
			const GIArgument  *in_args,
			int               n_in_args,
			const GIArgument  *out_args,
			int               n_out_args,
			GIArgument        *return_value,
			GError          **error)
{
  ffi_cif cif;
  ffi_type *rtype;
  ffi_type **atypes;
  const gchar *symbol;
  gpointer func;
  GITypeInfo *tinfo;
  GIArgInfo *ainfo;
  gboolean is_method;
  gboolean throws;
  gint n_args, n_invoke_args, in_pos, out_pos, i;
  gpointer *args;
  gboolean success = FALSE;
  GError *local_error = NULL;
  gpointer error_address = &local_error;

  symbol = g_function_info_get_symbol (info);

  if (!g_typelib_symbol (g_base_info_get_typelib((GIBaseInfo *) info),
                         symbol, &func))
    {
      g_set_error (error,
                   G_INVOKE_ERROR,
                   G_INVOKE_ERROR_SYMBOL_NOT_FOUND,
                   "Could not locate %s: %s", symbol, g_module_error ());

      return FALSE;
    }

  is_method = (g_function_info_get_flags (info) & GI_FUNCTION_IS_METHOD) != 0
    && (g_function_info_get_flags (info) & GI_FUNCTION_IS_CONSTRUCTOR) == 0;
  throws = g_function_info_get_flags (info) & GI_FUNCTION_THROWS;

  tinfo = g_callable_info_get_return_type ((GICallableInfo *)info);
  rtype = g_type_info_get_ffi_type (tinfo);
  g_base_info_unref ((GIBaseInfo *)tinfo);

  in_pos = 0;
  out_pos = 0;

  n_args = g_callable_info_get_n_args ((GICallableInfo *)info);
  if (is_method)
    {
      if (n_in_args == 0)
	{
	  g_set_error (error,
		       G_INVOKE_ERROR,
		       G_INVOKE_ERROR_ARGUMENT_MISMATCH,
		       "Too few \"in\" arguments (handling this)");
	  goto out;
	}
      n_invoke_args = n_args+1;
      in_pos++;
    }
  else
    n_invoke_args = n_args;

  if (throws)
    /* Add an argument for the GError */
    n_invoke_args ++;

  atypes = g_alloca (sizeof (ffi_type*) * n_invoke_args);
  args = g_alloca (sizeof (gpointer) * n_invoke_args);

  if (is_method)
    {
      atypes[0] = &ffi_type_pointer;
      args[0] = (gpointer) &in_args[0];
    }
  for (i = 0; i < n_args; i++)
    {
      int offset = (is_method ? 1 : 0);
      ainfo = g_callable_info_get_arg ((GICallableInfo *)info, i);
      switch (g_arg_info_get_direction (ainfo))
	{
	case GI_DIRECTION_IN:
	  tinfo = g_arg_info_get_type (ainfo);
	  atypes[i+offset] = g_type_info_get_ffi_type (tinfo);
	  g_base_info_unref ((GIBaseInfo *)tinfo);

	  if (in_pos >= n_in_args)
	    {
	      g_set_error (error,
			   G_INVOKE_ERROR,
			   G_INVOKE_ERROR_ARGUMENT_MISMATCH,
			   "Too few \"in\" arguments (handling in)");
	      goto out;
	    }

	  args[i+offset] = (gpointer)&in_args[in_pos];
	  in_pos++;

	  break;
	case GI_DIRECTION_OUT:
	  atypes[i+offset] = &ffi_type_pointer;

	  if (out_pos >= n_out_args)
	    {
	      g_set_error (error,
			   G_INVOKE_ERROR,
			   G_INVOKE_ERROR_ARGUMENT_MISMATCH,
			   "Too few \"out\" arguments (handling out)");
	      goto out;
	    }

	  args[i+offset] = (gpointer)&out_args[out_pos];
	  out_pos++;
	  break;
	case GI_DIRECTION_INOUT:
	  atypes[i+offset] = &ffi_type_pointer;

	  if (in_pos >= n_in_args)
	    {
	      g_set_error (error,
			   G_INVOKE_ERROR,
			   G_INVOKE_ERROR_ARGUMENT_MISMATCH,
			   "Too few \"in\" arguments (handling inout)");
	      goto out;
	    }

	  if (out_pos >= n_out_args)
	    {
	      g_set_error (error,
			   G_INVOKE_ERROR,
			   G_INVOKE_ERROR_ARGUMENT_MISMATCH,
			   "Too few \"out\" arguments (handling inout)");
	      goto out;
	    }

	  args[i+offset] = (gpointer)&in_args[in_pos];
	  in_pos++;
	  out_pos++;
	  break;
	default:
	  g_assert_not_reached ();
	}
      g_base_info_unref ((GIBaseInfo *)ainfo);
    }

  if (throws)
    {
      args[n_invoke_args - 1] = &error_address;
      atypes[n_invoke_args - 1] = &ffi_type_pointer;
    }

  if (in_pos < n_in_args)
    {
      g_set_error (error,
		   G_INVOKE_ERROR,
		   G_INVOKE_ERROR_ARGUMENT_MISMATCH,
		   "Too many \"in\" arguments (at end)");
      goto out;
    }
  if (out_pos < n_out_args)
    {
      g_set_error (error,
		   G_INVOKE_ERROR,
		   G_INVOKE_ERROR_ARGUMENT_MISMATCH,
		   "Too many \"out\" arguments (at end)");
      goto out;
    }

  if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, n_invoke_args, rtype, atypes) != FFI_OK)
    goto out;

  g_return_val_if_fail (return_value, FALSE);
  ffi_call (&cif, func, return_value, args);

  if (local_error)
    {
      g_propagate_error (error, local_error);
      success = FALSE;
    }
  else
    {
      success = TRUE;
    }
 out:
  return success;
}
