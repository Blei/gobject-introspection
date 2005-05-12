/* GObject introspection: Repository implementation
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

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

#include "girepository.h"
#include "gmetadata.h"

struct _GIBaseInfo 
{
  gint type;
  gint ref_count;
  GIBaseInfo *container;

  const guchar *metadata;
  guint32 offset;
};

struct _GIUnresolvedInfo
{
  gint type;
  gint ref_count;
  GIBaseInfo *container;

  const gchar *name;
  const gchar *namespace;
};

struct _GICallableInfo
{
  GIBaseInfo base;
};

struct _GIFunctionInfo
{
  GICallableInfo callable;
};

struct _GICallbackInfo
{
  GICallableInfo callable;
};

struct _GIRegisteredTypeInfo
{
  GIBaseInfo base;
};

struct _GIStructInfo
{
  GIRegisteredTypeInfo registered;
};

struct _GIEnumInfo
{
  GIRegisteredTypeInfo registered;
};

struct _GIObjectInfo
{
  GIRegisteredTypeInfo registered;
};

struct _GIInterfaceInfo
{
  GIRegisteredTypeInfo registered;
};

struct _GIConstantInfo
{
  GIBaseInfo base;
};

struct _GIValueInfo
{
  GIBaseInfo base;
};

struct _GISignalInfo
{
  GICallableInfo callable;
};

struct _GIVFuncInfo
{
  GICallableInfo callable;
};

struct _GIPropertyInfo
{
  GIBaseInfo base;
};

struct _GIFieldInfo
{
  GIBaseInfo base;
};

struct _GIArgInfo
{
  GIBaseInfo base;
};


struct _GITypeInfo
{
  GIBaseInfo base;
};


/* info creation */
GIBaseInfo *
g_info_new (GIInfoType     type,
	    GIBaseInfo    *container,
	    const guchar *metadata, 
	    guint32       offset)
{
  GIBaseInfo *info;

  info = g_new0 (GIBaseInfo, 1);

  info->ref_count = 1;
  info->type = type;

  info->metadata = metadata;
  info->offset = offset;

  if (container)
    info->container = g_base_info_ref (container);

  return info;
}

static GIBaseInfo *
g_info_from_entry (const guchar *metadata,
		   guint16       index)
{
  GIBaseInfo *result;
  DirEntry *entry = g_metadata_get_dir_entry (metadata, index);
  
  if (entry->local)
    result = g_info_new (entry->blob_type, NULL, metadata, entry->offset);
  else 
    {
      const gchar *namespace = g_metadata_get_string (metadata, entry->offset);
      const gchar *name = g_metadata_get_string (metadata, entry->name);
      
      GIRepository *repository = g_irepository_get_default ();
      
      result = g_irepository_find_by_name (repository, namespace, name);
      if (result == NULL)
	{
	  GIUnresolvedInfo *unresolved;

	  unresolved = g_new0 (GIUnresolvedInfo, 1);
	  
	  unresolved->type = GI_INFO_TYPE_UNRESOLVED;
	  unresolved->ref_count = 1;
	  unresolved->container = NULL;
	  unresolved->name = name;
	  unresolved->namespace = namespace;

	  result = (GIBaseInfo*)unresolved;
	}
    }

  return result;
}

/* GIBaseInfo functions */
GIBaseInfo *
g_base_info_ref (GIBaseInfo *info)
{
  info->ref_count++;

  return info;
}

void
g_base_info_unref (GIBaseInfo *info)
{
  info->ref_count--;

  if (!info->ref_count)
    {
      if (info->container)
	g_base_info_unref (info->container);

      g_free (info);
    }
}

GIInfoType
g_base_info_get_type (GIBaseInfo *info)
{
  
  return info->type;
}

