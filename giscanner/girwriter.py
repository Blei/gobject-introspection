# -*- Mode: Python -*-
# GObject-Introspection - a framework for introspecting GObject libraries
# Copyright (C) 2008  Johan Dahlin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

from __future__ import with_statement

import os

from .ast import (Callback, Class, Enum, Function, Interface, Member,
                  Sequence, Struct, Alias, Union)
from .glibast import (GLibBoxed, GLibEnum, GLibEnumMember,
                      GLibFlags, GLibObject, GLibInterface)
from .xmlwriter import XMLWriter


class GIRWriter(XMLWriter):

    def __init__(self, namespace, shlib, includes):
        super(GIRWriter, self).__init__()
        self._write_repository(namespace, shlib, includes)

    def _write_repository(self, namespace, shlib, includes=set()):
        attrs = [
            ('version', '1.0'),
            ('xmlns', 'http://www.gtk.org/introspection/core/1.0'),
            ('xmlns:c', 'http://www.gtk.org/introspection/c/1.0'),
            ('xmlns:glib', 'http://www.gtk.org/introspection/glib/1.0'),
            ]
        with self.tagcontext('repository', attrs):
            for include in includes:
                self._write_include(include)
            self._write_namespace(namespace, shlib)

    def _write_include(self, include):
        attrs = [('name', include)]
        self.write_tag('include', attrs)

    def _write_namespace(self, namespace, shlib):
        attrs = [('name', namespace.name),
                 ('shared-library', os.path.basename(shlib))]
        with self.tagcontext('namespace', attrs):
            for node in namespace.nodes:
                self._write_node(node)

    def _write_node(self, node):
        if isinstance(node, Function):
            self._write_function(node)
        elif isinstance(node, Enum):
            self._write_enum(node)
        elif isinstance(node, (Class, Interface)):
            self._write_class(node)
        elif isinstance(node, Callback):
            self._write_callback(node)
        elif isinstance(node, Struct):
            self._write_record(node)
        elif isinstance(node, Union):
            self._write_union(node)
        elif isinstance(node, GLibBoxed):
            self._write_boxed(node)
        elif isinstance(node, Member):
            # FIXME: atk_misc_instance singleton
            pass
        elif isinstance(node, Alias):
            self._write_alias(node)
        else:
            print 'WRITER: Unhandled node', node

    def _append_deprecated(self, node, attrs):
        if node.deprecated:
            attrs.append(('deprecated', node.deprecated))
            if node.deprecated_version:
                attrs.append(('deprecated-version',
                              node.deprecated_version))

    def _write_alias(self, alias):
        attrs = [('name', alias.name), ('target', alias.target)]
        if alias.ctype is not None:
            attrs.append(('c:type', alias.ctype))
        self.write_tag('alias', attrs)

    def _write_function(self, func, tag_name='function'):
        attrs = [('name', func.name),
                 ('c:identifier', func.symbol)]
        self._append_deprecated(func, attrs)
        with self.tagcontext(tag_name, attrs):
            self._write_return_type(func.retval)
            self._write_parameters(func.parameters)

    def _write_method(self, method):
        self._write_function(method, tag_name='method')

    def _write_constructor(self, method):
        self._write_function(method, tag_name='constructor')

    def _write_return_type(self, return_):
        if not return_:
            return

        attrs = []
        if return_.transfer:
            attrs.append(('transfer-ownership',
                          str(int(return_.transfer))))
        with self.tagcontext('return-value', attrs):
            self._write_type(return_.type)

    def _write_parameters(self, parameters):
        if not parameters:
            return
        with self.tagcontext('parameters'):
            for parameter in parameters:
                self._write_parameter(parameter)

    def _write_parameter(self, parameter):
        attrs = []
        if parameter.name is not None:
            attrs.append(('name', parameter.name))
        if parameter.direction != 'in':
            attrs.append(('direction', parameter.direction))
        if parameter.transfer:
            attrs.append(('transfer-ownership',
                          str(int(parameter.transfer))))
        if parameter.allow_none:
            attrs.append(('allow-none', '1'))
        with self.tagcontext('parameter', attrs):
            self._write_type(parameter.type)

    def _write_type(self, ntype, relation=None):
        if isinstance(ntype, basestring):
            typename = ntype
            type_cname = None
        else:
            typename = ntype.name
            type_cname = ntype.ctype
        attrs = [('name', typename)]
        if relation:
            attrs.append(('relation', relation))
        if isinstance(ntype, Sequence):
            if ntype.transfer:
                attrs.append(('transfer-ownership',
                              str(int(ntype.transfer))))
            with self.tagcontext('type', attrs):
                self._write_type(ntype.element_type, relation="element")
        else:
            # FIXME: figure out if type references a basic type
            #        or a boxed/class/interface etc. and skip
            #        writing the ctype if the latter.
            if type_cname is not None:
                attrs.append(('c:type', type_cname))
            self.write_tag('type', attrs)

    def _write_enum(self, enum):
        attrs = [('name', enum.name),
                 ('c:type', enum.symbol)]
        self._append_deprecated(enum, attrs)
        tag_name = 'enumeration'
        if isinstance(enum, GLibEnum):
            attrs.extend([('glib:type-name', enum.type_name),
                          ('glib:get-type', enum.get_type)])
            if isinstance(enum, GLibFlags):
                tag_name = 'bitfield'

        with self.tagcontext(tag_name, attrs):
            for member in enum.members:
                self._write_member(member)

    def _write_member(self, member):
        attrs = [('name', member.name),
                 ('value', str(member.value)),
                 ('c:identifier', member.symbol)]
        if isinstance(member, GLibEnumMember):
            attrs.append(('glib:nick', member.nick))
        self.write_tag('member', attrs)

    def _write_class(self, node):
        attrs = [('name', node.name),
                 ('c:type', node.ctype)]
        self._append_deprecated(node, attrs)
        if isinstance(node, Class):
            tag_name = 'class'
            if node.parent is not None:
                attrs.append(('parent', node.parent))
        else:
            tag_name = 'interface'
        if isinstance(node, (GLibObject, GLibInterface)):
            attrs.append(('glib:type-name', node.type_name))
            if node.get_type:
                attrs.append(('glib:get-type', node.get_type))
        with self.tagcontext(tag_name, attrs):
            if isinstance(node, GLibObject):
                for iface in node.interfaces:
                    self.write_tag('implements', [('name', iface)])
            if isinstance(node, Class):
                for method in node.constructors:
                    self._write_constructor(method)
            for method in node.methods:
                self._write_method(method)
            for prop in node.properties:
                self._write_property(prop)
            for field in node.fields:
                self._write_field(field)
            for signal in node.signals:
                self._write_signal(signal)

    def _write_boxed(self, boxed):
        attrs = [('c:type', boxed.ctype),
                 ('glib:name', boxed.name)]
        attrs.extend(self._boxed_attrs(boxed))
        with self.tagcontext('glib:boxed', attrs):
            self._write_boxed_ctors_methods(boxed)

    def _write_property(self, prop):
        attrs = [('name', prop.name)]
        # Properties are assumed to be readable
        if not prop.readable:
            attrs.append(('readable', '0'))
        if prop.writable:
            attrs.append(('writable', '1'))
        if prop.construct_only:
            attrs.append(('construct-only', '1'))
        with self.tagcontext('property', attrs):
            self._write_type(prop.type)

    def _write_callback(self, callback):
        # FIXME: reuse _write_function
        attrs = [('name', callback.name), ('c:type', callback.ctype)]
        self._append_deprecated(callback, attrs)
        with self.tagcontext('callback', attrs):
            self._write_return_type(callback.retval)
            self._write_parameters(callback.parameters)

    def _boxed_attrs(self, boxed):
        return [('glib:type-name', boxed.type_name),
                ('glib:get-type', boxed.get_type)]

    def _write_boxed_ctors_methods(self, boxed):
        for method in boxed.constructors:
            self._write_constructor(method)
        for method in boxed.methods:
            self._write_method(method)

    def _write_record(self, record):
        attrs = [('name', record.name),
                 ('c:type', record.symbol)]
        self._append_deprecated(record, attrs)
        if isinstance(record, GLibBoxed):
            attrs.extend(self._boxed_attrs(record))
        with self.tagcontext('record', attrs):
            if record.fields:
                for field in record.fields:
                    self._write_field(field)
            if isinstance(record, GLibBoxed):
                self._write_boxed_ctors_methods(record)

    def _write_union(self, union):
        attrs = [('name', union.name),
                 ('c:type', union.symbol)]
        self._append_deprecated(union, attrs)
        if isinstance(union, GLibBoxed):
            attrs.extend(self._boxed_attrs(union))
        with self.tagcontext('union', attrs):
            if union.fields:
                for field in union.fields:
                    self._write_field(field)
            if isinstance(union, GLibBoxed):
                self._write_boxed_ctors_methods(union)

    def _write_field(self, field):
        # FIXME: Just function
        if isinstance(field, (Callback, Function)):
            self._write_callback(field)
            return

        attrs = [('name', field.name)]
        with self.tagcontext('field', attrs):
            self._write_type(field.type)

    def _write_signal(self, signal):
        attrs = [('name', signal.name)]
        with self.tagcontext('glib:signal', attrs):
            self._write_return_type(signal.retval)
            self._write_parameters(signal.parameters)
