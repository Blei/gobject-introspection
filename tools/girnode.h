/* GObject introspection: Parsed GIR
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

#ifndef __G_IR_NODE_H__
#define __G_IR_NODE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GIrNode GIrNode; 
typedef struct _GIrNodeFunction GIrNodeFunction;
typedef struct _GIrNodeParam GIrNodeParam;
typedef struct _GIrNodeType GIrNodeType;
typedef struct _GIrNodeInterface GIrNodeInterface;
typedef struct _GIrNodeSignal GIrNodeSignal;
typedef struct _GIrNodeProperty GIrNodeProperty;
typedef struct _GIrNodeVFunc GIrNodeVFunc;
typedef struct _GIrNodeField GIrNodeField;
typedef struct _GIrNodeValue GIrNodeValue;
typedef struct _GIrNodeEnum GIrNodeEnum;
typedef struct _GIrNodeBoxed GIrNodeBoxed;
typedef struct _GIrNodeStruct GIrNodeStruct;
typedef struct _GIrNodeConstant GIrNodeConstant;
typedef struct _GIrNodeErrorDomain GIrNodeErrorDomain;
typedef struct _GIrNodeXRef GIrNodeXRef;
typedef struct _GIrNodeUnion GIrNodeUnion;

typedef enum 
{
  G_IR_NODE_INVALID,
  G_IR_NODE_FUNCTION,
  G_IR_NODE_CALLBACK,
  G_IR_NODE_STRUCT,
  G_IR_NODE_BOXED,
  G_IR_NODE_ENUM,
  G_IR_NODE_FLAGS,
  G_IR_NODE_OBJECT,
  G_IR_NODE_INTERFACE,
  G_IR_NODE_CONSTANT,
  G_IR_NODE_ERROR_DOMAIN,
  G_IR_NODE_UNION,
  G_IR_NODE_PARAM,
  G_IR_NODE_TYPE,
  G_IR_NODE_PROPERTY,
  G_IR_NODE_SIGNAL,
  G_IR_NODE_VALUE,
  G_IR_NODE_VFUNC,
  G_IR_NODE_FIELD,
  G_IR_NODE_XREF
} GIrNodeTypeId;

struct _GIrNode
{
  GIrNodeTypeId type;
  gchar *name;
};

struct _GIrNodeXRef
{
  GIrNode node;

  gchar *namespace;
};

struct _GIrNodeFunction
{
  GIrNode node;

  gboolean deprecated;

  gboolean is_method;
  gboolean is_setter;
  gboolean is_getter;
  gboolean is_constructor;
  gboolean wraps_vfunc;

  gchar *symbol;

  GIrNodeParam *result;
  GList *parameters;
};

struct _GIrNodeType 
{
  GIrNode node;

  gboolean is_pointer;
  gboolean is_basic;
  gboolean is_array;
  gboolean is_glist;
  gboolean is_gslist;
  gboolean is_ghashtable;
  gboolean is_interface;
  gboolean is_error;
  gint tag;

  gchar *unparsed;

  gboolean zero_terminated;
  gboolean has_length;
  gint length;
  
  GIrNodeType *parameter_type1;
  GIrNodeType *parameter_type2;  

  gchar *interface;
  gchar **errors;
};

struct _GIrNodeParam 
{
  GIrNode node;

  gboolean in;
  gboolean out;
  gboolean dipper;
  gboolean optional;
  gboolean retval;
  gboolean null_ok;
  gboolean transfer;
  gboolean shallow_transfer;
  
  GIrNodeType *type;
};

struct _GIrNodeProperty
{
  GIrNode node;

  gboolean deprecated;

  gchar *name;
  gboolean readable;
  gboolean writable;
  gboolean construct;
  gboolean construct_only;
  
  GIrNodeType *type;
};

struct _GIrNodeSignal 
{
  GIrNode node;

  gboolean deprecated;

  gboolean run_first;
  gboolean run_last;
  gboolean run_cleanup;
  gboolean no_recurse;
  gboolean detailed;
  gboolean action;
  gboolean no_hooks;
  
  gboolean has_class_closure;
  gboolean true_stops_emit;
  
  gint class_closure;
  
  GList *parameters;
  GIrNodeParam *result;    
};

struct _GIrNodeVFunc 
{
  GIrNode node;

  gboolean must_chain_up;
  gboolean must_be_implemented;
  gboolean must_not_be_implemented;
  gboolean is_class_closure;
  
  GList *parameters;
  GIrNodeParam *result;      

  gint offset;
};

struct _GIrNodeField
{
  GIrNode node;

  gboolean readable;
  gboolean writable;
  gint bits;
  gint offset;
  
  GIrNodeType *type;
};

struct _GIrNodeInterface
{
  GIrNode node;

  gboolean deprecated;

  gchar *gtype_name;
  gchar *gtype_init;

  gchar *parent;
  
  GList *interfaces;
  GList *prerequisites;

  GList *members;
};

struct _GIrNodeValue
{
  GIrNode node;

  gboolean deprecated;

  guint32 value;
};

struct _GIrNodeConstant
{
  GIrNode node;

  gboolean deprecated;

  GIrNodeType *type;
  
  gchar *value;
};

struct _GIrNodeEnum
{
  GIrNode node;

  gboolean deprecated;

  gchar *gtype_name;
  gchar *gtype_init;

  GList *values;
};

struct _GIrNodeBoxed
{ 
  GIrNode node;

  gboolean deprecated;

  gchar *gtype_name;
  gchar *gtype_init;
  
  GList *members;
};

struct _GIrNodeStruct
{
  GIrNode node;

  gboolean deprecated;
  
  GList *members;
};

struct _GIrNodeUnion
{
  GIrNode node;

  gboolean deprecated;
  
  GList *members;
  GList *discriminators;

  gchar *gtype_name;
  gchar *gtype_init;

  gint discriminator_offset;
  GIrNodeType *discriminator_type;
};


struct _GIrNodeErrorDomain
{
  GIrNode node;

  gboolean deprecated;
  
  gchar *name;
  gchar *getquark;
  gchar *codes;
};


GIrNode * g_ir_node_new             (GIrNodeTypeId type);
void      g_ir_node_free            (GIrNode    *node);
guint32   g_ir_node_get_size        (GIrNode    *node);
guint32   g_ir_node_get_full_size   (GIrNode    *node);
void      g_ir_node_build_metadata  (GIrNode    *node,
				      GIrModule  *module,
				      GList       *modules,
                                      GHashTable  *strings,
                                      GHashTable  *types,
				      guchar      *data,
				      guint32     *offset,
                                      guint32     *offset2);
int       g_ir_node_cmp             (GIrNode    *node,
				      GIrNode    *other);
gboolean  g_ir_node_can_have_member (GIrNode    *node);
void      g_ir_node_add_member      (GIrNode         *node,
				      GIrNodeFunction *member);
guint32   write_string               (const gchar *str,
				      GHashTable  *strings, 
				      guchar      *data,
				      guint32     *offset);

const gchar * g_ir_node_param_direction_string (GIrNodeParam * node);

G_END_DECLS

#endif  /* __G_IR_NODE_H__ */