const gchar *
g_base_info_get_name (GIBaseInfo *info)
{
  switch (info->type)
    {
    case GI_INFO_TYPE_FUNCTION:
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_STRUCT:
    case GI_INFO_TYPE_BOXED:
    case GI_INFO_TYPE_ENUM:
    case GI_INFO_TYPE_FLAGS:
    case GI_INFO_TYPE_OBJECT:
    case GI_INFO_TYPE_INTERFACE:
    case GI_INFO_TYPE_CONSTANT:
    case GI_INFO_TYPE_ERROR_DOMAIN:
      {
	CommonBlob *blob = (CommonBlob *)&info->metadata[info->offset];

	return g_metadata_get_string (info->metadata, blob->name);
      }
      break;

    case GI_INFO_TYPE_VALUE:
      {
	ValueBlob *blob = (ValueBlob *)&info->metadata[info->offset];

	return g_metadata_get_string (info->metadata, blob->name);
      }
      break;

    case GI_INFO_TYPE_SIGNAL:
      {
	SignalBlob *blob = (SignalBlob *)&info->metadata[info->offset];

	return g_metadata_get_string (info->metadata, blob->name);
      }
      break;

    case GI_INFO_TYPE_PROPERTY:
      {
	PropertyBlob *blob = (PropertyBlob *)&info->metadata[info->offset];

	return g_metadata_get_string (info->metadata, blob->name);
      }
      break;

    case GI_INFO_TYPE_VFUNC:
      {
	VFuncBlob *blob = (VFuncBlob *)&info->metadata[info->offset];

	return g_metadata_get_string (info->metadata, blob->name);
      }
      break;

    case GI_INFO_TYPE_FIELD:
      {
	FieldBlob *blob = (FieldBlob *)&info->metadata[info->offset];
	
	return g_metadata_get_string (info->metadata, blob->name);
      }
      break;

    case GI_INFO_TYPE_ARG:
      {
	ArgBlob *blob = (ArgBlob *)&info->metadata[info->offset];
	
	return g_metadata_get_string (info->metadata, blob->name);
      }
      break;

    case GI_INFO_TYPE_UNRESOLVED:
      {
	GIUnresolvedInfo *unresolved = (GIUnresolvedInfo *)info;

	return unresolved->name;
      }
      break;

    case GI_INFO_TYPE_TYPE:
    default: ;
      /* unnamed */
    }

  return NULL;
}

const gchar *
g_base_info_get_namespace (GIBaseInfo *info)
{
  Header *header = (Header *)info->metadata;  

  if (info->type == GI_INFO_TYPE_UNRESOLVED)
    {
      GIUnresolvedInfo *unresolved = (GIUnresolvedInfo *)info;
      
      return unresolved->namespace;
    }

  return g_metadata_get_string (info->metadata, header->namespace);
}

