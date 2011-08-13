/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 */

#include "girepository.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
test_enum_and_flags_cidentifier(GIRepository *repo)
{
    GITypelib *ret;
    GError *error = NULL;
    gint n_infos, i;

    ret = g_irepository_require (repo, "GIMarshallingTests", NULL, 0, &error);
    if (!ret)
        g_error ("%s", error->message);

    n_infos = g_irepository_get_n_infos (repo, "GIMarshallingTests");

    for (i = 0; i < n_infos; i++) {
        GIBaseInfo *info;

        info = g_irepository_get_info (repo, "GIMarshallingTests", i);

        /* both GI_INFO_TYPE_ENUM and GI_INFO_TYPE_FLAGS use GIEnumInfo */
        if (GI_IS_ENUM_INFO (info)) {
            gint n_values, j;

            n_values = g_enum_info_get_n_values ((GIEnumInfo *) info);
            for (j = 0; j < n_values; j++) {
                GIValueInfo *value_info;
                const gchar *c_identifier = NULL;

                value_info = g_enum_info_get_value ((GIEnumInfo *) info, j);
                c_identifier = g_base_info_get_attribute ((GIBaseInfo *) value_info, "c:identifier");

                if (c_identifier == NULL) {
                    g_error ("Error: no 'c:identifier' attribute on GIMarshallingTests.%s.%s\n",
                             g_base_info_get_name (info),
                             g_base_info_get_name ((GIBaseInfo *) value_info));
                }

                g_base_info_unref ((GIBaseInfo *) value_info);
            }
        }

        g_base_info_unref (info);
    }
}

static void
_check_enum_methods (GIBaseInfo *info, const gchar *name, const gchar *prefix)
{
    gint n_methods, i;

    n_methods = g_enum_info_get_n_methods ((GIEnumInfo *) info);
    if (n_methods <= 0)
        g_error ("%s should have methods", name);

    for (i = 0; i < n_methods; i += n_methods-1) {
        GIBaseInfo *function_info;
        GIFunctionInfoFlags flags;
        const gchar *symbol;
        function_info = g_enum_info_get_method ((GIEnumInfo *) info, i);
        if (!function_info)
            g_error ("Could not find %s method nr. %d", name, i+1);
        flags = g_function_info_get_flags ((GIFunctionInfo *) function_info);
        if (flags != 0)
            g_error ("%s methods should be static", name);
        symbol = g_function_info_get_symbol ((GIFunctionInfo *) function_info);
        if (!symbol || !g_str_has_prefix (symbol, prefix))
            g_error ("Could not find valid function symbol");
        g_base_info_unref (function_info);
    }
}

static void
test_enum_and_flags_static_methods(GIRepository *repo)
{
    GITypelib *ret;
    GError *error = NULL;
    GIBaseInfo *enum_info;

    ret = g_irepository_require (repo, "GIMarshallingTests", NULL, 0, &error);
    if (!ret)
        g_error ("%s", error->message);

    enum_info = g_irepository_find_by_name (repo, "GIMarshallingTests", "GEnum");
    if (!enum_info)
        g_error ("Could not find GIMarshallingTests.GEnum");
    _check_enum_methods (enum_info,
                         "GIMarshallingTests.GEnum",
                         "gi_marshalling_tests_genum_");
    g_base_info_unref (enum_info);

    enum_info = g_irepository_find_by_name (repo, "GIMarshallingTests", "Flags");
    if (!enum_info)
        g_error ("Could not find GIMarshallingTests.Flags");
    _check_enum_methods (enum_info,
                         "GIMarshallingTests.Flags",
                         "gi_marshalling_tests_flags_");
    g_base_info_unref (enum_info);
}

int
main(int argc, char **argv)
{
    GIRepository *repo;

    g_type_init ();

    repo = g_irepository_get_default ();

    /* do tests */
    test_enum_and_flags_cidentifier (repo);
    test_enum_and_flags_static_methods (repo);

    exit(0);
}
