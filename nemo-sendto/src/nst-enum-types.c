


#include <nemo-sendto-plugin.h>
#include "nst-enum-types.h"
#include <glib-object.h>

/* enumerations from "nemo-sendto-plugin.h" */
GType
nst_plugin_capabilities_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { NEMO_CAPS_NONE, "NEMO_CAPS_NONE", "none" },
      { NEMO_CAPS_SEND_DIRECTORIES, "NEMO_CAPS_SEND_DIRECTORIES", "send-directories" },
      { NEMO_CAPS_SEND_IMAGES, "NEMO_CAPS_SEND_IMAGES", "send-images" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("NstPluginCapabilities", values);
  }
  return etype;
}