gboolean 
g_base_info_is_deprecated (GIBaseInfo *info)
{
  switch (info->type)
    {
    case GI_INFO_TYPE_FUNCTION:
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_STRUCT:
    case GI_INFO_TYPE_BOXED:
    case GI_INFO_TYPE_ENUM:
    case GI_INFO_TYPE_FLAGS:
    case GI_INFO_TYPE_OBJECT:
    case GI_INFO_TYPE_INTERFACE:
    case GI_INFO_TYPE_CONSTANT:
    case GI_INFO_TYPE_ERROR_DOMAIN:
      {
	CommonBlob *blob = (CommonBlob *)&info->metadata[info->offset];

	return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_VALUE:
      {
	ValueBlob *blob = (ValueBlob *)&info->metadata[info->offset];

	return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_SIGNAL:
      {
	SignalBlob *blob = (SignalBlob *)&info->metadata[info->offset];

	return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_PROPERTY:
      {
	PropertyBlob *blob = (PropertyBlob *)&info->metadata[info->offset];

	return blob->deprecated;
      }
      break;

    case GI_INFO_TYPE_VFUNC:
    case GI_INFO_TYPE_FIELD:
    case GI_INFO_TYPE_ARG:
    case GI_INFO_TYPE_TYPE:
    default: ;
      /* no deprecation flag for these */
    }
  
  return FALSE;
}

static int
cmp_annotation (const void *av,
		const void *bv)
{
  const AnnotationBlob *a = av;
  const AnnotationBlob *b = bv;
 
  if (b->offset < a->offset)
    return -1;

  if (b->offset > a->offset)
    return 1;
  
  return 0;
}

const gchar *
g_base_info_get_annotation (GIBaseInfo   *info,
			    const gchar *name)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;
  AnnotationBlob blob, *first, *after, *res, *next;
  const gchar *rname;

  blob.offset = base->offset;
  
  first = (AnnotationBlob *) &base->metadata[header->annotations];
  after = (AnnotationBlob *) &base->metadata[header->annotations + 
					     header->n_annotations * header->annotation_blob_size];

  res = bsearch (&blob, first, header->n_annotations,
		 header->annotation_blob_size, cmp_annotation);
  
  if (res == NULL)
    return NULL;

  next = res;
  do 
    {
      res = next;
      next = res -= header->annotation_blob_size;
    }
  while (next >= first && next->offset == base->offset);
    
  next = res;
  do 
    {
      res = next;
      
      rname = g_metadata_get_string (base->metadata, res->name);
      if (strcmp (name, rname) == 0)
	return g_metadata_get_string (base->metadata, res->value);

      next = res += header->annotation_blob_size;
    }
  while (next < after && next->offset == base->offset);

  return NULL;
}

GIBaseInfo *
g_base_info_get_container (GIBaseInfo *info)
{
  info->container;
}


/* GIFunctionInfo functions */
const gchar *
g_function_info_get_symbol (GIFunctionInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  FunctionBlob *blob = (FunctionBlob *)&base->metadata[base->offset];

  return g_metadata_get_string (base->metadata, blob->symbol);
}

GIFunctionInfoFlags
g_function_info_get_flags (GIFunctionInfo *info)
{
  GIFunctionInfoFlags flags;
  GIBaseInfo *base = (GIBaseInfo *)info;
  FunctionBlob *blob = (FunctionBlob *)&base->metadata[base->offset];
  
  flags = 0;

  if (base->container != NULL)
    flags = flags | GI_FUNCTION_IS_METHOD;
    
  if (blob->constructor)
    flags = flags | GI_FUNCTION_IS_CONSTRUCTOR;

  if (blob->getter)
    flags = flags | GI_FUNCTION_IS_GETTER;

  if (blob->setter)
    flags = flags | GI_FUNCTION_IS_SETTER;

  if (blob->wraps_vfunc)
    flags = flags | GI_FUNCTION_WRAPS_VFUNC;

  return flags;
}

GIPropertyInfo *
g_function_info_get_property (GIFunctionInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  FunctionBlob *blob = (FunctionBlob *)&base->metadata[base->offset];
  GIInterfaceInfo *container = (GIInterfaceInfo *)base->container;
  
  return g_interface_info_get_property (container, blob->index);  
}

GIVFuncInfo *
g_function_info_get_vfunc (GIFunctionInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  FunctionBlob *blob = (FunctionBlob *)&base->metadata[base->offset];
  GIInterfaceInfo *container = (GIInterfaceInfo *)base->container;
  
  return g_interface_info_get_vfunc (container, blob->index);  
}


/* GICallableInfo functions */
static guint32
signature_offset (GICallableInfo *info)
{
  switch (info->base.type)
    {
    case GI_INFO_TYPE_FUNCTION:
    case GI_INFO_TYPE_VFUNC:
      return *(guint32 *)&info->base.metadata[info->base.offset + 12];
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_SIGNAL:
      return *(guint32 *)&info->base.metadata[info->base.offset + 8];
    }
  
  return 0;
}

GITypeInfo *
g_type_info_new (GIBaseInfo    *container,
		 const guchar *metadata,
		 guint32       offset)
{
  Header *header = (Header *)metadata;  
  SimpleTypeBlob *type = (SimpleTypeBlob *)&metadata[offset];

  return (GITypeInfo *) g_info_new (GI_INFO_TYPE_TYPE, container, metadata, 
				    type->reserved == 0 ? offset : type->offset);
}

GITypeInfo *
g_callable_info_get_return_type (GICallableInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  guint32 offset;

  offset = signature_offset (info);

  return g_type_info_new (base, base->metadata, offset);
}

Transfer 
g_callable_info_get_caller_owns (GICallableInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SignatureBlob *blob = (SignatureBlob *)&base->metadata[signature_offset (info)];

  if (blob->caller_owns_return_value)
    return GI_TRANSFER_EVERYTHING;
  else if (blob->caller_owns_return_container)
    return GI_TRANSFER_CONTAINER;
  else
    return GI_TRANSFER_NOTHING;
}

gint 
g_callable_info_get_n_args (GICallableInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  gint offset;
  SignatureBlob *blob;

  offset = signature_offset (info);
  blob = (SignatureBlob *)&base->metadata[offset];

  return blob->n_arguments;
}

GIArgInfo *
g_callable_info_get_arg (GICallableInfo *info,
			 gint           n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;
  gint offset;

  offset = signature_offset (info);
  
  return (GIArgInfo *) g_info_new (GI_INFO_TYPE_ARG, base, base->metadata, 
				   offset + header->signature_blob_size + n * header->arg_blob_size);
}

/* GIArgInfo function */
Direction
g_arg_info_get_direction (GIArgInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ArgBlob *blob = (ArgBlob *)&base->metadata[base->offset];
  
  if (blob->in && blob->out)
    return GI_DIRECTION_INOUT;
  else if (blob->out)
    return GI_DIRECTION_OUT;
  else
    return GI_DIRECTION_IN;
}

gboolean
g_arg_info_is_return_value (GIArgInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ArgBlob *blob = (ArgBlob *)&base->metadata[base->offset];
  
  return blob->return_value;
}

gboolean
g_arg_info_is_dipper (GIArgInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ArgBlob *blob = (ArgBlob *)&base->metadata[base->offset];
  
  return blob->dipper;
}

gboolean
g_arg_info_is_optional (GIArgInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ArgBlob *blob = (ArgBlob *)&base->metadata[base->offset];
  
  return blob->optional;
}

gboolean
g_arg_info_may_be_null (GIArgInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ArgBlob *blob = (ArgBlob *)&base->metadata[base->offset];
  
  return blob->null_ok;
}

Transfer
g_arg_info_get_ownership_transfer (GIArgInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ArgBlob *blob = (ArgBlob *)&base->metadata[base->offset];

  if (blob->transfer_ownership)
    return GI_TRANSFER_EVERYTHING;
  else if (blob->transfer_container_ownership)
    return GI_TRANSFER_CONTAINER;
  else
    return GI_TRANSFER_NOTHING;
}

GITypeInfo *
g_arg_info_get_type (GIArgInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
 
  return g_type_info_new (base, base->metadata, base->offset + 8);
}

/* GITypeInfo functions */
gboolean
g_type_info_is_pointer (GITypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];
  
  if (type->reserved == 0)
    return type->pointer;
  else
    {
      InterfaceTypeBlob *iface = (InterfaceTypeBlob *)&base->metadata[base->offset];
      
      return iface->pointer;
    }
}

GITypeTag
g_type_info_get_tag (GITypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];

  if (type->reserved == 0)
    return type->tag;
  else
    {
      InterfaceTypeBlob *iface = (InterfaceTypeBlob *)&base->metadata[base->offset];

      return iface->tag;
    }
}

