/* plugin-intl.h @replacement: replaces/LICENSE.1  */

#ifndef __PLUGIN_INTL_H__
#define __PLUGIN_INTL_H__

#include "config.h"

#ifndef GETTEXT_PACKAGE
#error "GETTEXT_PACKAGE must be defined (config.h should define it)"
#endif

#include <libintl.h>

#ifdef _
#undef _
#endif
#ifdef Q_
#undef Q_
#endif

/* Use dgettext to specify the translation domain explicitly */
#define _(String) dgettext (GETTEXT_PACKAGE, String)

/* For strings that need context (GIMP 3.0 style) */
#define Q_(String) g_dpgettext (GETTEXT_PACKAGE, String, 0)

#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif

#endif /* __PLUGIN_INTL_H__ */
