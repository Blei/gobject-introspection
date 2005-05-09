/* GObject introspection: A parser for the XML IDL format
 *
 * Copyright (C) 2005 Matthias Clasen
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

#include <glib.h>
#include "gidlmodule.h"
#include "gidlnode.h"

typedef enum
{
  STATE_START,
  STATE_END,
  STATE_ROOT,
  STATE_NAMESPACE,
  STATE_FUNCTION,
  STATE_PARAMETERS,
  STATE_OBJECT,
  STATE_INTERFACE,
  STATE_IMPLEMENTS,
  STATE_REQUIRES,
  STATE_ENUM,
  STATE_BOXED,
  STATE_STRUCT,
  STATE_SIGNAL,
  STATE_ERRORDOMAIN
} ParseState;

typedef struct _ParseContext ParseContext;
struct _ParseContext
{
  ParseState state;
  ParseState prev_state;

  GList *modules;

  GIdlModule *current_module;
  GIdlNode *current_node;
};

#define MISSING_ATTRIBUTE(error,element,attribute)                            \
  g_set_error (error,                                                         \
	       G_MARKUP_ERROR,                                                \
	       G_MARKUP_ERROR_INVALID_CONTENT,                                \
	       "The attribute '%s' on the element '%s' must be specified",    \
	       attribute, element)

static const gchar *
find_attribute (const gchar  *name, 
		const gchar **attribute_names,
		const gchar **attribute_values)
{
  gint i;
  
  for (i = 0; attribute_names[i] != NULL; i++)
    if (strcmp (attribute_names[i], name) == 0)
      return attribute_values[i];
  
  return 0;
}

static GIdlNodeType *
parse_type_internal (gchar *str, gchar **rest)
{
  gint i;

  static struct {
    const gchar *str;
    gint tag;
    gboolean pointer;
  } basic[] = {
    { "void",     0, 0 },
    { "gpointer", 0, 1 },
    { "gboolean", 1, 0 },
    { "int8_t",   2, 0 },
    { "int8",     2, 0 },
    { "gint8",    2, 0 },
    { "uint8_t",  3, 0 },
    { "uint8",    3, 0 },
    { "guint8",   3, 0 },
    { "int16_t",  4, 0 },
    { "int16",    4, 0 },
    { "gint16",   4, 0 },
    { "uint16_t", 5, 0 },
    { "uint16",   5, 0 },
    { "guint16",  5, 0 },
    { "int32_t",  6, 0 },
    { "int32",    6, 0 },
    { "gint32",   6, 0 },
    { "uint32_t", 7, 0 },
    { "uint32",   7, 0 },
    { "guint32",  7, 0 },
    { "int64_t",  8, 0 },
    { "int64",    8, 0 },
    { "gint64",   8, 0 },
    { "uint64_t", 9, 0 },
    { "uint64",   9, 0 },
    { "guint64",  9, 0 },
    { "float",   10, 0 },
    { "gfloat",  10, 0 },
    { "double",  11, 0 },
    { "gdouble", 11, 0 },
    { "gchar",   12, 0 },  
    { "char",    12, 0 },
    { "GString", 13, 0 },
    { "int",     14, 0 },
    { "gint",    14, 0 },
    { "uint",    15, 0 },
    { "guint",   15, 0 },
    { "long",    16, 0 },
    { "glong",   16, 0 },
    { "ulong",   17, 0 },
    { "gulong",  17, 0 }
  };  

  gint n_basic = G_N_ELEMENTS (basic);
  gchar *start, *end;
  
  GIdlNodeType *type;
  
  type = (GIdlNodeType *)g_idl_node_new (G_IDL_NODE_TYPE);
  
  str = g_strstrip (str);

  type->unparsed = g_strdup (str);
 
  *rest = str;
  for (i = 0; i < n_basic; i++)
    {
      if (g_str_has_prefix (*rest, basic[i].str))
	{
	  type->is_basic = TRUE;
	  type->tag = basic[i].tag;
 	  type->is_pointer = basic[i].pointer;

	  *rest += strlen(basic[i].str);
	  *rest = g_strchug (*rest);
	  if (**rest == '*')
	    {
	      type->is_pointer = TRUE;
	      (*rest)++;
	    }

	  break;
	}
    }

  if (i < n_basic)
    /* found a basic type */;
  else if (g_str_has_prefix (*rest, "GList") ||
	   g_str_has_prefix (*rest, "GSList"))
    {
      if (g_str_has_prefix (*rest, "GList"))
	{
	  type->tag = 22;
	  type->is_glist = TRUE;
	  type->is_pointer = TRUE;
	  *rest += strlen ("GList");
	}
      else
	{
	  type->tag = 23;
	  type->is_gslist = TRUE;
	  type->is_pointer = TRUE;
	  *rest += strlen ("GSList");
	}
      
      *rest = g_strchug (*rest);
      
      if (**rest != '<')
	goto error;
      (*rest)++;
      
      type->parameter_type1 = parse_type_internal (*rest, rest);
      if (type->parameter_type1 == NULL)
	goto error;
      
      *rest = g_strchug (*rest);
      
      if ((*rest)[0] != '>')
	goto error;
      (*rest)++;
    }
  else if (g_str_has_prefix (*rest, "GHashTable"))
    {
      type->tag = 24;
      type->is_ghashtable = TRUE;
      type->is_pointer = TRUE;
      *rest += strlen ("GHashTable");
      
      *rest = g_strchug (*rest);
      
      if (**rest != '<')
	goto error;
      (*rest)++;
      
      type->parameter_type1 = parse_type_internal (*rest, rest);
      if (type->parameter_type1 == NULL)
	goto error;
      
      *rest = g_strchug (*rest);
      
      if ((*rest)[0] != ',')
	goto error;
      (*rest)++;
      
      type->parameter_type2 = parse_type_internal (*rest, rest);
      if (type->parameter_type2 == NULL)
	goto error;
      
      if ((*rest)[0] != '>')
	goto error;
      (*rest)++;
    }
  else if (g_str_has_prefix (*rest, "GError"))
    {
      type->tag = 25;
      type->is_error = TRUE;
      type->is_pointer = TRUE;
      *rest += strlen ("GError");
      
      *rest = g_strchug (*rest);
      
      if (**rest != '<')
	goto error;
      (*rest)++;
      
      end = strchr (*rest, '>');
      str = g_strndup (*rest, end - *rest);
      type->errors = g_strsplit (str, ",", 0);
      g_free (str);

      *rest = end + 1;
    }
  else 
    {
      type->tag = 21;
      type->is_interface = TRUE;
      start = *rest;

      /* must be an interface type */
      while (g_ascii_isalnum (**rest) || 
	     **rest == '.' || 
	     **rest == '-' || 
	     **rest == '_' ||
	     **rest == ':')
	(*rest)++;

      type->interface = g_strndup (start, *rest - start);

      *rest = g_strchug (*rest);
	  if (**rest == '*')
	    {
	      type->is_pointer = TRUE;
	      (*rest)++;
	    }
    }
  
  *rest = g_strchug (*rest);
  if (g_str_has_prefix (*rest, "["))
    {
      GIdlNodeType *array;

      array = (GIdlNodeType *)g_idl_node_new (G_IDL_NODE_TYPE);

      array->tag = 20;
      array->is_pointer = TRUE;
      array->is_array = TRUE;
      
      array->parameter_type1 = type;

      array->zero_terminated = FALSE;
      array->has_length = FALSE;
      array->length = 0;

      if (!g_str_has_prefix (*rest, "[]"))
	{
	  gchar *end, *str, **opts;
	  
	  end = strchr (*rest, ']');
	  str = g_strndup (*rest + 1, (end - *rest) - 1); 
	  opts = g_strsplit (str, ",", 0);
	  
	  *rest = end + 1;

	  for (i = 0; opts[i]; i++)
	    {
	      gchar **vals;
	      
	      vals = g_strsplit (opts[i], "=", 0);

	      if (strcmp (vals[0], "zero-terminated") == 0)
		array->zero_terminated = (strcmp (vals[1], "1") == 0);
	      else if (strcmp (vals[0], "length") == 0)
		{
		  array->has_length = TRUE;
		  array->length = atoi (vals[1]);
		}

	      g_strfreev (vals);
	    }

	  g_free (str);
	  g_strfreev (opts);
	}
	      
      type = array;
    }

  return type;

 error:
  g_idl_node_free ((GIdlNode *)type);
  
  return NULL;
}