GITypeInfo *
g_type_info_get_param_type (GITypeInfo *info,
			    gint       n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];
  
  if (type->reserved != 0)
    {
      ParamTypeBlob *param = (ParamTypeBlob *)&base->metadata[base->offset];

      switch (param->tag)
	{
	case GI_TYPE_TAG_ARRAY: 
	case GI_TYPE_TAG_GLIST:
	case GI_TYPE_TAG_GSLIST:
	case GI_TYPE_TAG_GHASH:
	  return g_type_info_new (base, base->metadata, base->offset + 4 + 4 * n);
	  break;
	  
	default: ;
	}
    }
      
  return NULL;
}

GIBaseInfo *
g_type_info_get_interface (GITypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];
  
  if (type->reserved != 0)
    {
      InterfaceTypeBlob *blob = (InterfaceTypeBlob *)&base->metadata[base->offset];
      
      if (blob->tag == GI_TYPE_TAG_INTERFACE)
	return g_info_from_entry (base->metadata, blob->interface);
    }

  return NULL;
}

gint
g_type_info_get_array_length (GITypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];
  
  if (type->reserved != 0)
    {
      ArrayTypeBlob *blob = (ArrayTypeBlob *)&base->metadata[base->offset];

      if (blob->tag == GI_TYPE_TAG_ARRAY)
	{
	  if (blob->has_length)
	    return blob->length;
	}
    }

  return -1;
}

gboolean
g_type_info_is_zero_terminated (GITypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];
  
  if (type->reserved != 0)
    {
      ArrayTypeBlob *blob = (ArrayTypeBlob *)&base->metadata[base->offset];

      if (blob->tag == GI_TYPE_TAG_ARRAY)
	return blob->zero_terminated;
    }

  return FALSE;
}

gint
g_type_info_get_n_error_domains (GITypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];
  
  if (type->reserved != 0)
    {
      ErrorTypeBlob *blob = (ErrorTypeBlob *)&base->metadata[base->offset];

      if (blob->tag == GI_TYPE_TAG_ERROR)
	return blob->n_domains;
    }

  return 0;
}

GIErrorDomainInfo *
g_type_info_get_error_domain (GITypeInfo *info,
			      gint       n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SimpleTypeBlob *type = (SimpleTypeBlob *)&base->metadata[base->offset];
  
  if (type->reserved != 0)
    {
      ErrorTypeBlob *blob = (ErrorTypeBlob *)&base->metadata[base->offset];

      if (blob->tag == GI_TYPE_TAG_ERROR)
	return (GIErrorDomainInfo *) g_info_from_entry (base->metadata, 
							blob->domains[n]);
    }

  return NULL;
}


/* GIErrorDomainInfo functions */
const gchar *
g_error_domain_info_get_quark (GIErrorDomainInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ErrorDomainBlob *blob = (ErrorDomainBlob *)&base->metadata[base->offset];

  return g_metadata_get_string (base->metadata, blob->get_quark);
}

GIInterfaceInfo *
g_error_domain_info_get_codes (GIErrorDomainInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ErrorDomainBlob *blob = (ErrorDomainBlob *)&base->metadata[base->offset];
  
  return (GIInterfaceInfo *) g_info_from_entry (base->metadata, blob->error_codes);
}


/* GIValueInfo functions */ 
glong
g_value_info_get_value (GIValueInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ValueBlob *blob = (ValueBlob *)&base->metadata[base->offset];

  return (glong)blob->value;
}

/* GIFieldInfo functions */
GIFieldInfoFlags
g_field_info_get_flags (GIFieldInfo *info)
{
  GIFieldInfoFlags flags;

  GIBaseInfo *base = (GIBaseInfo *)info;
  FieldBlob *blob = (FieldBlob *)&base->metadata[base->offset];

  flags = 0;

  if (blob->readable)
    flags = flags | GI_FIELD_IS_READABLE;

  if (blob->writable)
    flags = flags | GI_FIELD_IS_WRITABLE;

  return flags;
}

gint
g_field_info_get_size (GIFieldInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  FieldBlob *blob = (FieldBlob *)&base->metadata[base->offset];
  
  return blob->bits;
}

gint
g_field_info_get_offset (GIFieldInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  FieldBlob *blob = (FieldBlob *)&base->metadata[base->offset];
  
  return blob->struct_offset;
}

GITypeInfo *
g_field_info_get_type (GIFieldInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  
  return g_type_info_new (base, base->metadata, base->offset + 8);
}

/* GIRegisteredTypeInfo functions */
const gchar *
g_registered_type_info_get_type_name (GIRegisteredTypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  RegisteredTypeBlob *blob = (RegisteredTypeBlob *)&base->metadata[base->offset];

  if (blob->gtype_name)
    return g_metadata_get_string (base->metadata, blob->gtype_name);

  return NULL;
}

const gchar *
g_registered_type_info_get_type_init (GIRegisteredTypeInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  RegisteredTypeBlob *blob = (RegisteredTypeBlob *)&base->metadata[base->offset];

  if (blob->gtype_init)
    return g_metadata_get_string (base->metadata, blob->gtype_init);

  return NULL;
}


/* GIStructInfo functions */
gint
g_struct_info_get_n_fields (GIStructInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  StructBlob *blob = (StructBlob *)&base->metadata[base->offset];
  
  return blob->n_fields;
}

GIFieldInfo *
g_struct_info_get_field (GIStructInfo *info,
			 gint         n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
 
  return (GIFieldInfo *) g_info_new (GI_INFO_TYPE_FIELD, base, base->metadata, 
				     base->offset + header->struct_blob_size + 
				     n * header->field_blob_size);
}

gint
g_struct_info_get_n_methods (GIStructInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  StructBlob *blob = (StructBlob *)&base->metadata[base->offset];
  
  return blob->n_methods;
}

GIFunctionInfo *
g_struct_info_get_method (GIStructInfo *info,
			  gint         n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  StructBlob *blob = (StructBlob *)&base->metadata[base->offset];
  Header *header = (Header *)base->metadata;  
  gint offset;

  offset = base->offset + header->struct_blob_size 
    + blob->n_fields * header->field_blob_size 
    + n * header->function_blob_size;
  return (GIFunctionInfo *) g_info_new (GI_INFO_TYPE_FUNCTION, base, 
					base->metadata, offset);
}

static GIFunctionInfo *
find_method (GIBaseInfo   *base,
	     guint32       offset,
	     gint          n_methods,
	     const gchar  *name)
{
  /* FIXME hash */
  Header *header = (Header *)base->metadata;  
  gint i;

  for (i = 0; i < n_methods; i++)
    {
      FunctionBlob *fblob = (FunctionBlob *)&base->metadata[offset];
      const gchar *fname = (const gchar *)&base->metadata[fblob->name];

      if (strcmp (name, fname) == 0)
	return (GIFunctionInfo *) g_info_new (GI_INFO_TYPE_FUNCTION, base, 
					      base->metadata, offset);  
      
      offset += header->function_blob_size;
    }
      
  return NULL;
}