static GIdlNodeType *
parse_type (const gchar *type)
{
  gchar *str;
  gchar *rest;
  GIdlNodeType *node;
  
  str = g_strdup (type);
  node = parse_type_internal (str, &rest);
  g_free (str);

  return node;
}

static gboolean
start_boxed (GMarkupParseContext *context,
	     const gchar         *element_name,
	     const gchar        **attribute_names,
	     const gchar        **attribute_values,
	     ParseContext        *ctx,
	     GError             **error)
{
  if (strcmp (element_name, "boxed") == 0 && 
      ctx->state == STATE_NAMESPACE)
    {
      const gchar *name;
      const gchar *cname;
      const gchar *typeinit;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      typeinit = find_attribute ("get-type", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else
	{
	  GIdlNodeBoxed *boxed;

	  boxed = (GIdlNodeBoxed *) g_idl_node_new (G_IDL_NODE_BOXED);
	  
	  ((GIdlNode *)boxed)->name = g_strdup (name);
	  boxed->c_name = g_strdup (cname);
	  boxed->init_func = g_strdup (typeinit);
	  if (deprecated && strcmp (deprecated, "1") == 0)
	    boxed->deprecated = TRUE;
	  else
	    boxed->deprecated = FALSE;
	  
	  ctx->current_node = (GIdlNode *)boxed;
	  ctx->current_module->entries = 
	    g_list_append (ctx->current_module->entries, boxed);
	  
	  ctx->state = STATE_BOXED;
	}
	  
      return TRUE;
    }

  return FALSE;
}

static gboolean
start_function (GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		ParseContext       *ctx,
		GError             **error)
{
  if ((ctx->state == STATE_NAMESPACE &&
       (strcmp (element_name, "function") == 0 ||
	strcmp (element_name, "callback") == 0)) ||
      ((ctx->state == STATE_OBJECT ||
	ctx->state == STATE_INTERFACE ||
	ctx->state == STATE_BOXED ||
	ctx->state == STATE_STRUCT) &&
       strcmp (element_name, "method") == 0) ||
      ((ctx->state == STATE_OBJECT ||
	ctx->state == STATE_BOXED) &&
       strcmp (element_name, "constructor") == 0))
    {
      const gchar *name;
      const gchar *cname;
      const gchar *deprecated;
      const gchar *type;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      type = find_attribute ("type", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (strcmp (element_name, "callback") != 0 && cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else
	{
	  GIdlNodeFunction *function;
	  
	  function = (GIdlNodeFunction *) g_idl_node_new (G_IDL_NODE_FUNCTION);

	  ((GIdlNode *)function)->name = g_strdup (name);
	  function->c_name = g_strdup (cname);
	  function->parameters = NULL;
	  if (deprecated && strcmp (deprecated, "1") == 0)
	    function->deprecated = TRUE;
	  else
	    function->deprecated = FALSE;
	
	  if (strcmp (element_name, "method") == 0 ||
	      strcmp (element_name, "constructor") == 0)
	    {
	      function->is_method = TRUE;

	      if (type && strcmp (type, "setter") == 0)
		function->is_setter = TRUE;
	      else if (type && strcmp (type, "getter") == 0)
		function->is_getter = TRUE;		  

	      if (strcmp (element_name, "constructor") == 0)
		function->is_constructor = TRUE;
	      else
		function->is_constructor = FALSE;
	    }
	  else
	    {
	      function->is_method = FALSE;
	      function->is_setter = FALSE;
	      function->is_getter = FALSE;
	      function->is_constructor = FALSE;
	      if (strcmp (element_name, "callback") == 0)
		((GIdlNode *)function)->type = G_IDL_NODE_CALLBACK;
	    }
	  
	  if (ctx->current_node == NULL)
	    {
	      ctx->current_module->entries = 
		g_list_append (ctx->current_module->entries, function);	      
	    }
	  else
	    switch (ctx->current_node->type)
	      {
	      case G_IDL_NODE_INTERFACE:
		{
		  GIdlNodeInterface *iface;
		  
		  iface = (GIdlNodeInterface *)ctx->current_node;
		  iface->members = g_list_append (iface->members, function);
		}
		break;
	      case G_IDL_NODE_BOXED:
		{
		  GIdlNodeBoxed *boxed;

		  boxed = (GIdlNodeBoxed *)ctx->current_node;
		  boxed->members = g_list_append (boxed->members, function);
		}
		break;
	      case G_IDL_NODE_STRUCT:
		{
		  GIdlNodeStruct *struct_;
		  
		  struct_ = (GIdlNodeStruct *)ctx->current_node;
		  struct_->members = g_list_append (struct_->members, function);		}
		break;
	      }
	  
	  ctx->current_node = (GIdlNode *)function;
	  ctx->state = STATE_FUNCTION;

	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
start_parameter (GMarkupParseContext *context,
		 const gchar         *element_name,
		 const gchar        **attribute_names,
		 const gchar        **attribute_values,
		 ParseContext       *ctx,
		 GError             **error)
{
  if (strcmp (element_name, "parameter") == 0 &&
      ctx->state == STATE_PARAMETERS)
    {
      const gchar *type;
      const gchar *name;
      const gchar *direction;
      const gchar *retval;
      const gchar *dipper;
      const gchar *optional;
      const gchar *nullok;
      const gchar *transfer;
      
      type = find_attribute ("type", attribute_names, attribute_values);
      name = find_attribute ("name", attribute_names, attribute_values);
      direction = find_attribute ("direction", attribute_names, attribute_values);
      retval = find_attribute ("retval", attribute_names, attribute_values);
      dipper = find_attribute ("dipper", attribute_names, attribute_values);
      optional = find_attribute ("optional", attribute_names, attribute_values);
      nullok = find_attribute ("null-ok", attribute_names, attribute_values);
      transfer = find_attribute ("transfer", attribute_names, attribute_values);

      if (type == NULL)
	MISSING_ATTRIBUTE (error, element_name, "type");
      else if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else
	{
	  GIdlNodeParam *param;

	  param = (GIdlNodeParam *)g_idl_node_new (G_IDL_NODE_PARAM);
	  
	  if (direction && strcmp (direction, "out") == 0)
	    {
	      param->in = FALSE;
	      param->out = TRUE;
	    }
	  else if (direction && strcmp (direction, "inout") == 0)
	    {
	      param->in = TRUE;
	      param->out = TRUE;
	    }
	  else
	    {
	      param->in = TRUE;
	      param->out = FALSE;
	    }

	  if (retval && strcmp (retval, "1") == 0)
	    param->retval = TRUE;
	  else
	    param->retval = FALSE;

	  if (dipper && strcmp (dipper, "1") == 0)
	    param->dipper = TRUE;
	  else
	    param->dipper = FALSE;

	  if (optional && strcmp (optional, "1") == 0)
	    param->optional = TRUE;
	  else
	    param->optional = FALSE;

	  if (nullok && strcmp (nullok, "1") == 0)
	    param->null_ok = TRUE;
	  else
	    param->null_ok = FALSE;

	  if (transfer && strcmp (transfer, "none") == 0)
	    {
	      param->transfer = FALSE;
	      param->shallow_transfer = FALSE;
	    }
	  else if (transfer && strcmp (transfer, "shallow") == 0)
	    {
	      param->transfer = FALSE;
	      param->shallow_transfer = TRUE;
	    }
	  else
	    {
	      param->transfer = TRUE;
	      param->shallow_transfer = FALSE;
	    }
	  
	  ((GIdlNode *)param)->name = g_strdup (name);
	  param->type = parse_type (type);
	  
	  switch (ctx->current_node->type)
	    {
	    case G_IDL_NODE_FUNCTION:
	    case G_IDL_NODE_CALLBACK:
	      {
		GIdlNodeFunction *func;

		func = (GIdlNodeFunction *)ctx->current_node;
		func->parameters = g_list_append (func->parameters, param);
	      }
	      break;
	    case G_IDL_NODE_SIGNAL:
	      {
		GIdlNodeSignal *signal;

		signal = (GIdlNodeSignal *)ctx->current_node;
		signal->parameters = g_list_append (signal->parameters, param);
	      }
	      break;
	    case G_IDL_NODE_VFUNC:
	      {
		GIdlNodeVFunc *vfunc;
		
		vfunc = (GIdlNodeVFunc *)ctx->current_node;
		vfunc->parameters = g_list_append (vfunc->parameters, param);
	      }
	      break;
	    }
	}

      return TRUE;
    }

  return FALSE;
}

static gboolean
start_field (GMarkupParseContext *context,
	     const gchar         *element_name,
	     const gchar        **attribute_names,
	     const gchar        **attribute_values,
	     ParseContext       *ctx,
	     GError             **error)
{
  if (strcmp (element_name, "field") == 0 &&
      (ctx->state == STATE_OBJECT ||
       ctx->state == STATE_BOXED ||
       ctx->state == STATE_STRUCT))
    {
      const gchar *cname;
      const gchar *type;
      const gchar *readable;
      const gchar *writable;
      const gchar *bits;
      
      cname = find_attribute ("cname", attribute_names, attribute_values);
      type = find_attribute ("type", attribute_names, attribute_values);
      readable = find_attribute ("readable", attribute_names, attribute_values);
      writable = find_attribute ("writable", attribute_names, attribute_values);
      bits = find_attribute ("bits", attribute_names, attribute_values);
      
      if (cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else if (type == NULL)
	MISSING_ATTRIBUTE (error, element_name, "type");
      else
	{
	  GIdlNodeField *field;

	  field = (GIdlNodeField *)g_idl_node_new (G_IDL_NODE_FIELD);
	  field->c_name = g_strdup (cname);
	  if (readable && strcmp (readable, "1") == 0)
	    field->readable = TRUE;
	  else
	    field->readable = FALSE;
	  
	  if (writable && strcmp (writable, "1") == 0)
	    field->writable = TRUE;
	  else
	    field->writable = FALSE;
	  
	  if (bits)
	    field->bits = atoi (bits);
	  else
	    field->bits = 0;
	  
	  field->type = parse_type (type);
	  
	  switch (ctx->current_node->type)
	    {
	    case G_IDL_NODE_INTERFACE:
	      {
		GIdlNodeInterface *iface;

		iface = (GIdlNodeInterface *)ctx->current_node;
		iface->members = g_list_append (iface->members, field);
	      }
	      break;
	    case G_IDL_NODE_BOXED:
	      {
		GIdlNodeBoxed *boxed;

		boxed = (GIdlNodeBoxed *)ctx->current_node;
		boxed->members = g_list_append (boxed->members, field);
	      }
	      break;
	    case G_IDL_NODE_STRUCT:
	      {
		GIdlNodeStruct *struct_;

		struct_ = (GIdlNodeStruct *)ctx->current_node;
		struct_->members = g_list_append (struct_->members, field);
	      }
	      break;
	    }
	}
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
start_enum (GMarkupParseContext *context,
	     const gchar         *element_name,
	     const gchar        **attribute_names,
	     const gchar        **attribute_values,
	     ParseContext       *ctx,
	     GError             **error)
{
  if ((strcmp (element_name, "enum") == 0 && ctx->state == STATE_NAMESPACE) ||
      (strcmp (element_name, "flags") == 0 && ctx->state == STATE_NAMESPACE))
    {
      const gchar *name;
      const gchar *cname;
      const gchar *type;
      const gchar *typeinit;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      typeinit = find_attribute ("get-type", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else 
	{	      
	  GIdlNodeEnum *enum_;
	  
	  if (strcmp (element_name, "enum") == 0)
	    enum_ = (GIdlNodeEnum *) g_idl_node_new (G_IDL_NODE_ENUM);
	  else
	    enum_ = (GIdlNodeEnum *) g_idl_node_new (G_IDL_NODE_FLAGS);
	  ((GIdlNode *)enum_)->name = g_strdup (name);
	  enum_->c_name = g_strdup (cname);
	  enum_->init_func = g_strdup (typeinit);
	  if (deprecated && strcmp (deprecated, "1") == 0)
	    enum_->deprecated = TRUE;
	  else
	    enum_->deprecated = FALSE;

	  ctx->current_node = (GIdlNode *) enum_;
	  ctx->current_module->entries = 
	    g_list_append (ctx->current_module->entries, enum_);	      
	  
	  ctx->state = STATE_ENUM;
	}
      
      return TRUE;
    }
  return FALSE;
}

static gboolean
start_property (GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		ParseContext       *ctx,
		GError             **error)
{
  if (strcmp (element_name, "property") == 0 &&
      (ctx->state == STATE_OBJECT ||
       ctx->state == STATE_INTERFACE))
    {
      const gchar *name;
      const gchar *cname;
      const gchar *type;
      const gchar *readable;
      const gchar *writable;
      const gchar *construct;
      const gchar *construct_only;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      type = find_attribute ("type", attribute_names, attribute_values);
      readable = find_attribute ("readable", attribute_names, attribute_values);
      writable = find_attribute ("writable", attribute_names, attribute_values);
      construct = find_attribute ("construct", attribute_names, attribute_values);
      construct_only = find_attribute ("construct-only", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (type == NULL)
	MISSING_ATTRIBUTE (error, element_name, "type");
      else 
	{	      
	  GIdlNodeProperty *property;
	  GIdlNodeInterface *iface;
	  
	  property = (GIdlNodeProperty *) g_idl_node_new (G_IDL_NODE_PROPERTY);

	  ((GIdlNode *)property)->name = g_strdup (name);
	  property->c_name = g_strdup (cname);
	  
	  if (readable && strcmp (readable, "1") == 0)
	    property->readable = TRUE;
	  else
	    property->readable = FALSE;
	  if (writable && strcmp (writable, "1") == 0)
	    property->writable = TRUE;
	  else
	    property->writable = FALSE;
	  if (construct && strcmp (construct, "1") == 0)
	    property->construct = TRUE;
	  else
	    property->construct = FALSE;
	  if (construct_only && strcmp (construct_only, "1") == 0)
	    property->construct_only = TRUE;
	  else
	    property->construct_only = FALSE;

	  property->type = parse_type (type);
	  
	  iface = (GIdlNodeInterface *)ctx->current_node;
	  iface->members = g_list_append (iface->members, property);
	}
      
      return TRUE;
    }
  return FALSE;
}

static gint
parse_value (const gchar *str)
{
  gchar *shift_op;
 
  /* FIXME just a quick hack */
  shift_op = strstr (str, "<<");

  if (shift_op)
    {
      gint base, shift;

      base = strtol (str, NULL, 10);
      shift = strtol (shift_op + 3, NULL, 10);
      
      return base << shift;
    }
  else
    return strtol (str, NULL, 10);

  return 0;
}

static gboolean
start_member (GMarkupParseContext *context,
	      const gchar         *element_name,
	      const gchar        **attribute_names,
	      const gchar        **attribute_values,
	      ParseContext       *ctx,
	      GError             **error)
{
  if (strcmp (element_name, "member") == 0 &&
      ctx->state == STATE_ENUM)
    {
      const gchar *name;
      const gchar *cname;
      const gchar *value;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      value = find_attribute ("value", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else 
	{	      
	  GIdlNodeEnum *enum_;
	  GIdlNodeValue *value_;

	  value_ = (GIdlNodeValue *) g_idl_node_new (G_IDL_NODE_VALUE);

	  ((GIdlNode *)value_)->name = g_strdup (name);
	  value_->c_name = g_strdup (cname);
	  
	  value_->value = parse_value (value);
	  
	  if (deprecated && strcmp (deprecated, "1") == 0)
	    value_->deprecated = TRUE;
	  else
	    value_->deprecated = FALSE;

	  enum_ = (GIdlNodeEnum *)ctx->current_node;
	  enum_->values = g_list_append (enum_->values, value_);
	}
      
      return TRUE;
    }
  return FALSE;
}

static gboolean
start_constant (GMarkupParseContext *context,
		const gchar         *element_name,
		const gchar        **attribute_names,
		const gchar        **attribute_values,
		ParseContext       *ctx,
		GError             **error)
{
  if (strcmp (element_name, "constant") == 0 &&
      (ctx->state == STATE_NAMESPACE ||
       ctx->state == STATE_OBJECT ||
       ctx->state == STATE_INTERFACE))
    {
      const gchar *name;
      const gchar *type;
      const gchar *value;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      type = find_attribute ("type", attribute_names, attribute_values);
      value = find_attribute ("value", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (type == NULL)
	MISSING_ATTRIBUTE (error, element_name, "type");
      else if (value == NULL)
	MISSING_ATTRIBUTE (error, element_name, "value");
      else 
	{	      
	  GIdlNodeConstant *constant;

	  constant = (GIdlNodeConstant *) g_idl_node_new (G_IDL_NODE_CONSTANT);

	  ((GIdlNode *)constant)->name = g_strdup (name);
	  constant->value = g_strdup (value);
	  
	  constant->type = parse_type (type);

	  if (deprecated && strcmp (deprecated, "1") == 0)
	    constant->deprecated = TRUE;
	  else
	    constant->deprecated = FALSE;

	  if (ctx->state == STATE_NAMESPACE)
	    {
	      ctx->current_node = (GIdlNode *) constant;
	      ctx->current_module->entries = 
		g_list_append (ctx->current_module->entries, constant);
	    }
	  else
	    {
	      GIdlNodeInterface *iface;

	      iface = (GIdlNodeInterface *)ctx->current_node;
	      iface->members = g_list_append (iface->members, constant);
	    }
	}
      
      return TRUE;
    }
  return FALSE;
}

static gboolean
start_errordomain (GMarkupParseContext *context,
		   const gchar         *element_name,
		   const gchar        **attribute_names,
		   const gchar        **attribute_values,
		   ParseContext       *ctx,
		   GError             **error)
{
  if (strcmp (element_name, "errordomain") == 0 &&
      ctx->state == STATE_NAMESPACE)
    {
      const gchar *name;
      const gchar *getquark;
      const gchar *codes;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      getquark = find_attribute ("get-quark", attribute_names, attribute_values);
      codes = find_attribute ("codes", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (getquark == NULL)
	MISSING_ATTRIBUTE (error, element_name, "getquark");
      else if (codes == NULL)
	MISSING_ATTRIBUTE (error, element_name, "codes");
      else 
	{	      
	  GIdlNodeErrorDomain *domain;

	  domain = (GIdlNodeErrorDomain *) g_idl_node_new (G_IDL_NODE_ERROR_DOMAIN);

	  ((GIdlNode *)domain)->name = g_strdup (name);
	  domain->getquark = g_strdup (getquark);
	  domain->codes = g_strdup (codes);

	  if (deprecated && strcmp (deprecated, "1") == 0)
	    domain->deprecated = TRUE;
	  else
	    domain->deprecated = FALSE;

	  ctx->current_node = (GIdlNode *) domain;
	  ctx->current_module->entries = 
	    g_list_append (ctx->current_module->entries, domain);

	  ctx->state = STATE_ERRORDOMAIN;
	}
      
      return TRUE;
    }
  return FALSE;
}

static gboolean
start_interface (GMarkupParseContext *context,
		 const gchar         *element_name,
		 const gchar        **attribute_names,
		 const gchar        **attribute_values,
		 ParseContext       *ctx,
		 GError             **error)
{
  if (strcmp (element_name, "interface") == 0 &&
      ctx->state == STATE_NAMESPACE)
    {
      const gchar *name;
      const gchar *cname;
      const gchar *typeinit;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      typeinit = find_attribute ("get-type", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else
	{
	  GIdlNodeInterface *iface;

	  iface = (GIdlNodeInterface *) g_idl_node_new (G_IDL_NODE_INTERFACE);
	  ((GIdlNode *)iface)->name = g_strdup (name);
	  iface->c_name = g_strdup (cname);
	  iface->init_func = g_strdup (typeinit);
	  if (deprecated && strcmp (deprecated, "1") == 0)
	    iface->deprecated = TRUE;
	  else
	    iface->deprecated = FALSE;
	  
	  ctx->current_node = (GIdlNode *) iface;
	  ctx->current_module->entries = 
	    g_list_append (ctx->current_module->entries, iface);	      
	  
	  ctx->state = STATE_INTERFACE;
	  
	}
      
      return TRUE;
    }
  return FALSE;
}

static gboolean
start_object (GMarkupParseContext *context,
	      const gchar         *element_name,
	      const gchar        **attribute_names,
	      const gchar        **attribute_values,
	      ParseContext       *ctx,
	      GError             **error)
{
  if (strcmp (element_name, "object") == 0 &&
      ctx->state == STATE_NAMESPACE)
    {
      const gchar *name;
      const gchar *cname;
      const gchar *parent;
      const gchar *typeinit;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      parent = find_attribute ("parent", attribute_names, attribute_values);
      typeinit = find_attribute ("get-type", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else
	{
	  GIdlNodeInterface *iface;

	  iface = (GIdlNodeInterface *) g_idl_node_new (G_IDL_NODE_OBJECT);
	  ((GIdlNode *)iface)->name = g_strdup (name);
	  iface->c_name = g_strdup (cname);
	  iface->init_func = g_strdup (typeinit);
	  iface->parent = g_strdup (parent);
	  if (deprecated && strcmp (deprecated, "1") == 0)
	    iface->deprecated = TRUE;
	  else
	    iface->deprecated = FALSE;
	  
	  ctx->current_node = (GIdlNode *) iface;
	  ctx->current_module->entries = 
	    g_list_append (ctx->current_module->entries, iface);	      
	  
	  ctx->state = STATE_OBJECT;
	}
      
      return TRUE;
    }
  return  FALSE;
}

static gboolean
start_return_type (GMarkupParseContext *context,
		   const gchar         *element_name,
		   const gchar        **attribute_names,
		   const gchar        **attribute_values,
		   ParseContext       *ctx,
		   GError             **error)
{
  if (strcmp (element_name, "return-type") == 0 &&
      ctx->state == STATE_FUNCTION)
    {
      const gchar *type;
      const gchar *nullok;
      const gchar *transfer;
      
      type = find_attribute ("type", attribute_names, attribute_values);
      nullok = find_attribute ("null-ok", attribute_names, attribute_values);
      transfer = find_attribute ("transfer", attribute_names, attribute_values);
      if (type == NULL)
	MISSING_ATTRIBUTE (error, element_name, "type");
      else
	{
	  GIdlNodeParam *param;

	  param = (GIdlNodeParam *)g_idl_node_new (G_IDL_NODE_PARAM);
	  param->in = FALSE;
	  param->out = FALSE;
	  param->retval = TRUE;
	  if (nullok && strcmp (nullok, "1") == 0)
	    param->null_ok = TRUE;
	  else
	    param->null_ok = FALSE;
	  if (transfer && strcmp (transfer, "none") == 0)
	    {
	      param->transfer = FALSE;
	      param->shallow_transfer = FALSE;
	    }
	  else if (transfer && strcmp (transfer, "shallow") == 0)
	    {
	      param->transfer = FALSE;
	      param->shallow_transfer = TRUE;
	    }
	  else
	    {
	      param->transfer = TRUE;
	      param->shallow_transfer = FALSE;
	    }
	  
	  param->type = parse_type (type);
	  
	  switch (ctx->current_node->type)
	    {
	    case G_IDL_NODE_FUNCTION:
	    case G_IDL_NODE_CALLBACK:
	      {
		GIdlNodeFunction *func = (GIdlNodeFunction *)ctx->current_node;
		func->result = param;
	      }
	      break;
	    case G_IDL_NODE_SIGNAL:
	      {
		GIdlNodeSignal *signal = (GIdlNodeSignal *)ctx->current_node;
		signal->result = param;
	      }
	      break;
	    case G_IDL_NODE_VFUNC:
	      {
		GIdlNodeVFunc *vfunc = (GIdlNodeVFunc *)ctx->current_node;
		vfunc->result = param;
	      }
	      break;
	    }
	}
      
      return TRUE;
    }

  return FALSE;
}

static gboolean
start_signal (GMarkupParseContext *context,
	      const gchar         *element_name,
	      const gchar        **attribute_names,
	      const gchar        **attribute_values,
	      ParseContext       *ctx,
	      GError             **error)
{
  if (strcmp (element_name, "signal") == 0 && 
      (ctx->state == STATE_OBJECT ||
       ctx->state == STATE_INTERFACE))
    {
      const gchar *name;
      const gchar *cname;
      const gchar *when;
      const gchar *no_recurse;
      const gchar *detailed;
      const gchar *action;
      const gchar *no_hooks;
      const gchar *has_class_closure;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      when = find_attribute ("when", attribute_names, attribute_values);
      no_recurse = find_attribute ("no-recurse", attribute_names, attribute_values);
      detailed = find_attribute ("detailed", attribute_names, attribute_values);
      action = find_attribute ("action", attribute_names, attribute_values);
      no_hooks = find_attribute ("no-hooks", attribute_names, attribute_values);
      has_class_closure = find_attribute ("has-class-closure", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (when == NULL)
	MISSING_ATTRIBUTE (error, element_name, "when");
      else
	{
	  GIdlNodeInterface *iface;
	  GIdlNodeSignal *signal;

	  signal = (GIdlNodeSignal *)g_idl_node_new (G_IDL_NODE_SIGNAL);
	  
	  ((GIdlNode *)signal)->name = g_strdup (name);
	  signal->c_name = g_strdup (cname);
	  
	  signal->run_first = FALSE;
	  signal->run_last = FALSE;
	  signal->run_cleanup = FALSE;
	  if (strcmp (when, "FIRST") == 0)
	    signal->run_first = TRUE;
	  else if (strcmp (when, "LAST") == 0)
	    signal->run_last = TRUE;
	  else 
	    signal->run_cleanup = TRUE;
	  
	  if (no_recurse && strcmp (no_recurse, "1") == 0)
	    signal->no_recurse = TRUE;
	  else
	    signal->no_recurse = FALSE;
	  if (detailed && strcmp (detailed, "1") == 0)
	    signal->detailed = TRUE;
	  else
	    signal->detailed = FALSE;
	  if (action && strcmp (action, "1") == 0)
	    signal->action = TRUE;
	  else
	    signal->action = FALSE;
	  if (no_hooks && strcmp (no_hooks, "1") == 0)
	    signal->no_hooks = TRUE;
	  else
	    signal->no_hooks = FALSE;
	  if (has_class_closure && strcmp (has_class_closure, "1") == 0)
	    signal->has_class_closure = TRUE;
	  else
	    signal->has_class_closure = FALSE;

	  iface = (GIdlNodeInterface *)ctx->current_node;
	  iface->members = g_list_append (iface->members, signal);

	  ctx->current_node = (GIdlNode *)signal;
	  ctx->state = STATE_FUNCTION;
	}
      
      return TRUE;
    }
  return FALSE;
}

static gboolean
start_vfunc (GMarkupParseContext *context,
	     const gchar         *element_name,
	     const gchar        **attribute_names,
	     const gchar        **attribute_values,
	     ParseContext       *ctx,
	     GError             **error)
{
  if (strcmp (element_name, "vfunc") == 0 && 
      (ctx->state == STATE_OBJECT ||
       ctx->state == STATE_INTERFACE))
    {
      const gchar *name;
      const gchar *cname;
      const gchar *must_chain_up;
      const gchar *override;
      const gchar *is_class_closure;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      must_chain_up = find_attribute ("must-chain-up", attribute_names, attribute_values);	  
      override = find_attribute ("override", attribute_names, attribute_values);
      is_class_closure = find_attribute ("is-class-closure", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else
	{
	  GIdlNodeInterface *iface;
	  GIdlNodeVFunc *vfunc;

	  vfunc = (GIdlNodeVFunc *)g_idl_node_new (G_IDL_NODE_VFUNC);
	  
	  ((GIdlNode *)vfunc)->name = g_strdup (name);
	  vfunc->c_name = g_strdup (cname);

	  if (must_chain_up && strcmp (must_chain_up, "1") == 0)
	    vfunc->must_chain_up = TRUE;
	  else
	    vfunc->must_chain_up = FALSE;

	  if (override && strcmp (override, "always") == 0)
	    {
	      vfunc->must_be_implemented = TRUE;
	      vfunc->must_not_be_implemented = FALSE;
	    }
	  else if (override && strcmp (override, "never") == 0)
	    {
	      vfunc->must_be_implemented = FALSE;
	      vfunc->must_not_be_implemented = TRUE;
	    }
	  else
	    {
	      vfunc->must_be_implemented = FALSE;
	      vfunc->must_not_be_implemented = FALSE;
	    }
	  
	  if (is_class_closure && strcmp (is_class_closure, "1") == 0)
	    vfunc->is_class_closure = TRUE;
	  else
	    vfunc->is_class_closure = FALSE;
	  
	  iface = (GIdlNodeInterface *)ctx->current_node;
	  iface->members = g_list_append (iface->members, vfunc);

	  ctx->current_node = (GIdlNode *)vfunc;
	  ctx->state = STATE_FUNCTION;
	}
      
      return TRUE;
    }
  return FALSE;
}


static gboolean
start_struct (GMarkupParseContext *context,
	      const gchar         *element_name,
	      const gchar        **attribute_names,
	      const gchar        **attribute_values,
	      ParseContext       *ctx,
	      GError             **error)
{
  if (strcmp (element_name, "struct") == 0 && 
      ctx->state == STATE_NAMESPACE)
    {
      const gchar *name;
      const gchar *cname;
      const gchar *deprecated;
      
      name = find_attribute ("name", attribute_names, attribute_values);
      cname = find_attribute ("cname", attribute_names, attribute_values);
      deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
      
      if (name == NULL)
	MISSING_ATTRIBUTE (error, element_name, "name");
      else if (cname == NULL)
	MISSING_ATTRIBUTE (error, element_name, "cname");
      else
	{
	  GIdlNodeStruct *struct_;

	  struct_ = (GIdlNodeStruct *) g_idl_node_new (G_IDL_NODE_STRUCT);
	  
	  ((GIdlNode *)struct_)->name = g_strdup (name);
	  struct_->c_name = g_strdup (cname);
	  if (deprecated && strcmp (deprecated, "1") == 0)
	    struct_->deprecated = TRUE;
	  else
	    struct_->deprecated = FALSE;

	  ctx->current_node = (GIdlNode *)struct_;
	  ctx->current_module->entries = 
	    g_list_append (ctx->current_module->entries, struct_);
	  
	  ctx->state = STATE_STRUCT;
	    }
      return TRUE;
    }
  return FALSE;
}
  
static void
start_element_handler (GMarkupParseContext *context,
		       const gchar         *element_name,
		       const gchar        **attribute_names,
		       const gchar        **attribute_values,
		       gpointer             user_data,
		       GError             **error)
{
  ParseContext *ctx = user_data;
  gint i, line_number, char_number;

  switch (element_name[0])
    {
    case 'a':
      if (strcmp (element_name, "api") == 0 && ctx->state == STATE_START)
	{
	  const gchar *version;

	  version = find_attribute ("version", attribute_names, attribute_values);
	  
	  if (version == NULL)
	    MISSING_ATTRIBUTE (error, element_name, "version");
	  else if (strcmp (version, "1.0") != 0)
	    g_set_error (error,
			 G_MARKUP_ERROR,
			 G_MARKUP_ERROR_INVALID_CONTENT,
			 "Unsupported version '%s'",
			 version);
	  else
	    ctx->state = STATE_ROOT;
	  
	  goto out;
	}
      break;

    case 'b':
      if (start_boxed (context, element_name,
		       attribute_names, attribute_values,
		       ctx, error))
	goto out;
      break;

    case 'c':
      if (start_function (context, element_name, 
			  attribute_names, attribute_values,
			  ctx, error))
	goto out;
      else if (start_constant (context, element_name,
			       attribute_names, attribute_values,
			       ctx, error))
	goto out;
      break;

    case 'e':
      if (start_enum (context, element_name, 
		      attribute_names, attribute_values,
		      ctx, error))
	goto out;
      else if (start_errordomain (context, element_name, 
		      attribute_names, attribute_values,
		      ctx, error))
	goto out;
      break;

    case 'f':
      if (start_function (context, element_name, 
			  attribute_names, attribute_values,
			  ctx, error))
	goto out;
      else if (start_field (context, element_name, 
			    attribute_names, attribute_values,
			    ctx, error))
	goto out;
      else if (start_enum (context, element_name, 
			   attribute_names, attribute_values,
			   ctx, error))
	goto out;
      
      break;

    case 'i':
      if (start_interface (context, element_name, 
			   attribute_names, attribute_values,
			   ctx, error))
	goto out;
      if (strcmp (element_name, "implements") == 0 &&
	  ctx->state == STATE_OBJECT)
	{
	  ctx->state = STATE_IMPLEMENTS;

	  goto out;
	}
      else if (strcmp (element_name, "interface") == 0 &&
	       ctx->state == STATE_IMPLEMENTS)
	{
	  const gchar *cname;

	  cname = find_attribute ("name", attribute_names, attribute_values);

	  if (cname == NULL)
	    MISSING_ATTRIBUTE (error, element_name, "name");
	  else
	    {  
	      GIdlNodeInterface *iface;

	      iface = (GIdlNodeInterface *)ctx->current_node;
	      iface ->interfaces = g_list_append (iface->interfaces, g_strdup (cname));
	    }

	  goto out;
	}
      else if (strcmp (element_name, "interface") == 0 &&
	       ctx->state == STATE_REQUIRES)
	{
	  const gchar *cname;

	  cname = find_attribute ("name", attribute_names, attribute_values);

	  if (cname == NULL)
	    MISSING_ATTRIBUTE (error, element_name, "name");
	  else
	    {  
	      GIdlNodeInterface *iface;

	      iface = (GIdlNodeInterface *)ctx->current_node;
	      iface ->prerequisites = g_list_append (iface->prerequisites, g_strdup (cname));
	    }

	  goto out;
	}
      break;

    case 'm':
      if (start_function (context, element_name, 
			  attribute_names, attribute_values,
			  ctx, error))
	goto out;
      else if (start_member (context, element_name, 
			  attribute_names, attribute_values,
			  ctx, error))
	goto out;
      break;

    case 'n':
      if (strcmp (element_name, "namespace") == 0 && ctx->state == STATE_ROOT)
	{
	  const gchar *name;
	  
	  name = find_attribute ("name", attribute_names, attribute_values);

	  if (name == NULL)
	    MISSING_ATTRIBUTE (error, element_name, "name");
	  else
	    {
	      ctx->current_module = g_idl_module_new (name);
	      ctx->modules = g_list_append (ctx->modules, ctx->current_module);

	      ctx->state = STATE_NAMESPACE;
	    }

	  goto out;
	}
      break;

    case 'o':
      if (start_object (context, element_name, 
			attribute_names, attribute_values,
			ctx, error))
	goto out;
      else if (strcmp (element_name, "object") == 0 &&
	       ctx->state == STATE_REQUIRES)
	{
	  const gchar *cname;

	  cname = find_attribute ("cname", attribute_names, attribute_values);

	  if (cname == NULL)
	    MISSING_ATTRIBUTE (error, element_name, "cname");
	  else
	    {  
	      GIdlNodeInterface *iface;

	      iface = (GIdlNodeInterface *)ctx->current_node;
	      iface ->prerequisites = g_list_append (iface->prerequisites, g_strdup (cname));
	    }

	  goto out;
	}
      break;

    case 'p':
      if (start_property (context, element_name,
			  attribute_names, attribute_values,
			  ctx, error))
	goto out;
      else if (strcmp (element_name, "parameters") == 0 &&
	  ctx->state == STATE_FUNCTION)
	{
	  ctx->state = STATE_PARAMETERS;

	  goto out;
	}
      else if (start_parameter (context, element_name,
				attribute_names, attribute_values,
				ctx, error))
	goto out;

      break;

    case 'r':
      if (start_return_type (context, element_name,
			     attribute_names, attribute_values,
			     ctx, error))
	goto out;      
      else if (strcmp (element_name, "requires") == 0 &&
	       ctx->state == STATE_INTERFACE)
	{
	  ctx->state = STATE_REQUIRES;
	  
	  goto out;
	}

      break;

    case 's':
      if (start_signal (context, element_name,
			attribute_names, attribute_values,
			ctx, error))
	goto out;      
      else if (start_struct (context, element_name,
			     attribute_names, attribute_values,
			     ctx, error))
	goto out;      

      break;

    case 'v':
      if (start_vfunc (context, element_name,
		       attribute_names, attribute_values,
		       ctx, error))
	goto out;      
      break;
    }

  g_markup_parse_context_get_position (context, &line_number, &char_number);

  g_set_error (error,
	       G_MARKUP_ERROR,
	       G_MARKUP_ERROR_UNKNOWN_ELEMENT,
	       "Unexpected start tag '%s' on line %d char %d",
	       element_name,
	       line_number, char_number);
  
 out: ;
      
}

static void
end_element_handler (GMarkupParseContext *context,
		     const gchar         *element_name,
		     gpointer             user_data,
		     GError             **error)
{
  ParseContext *ctx = user_data;

  switch (ctx->state)
    {
    case STATE_START:
    case STATE_END:
      /* no need to GError here, GMarkup already catches this */
      break;

    case STATE_ROOT:
      ctx->state = STATE_END;
      break;

    case STATE_NAMESPACE:
      ctx->current_module = NULL;
      ctx->state = STATE_ROOT;
      break;

    case STATE_FUNCTION:
      if (strcmp (element_name, "return-type") == 0)
	/* do nothing */ ;
	
      else if (ctx->current_node == g_list_last (ctx->current_module->entries)->data)
	{
	  ctx->current_node = NULL;
	  ctx->state = STATE_NAMESPACE;
	}
      else 
	{ 
	  ctx->current_node = g_list_last (ctx->current_module->entries)->data;
	  if (ctx->current_node->type == G_IDL_NODE_INTERFACE)
	    ctx->state = STATE_INTERFACE;
	  else if (ctx->current_node->type == G_IDL_NODE_OBJECT)
	    ctx->state = STATE_OBJECT;
	  else if (ctx->current_node->type == G_IDL_NODE_BOXED)
	    ctx->state = STATE_BOXED;
	  else if (ctx->current_node->type == G_IDL_NODE_STRUCT)
	    ctx->state = STATE_STRUCT;
	}
      break;

    case STATE_OBJECT:
      if (strcmp (element_name, "object") == 0)
	{
	  ctx->current_node = NULL;
	  ctx->state = STATE_NAMESPACE;
	}
      break;

    case STATE_ERRORDOMAIN:
      if (strcmp (element_name, "errordomain") == 0)
	{
	  ctx->current_node = NULL;
	  ctx->state = STATE_NAMESPACE;
	}
      break;

    case STATE_INTERFACE:
      if (strcmp (element_name, "interface") == 0)
	{
	  ctx->current_node = NULL;
	  ctx->state = STATE_NAMESPACE;
	}
      break;

    case STATE_ENUM:
      if (strcmp (element_name, "enum") == 0 ||
	  strcmp (element_name, "flags") == 0)
	{
	  ctx->current_node = NULL;
	  ctx->state = STATE_NAMESPACE;
	}
      break;

    case STATE_BOXED:
      if (strcmp (element_name, "boxed") == 0)
	{
	  ctx->current_node = NULL;
	  ctx->state = STATE_NAMESPACE;
	}
      break;

    case STATE_STRUCT:
      if (strcmp (element_name, "struct") == 0)
	{
	  ctx->current_node = NULL;
	  ctx->state = STATE_NAMESPACE;
	}
      break;
    case STATE_IMPLEMENTS:
      ctx->state = STATE_OBJECT;
      break;
    case STATE_REQUIRES:
      ctx->state = STATE_INTERFACE;
      break;
    case STATE_PARAMETERS:
      if (strcmp (element_name, "parameters") == 0)
	ctx->state = STATE_FUNCTION;
      break;
    default:
      g_error ("Unhandled state %d in end_element_handler\n", ctx->state);
    }
}

static void 
text_handler (GMarkupParseContext *context,
	      const gchar         *text,
	      gsize                text_len,  
	      gpointer             user_data,
	      GError             **error)
{
  /* FIXME warn about non-whitespace text */
}

static void
cleanup (GMarkupParseContext *context,
	 GError              *error,
	 gpointer             user_data)
{
  ParseContext *ctx = user_data;
  GList *m;

  for (m = ctx->modules; m; m = m->next)
    g_idl_module_free (m->data);
  g_list_free (ctx->modules);
  ctx->modules = NULL;
  
  ctx->current_module = NULL;
}

static GMarkupParser parser = 
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  cleanup
};

GList * 
g_idl_parse_string (const gchar  *buffer, 
		    gssize        length,
                    GError      **error)
{
  ParseContext ctx = { 0 };
  GMarkupParseContext *context;

  ctx.state = STATE_START;
  
  context = g_markup_parse_context_new (&parser, 0, &ctx, NULL);
  if (!g_markup_parse_context_parse (context, buffer, length, error))
    goto out;

  if (!g_markup_parse_context_end_parse (context, error))
    goto out;

 out:
  
  g_markup_parse_context_free (context);
  
  return ctx.modules;
}

GList *
g_idl_parse_file (const gchar  *filename,
		  GError      **error)
{
  gchar *buffer;
  gssize length;
  GList *modules;

  if (!g_file_get_contents (filename, &buffer, &length, error))
    return NULL;
  
  modules = g_idl_parse_string (buffer, length, error);

  g_free (buffer);

  return modules;
}