GIFunctionInfo *
g_struct_info_find_method (GIStructInfo *info,
			   const gchar  *name)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  StructBlob *blob = (StructBlob *)&base->metadata[base->offset];

  offset = base->offset + header->struct_blob_size
    + blob->n_fields * header->field_blob_size;

  return find_method (base, offset, blob->n_methods, name);
}

gint
g_enum_info_get_n_values (GIEnumInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  EnumBlob *blob = (EnumBlob *)&base->metadata[base->offset];

  return blob->n_values;
}

GIValueInfo *
g_enum_info_get_value (GIEnumInfo *info,
		       gint            n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  EnumBlob *blob = (EnumBlob *)&base->metadata[base->offset];
  gint offset;

  offset = base->offset + header->enum_blob_size 
    + n * header->value_blob_size;
  return (GIValueInfo *) g_info_new (GI_INFO_TYPE_VALUE, base, base->metadata, offset);
}

/* GIObjectInfo functions */
GIObjectInfo *
g_object_info_get_parent (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  if (blob->parent)
    return (GIObjectInfo *) g_info_from_entry (base->metadata, blob->parent);
  else
    return NULL;
}

gint
g_object_info_get_n_interfaces (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  return blob->n_interfaces;
}

GIInterfaceInfo *
g_object_info_get_interface (GIObjectInfo *info,
			     gint          n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  return (GIInterfaceInfo *) g_info_from_entry (base->metadata, blob->interfaces[n]);
}

gint
g_object_info_get_n_fields (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  return blob->n_fields;
}

GIFieldInfo *
g_object_info_get_field (GIObjectInfo *info,
			 gint          n)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2) * 2
    + n * header->field_blob_size;
  
  return (GIFieldInfo *) g_info_new (GI_INFO_TYPE_FIELD, base, base->metadata, offset);
}

gint
g_object_info_get_n_properties (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  return blob->n_properties;  
}

GIPropertyInfo *
g_object_info_get_property (GIObjectInfo *info,
			    gint          n)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2) * 2
    + blob->n_fields * header->field_blob_size
    + n * header->property_blob_size;

  return (GIPropertyInfo *) g_info_new (GI_INFO_TYPE_PROPERTY, base, 
					base->metadata, offset);
}

gint
g_object_info_get_n_methods (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  return blob->n_methods;
}

GIFunctionInfo *
g_object_info_get_method (GIObjectInfo *info,
			  gint          n)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2) * 2
    + blob->n_fields * header->field_blob_size
    + blob->n_properties * header->property_blob_size
    + n * header->function_blob_size;

    return (GIFunctionInfo *) g_info_new (GI_INFO_TYPE_FUNCTION, base, 
					  base->metadata, offset);  
}

GIFunctionInfo *
g_object_info_find_method (GIObjectInfo *info,
			   const gchar  *name)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  offset = base->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2) * 2
    + blob->n_fields * header->field_blob_size +
    + blob->n_properties * header->property_blob_size;

  return find_method (base, offset, blob->n_methods, name);
}

gint
g_object_info_get_n_signals (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];

  return blob->n_signals;
}

GISignalInfo *
g_object_info_get_signal (GIObjectInfo *info,
			  gint          n)
{
  gint i, offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2) * 2
    + blob->n_fields * header->field_blob_size
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size 
    + n * header->signal_blob_size;

  return (GISignalInfo *) g_info_new (GI_INFO_TYPE_SIGNAL, base, 
				      base->metadata, offset);  
}

gint
g_object_info_get_n_vfuncs (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  return blob->n_vfuncs;
}

GIVFuncInfo *
g_object_info_get_vfunc (GIObjectInfo *info,
			 gint          n)
{
  gint i, offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2) * 2
    + blob->n_fields * header->field_blob_size
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size 
    + blob->n_signals * header->signal_blob_size 
    + n * header->vfunc_blob_size;

  return (GIVFuncInfo *) g_info_new (GI_INFO_TYPE_VFUNC, base, 
				     base->metadata, offset);  
}

gint
g_object_info_get_n_constants (GIObjectInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  return blob->n_constants;
}

GIConstantInfo *
g_object_info_get_constant (GIObjectInfo *info,
			    gint          n)
{
  gint i, offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  ObjectBlob *blob = (ObjectBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2) * 2
    + blob->n_fields * header->field_blob_size
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size 
    + blob->n_signals * header->signal_blob_size 
    + blob->n_vfuncs * header->vfunc_blob_size 
    + n * header->constant_blob_size;

  return (GIConstantInfo *) g_info_new (GI_INFO_TYPE_CONSTANT, base, 
					base->metadata, offset);  
}


/* GIInterfaceInfo functions */
gint
g_interface_info_get_n_prerequisites (GIInterfaceInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];

  return blob->n_prerequisites;
}

GIBaseInfo *
g_interface_info_get_prerequisite (GIInterfaceInfo *info,
				   gint            n)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];

  return g_info_from_entry (base->metadata, blob->prerequisites[n]);
}


gint
g_interface_info_get_n_properties (GIInterfaceInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];

  return blob->n_properties;  
}

GIPropertyInfo *
g_interface_info_get_property (GIInterfaceInfo *info,
			       gint            n)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2)) * 2
    + n * header->property_blob_size;

  return (GIPropertyInfo *) g_info_new (GI_INFO_TYPE_PROPERTY, base, 
					base->metadata, offset);
}

gint
g_interface_info_get_n_methods (GIInterfaceInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];

  return blob->n_methods;
}

GIFunctionInfo *
g_interface_info_get_method (GIInterfaceInfo *info,
			     gint            n)
{
  gint i, offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2)) * 2
    + blob->n_properties * header->property_blob_size 
    + n * header->function_blob_size;
  
  return (GIFunctionInfo *) g_info_new (GI_INFO_TYPE_FUNCTION, base, 
					base->metadata, offset);  
}

GIFunctionInfo *
g_interface_info_find_method (GIInterfaceInfo *info,
			      const gchar     *name)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];

  offset = base->offset + header->object_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2)) * 2
    + blob->n_properties * header->property_blob_size;

  return find_method (base, offset, blob->n_methods, name);
}

gint
g_interface_info_get_n_signals (GIInterfaceInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];

  return blob->n_signals;
}

GISignalInfo *
g_interface_info_get_signal (GIInterfaceInfo *info,
			     gint            n)
{
  gint i, offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->interface_blob_size 
    + (blob->n_prerequisites + (blob->n_prerequisites % 2)) * 2
    + blob->n_properties * header->property_blob_size 
    + blob->n_methods * header->function_blob_size 
    + n * header->signal_blob_size;
  
  return (GISignalInfo *) g_info_new (GI_INFO_TYPE_SIGNAL, base, 
				      base->metadata, offset);  
}

gint
g_interface_info_get_n_vfuncs (GIInterfaceInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];

  return blob->n_vfuncs;
}

GIVFuncInfo *
g_interface_info_get_vfunc (GIInterfaceInfo *info,
			    gint            n)
{
  gint i, offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->interface_blob_size 
    + (blob->n_prerequisites + (blob->n_prerequisites % 2)) * 2
    + blob->n_properties * header->property_blob_size 
    + blob->n_methods * header->function_blob_size 
    + blob->n_signals * header->signal_blob_size
    + n * header->vfunc_blob_size;
  
  return (GIVFuncInfo *) g_info_new (GI_INFO_TYPE_VFUNC, base, 
				     base->metadata, offset);  
}

gint
g_interface_info_get_n_constants (GIInterfaceInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];
  
  return blob->n_constants;
}

GIConstantInfo *
g_interface_info_get_constant (GIInterfaceInfo *info,
			       gint             n)
{
  gint offset;
  GIBaseInfo *base = (GIBaseInfo *)info;
  Header *header = (Header *)base->metadata;  
  InterfaceBlob *blob = (InterfaceBlob *)&base->metadata[base->offset];
  
  offset = base->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2)) * 2
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size 
    + blob->n_signals * header->signal_blob_size 
    + blob->n_vfuncs * header->vfunc_blob_size 
    + n * header->constant_blob_size;

  return (GIConstantInfo *) g_info_new (GI_INFO_TYPE_CONSTANT, base, 
					base->metadata, offset);  
}




/* GIPropertyInfo functions */
GParamFlags
g_property_info_get_flags (GIPropertyInfo *info)
{
  GParamFlags flags;
  GIBaseInfo *base = (GIBaseInfo *)info;
  PropertyBlob *blob = (PropertyBlob *)&base->metadata[base->offset];
  
  flags = 0;

  if (blob->readable)
    flags = flags | G_PARAM_READABLE;

  if (blob->writable)
    flags = flags | G_PARAM_WRITABLE;

  if (blob->construct)
    flags = flags | G_PARAM_CONSTRUCT;

  if (blob->construct_only)
    flags = flags | G_PARAM_CONSTRUCT_ONLY;

  return flags;
}

GITypeInfo *
g_property_info_get_type (GIPropertyInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;

  return g_type_info_new (base, base->metadata, base->offset + 8);  
}


/* GISignalInfo functions */
GSignalFlags
g_signal_info_get_flags (GISignalInfo *info)
{
  GSignalFlags flags;

  GIBaseInfo *base = (GIBaseInfo *)info;
  SignalBlob *blob = (SignalBlob *)&base->metadata[base->offset];

  flags = 0;

  if (blob->run_first)
    flags = flags | G_SIGNAL_RUN_FIRST;

  if (blob->run_last)
    flags = flags | G_SIGNAL_RUN_LAST;

  if (blob->run_cleanup)
    flags = flags | G_SIGNAL_RUN_CLEANUP;

  if (blob->no_recurse)
    flags = flags | G_SIGNAL_NO_RECURSE;

  if (blob->detailed)
    flags = flags | G_SIGNAL_DETAILED;

  if (blob->action)
    flags = flags | G_SIGNAL_ACTION;

  if (blob->no_hooks)
    flags = flags | G_SIGNAL_NO_HOOKS;

  return flags;
}

GIVFuncInfo *
g_signal_info_get_class_closure (GISignalInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SignalBlob *blob = (SignalBlob *)&base->metadata[base->offset];

  if (blob->has_class_closure)
    return g_interface_info_get_vfunc ((GIInterfaceInfo *)base->container, blob->class_closure);

  return NULL;
}

gboolean
g_signal_info_true_stops_emit (GISignalInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  SignalBlob *blob = (SignalBlob *)&base->metadata[base->offset];

  return blob->true_stops_emit;
}

/* GIVFuncInfo functions */
GIVFuncInfoFlags
g_vfunc_info_get_flags (GIVFuncInfo *info)
{
  GIVFuncInfoFlags flags;

  GIBaseInfo *base = (GIBaseInfo *)info;
  VFuncBlob *blob = (VFuncBlob *)&base->metadata[base->offset];

  flags = 0;

  if (blob->must_chain_up)
    flags = flags | GI_VFUNC_MUST_CHAIN_UP;

  if (blob->must_be_implemented)
    flags = flags | GI_VFUNC_MUST_OVERRIDE;

  if (blob->must_not_be_implemented)
    flags = flags | GI_VFUNC_MUST_NOT_OVERRIDE;

  return flags;
}

gint
g_vfunc_info_get_offset (GIVFuncInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  VFuncBlob *blob = (VFuncBlob *)&base->metadata[base->offset];
  
  return blob->struct_offset;
}

GISignalInfo *
g_vfunc_info_get_signal (GIVFuncInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  VFuncBlob *blob = (VFuncBlob *)&base->metadata[base->offset];

  if (blob->class_closure)
    return g_interface_info_get_signal ((GIInterfaceInfo *)base->container, blob->signal);
  
  return NULL;
}


/* GIConstantInfo functions */
GITypeInfo *
g_constant_info_get_type (GIConstantInfo *info)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  
  return g_type_info_new (base, base->metadata, base->offset + 8);
}

gint
g_constant_info_get_value (GIConstantInfo *info, 
			   GArgument      *value)
{
  GIBaseInfo *base = (GIBaseInfo *)info;
  ConstantBlob *blob = (ConstantBlob *)&base->metadata[base->offset];

  /* FIXME non-basic types ? */
  if (blob->type.reserved == 0)
    {
      if (blob->type.pointer)
	value->v_pointer = g_memdup (&base->metadata[blob->offset], blob->size);
      else
	{
	  switch (blob->type.tag)
	    {
	    case GI_TYPE_TAG_BOOLEAN:
	      value->v_boolean = *(gboolean*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_INT8:
	      value->v_int8 = *(gint8*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_UINT8:
	      value->v_uint8 = *(guint8*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_INT16:
	      value->v_int16 = *(gint16*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_UINT16:
	      value->v_uint16 = *(guint16*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_INT32:
	      value->v_int32 = *(gint32*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_UINT32:
	      value->v_uint32 = *(guint32*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_INT64:
	      value->v_int64 = *(gint64*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_UINT64:
	      value->v_uint64 = *(guint64*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_FLOAT:
	      value->v_float = *(gfloat*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_DOUBLE:
	      value->v_double = *(gdouble*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_INT:
	      value->v_int = *(gint*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_UINT:
	      value->v_uint = *(guint*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_LONG:
	      value->v_long = *(glong*)&base->metadata[blob->offset];
	      break;
	    case GI_TYPE_TAG_ULONG:
	      value->v_ulong = *(gulong*)&base->metadata[blob->offset];
	      break;
	    }
	}
    }

  return blob->size;
}
