#ifndef NEMO_FITS_EXTENSION_H
#define NEMO_FITS_EXTENSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define NEMO_TYPE_FITS_EXTENSION  (nemo_fits_extension_get_type ())
#define NEMO_FITS_EXTENSION(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), NEMO_TYPE_FITS_EXTENSION, NemoFitsExtension))
#define NEMO_IS_FITS_EXTENSION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NEMO_TYPE_FITS_EXTENSION))

typedef struct _NemoFitsExtension      NemoFitsExtension;
typedef struct _NemoFitsExtensionClass NemoFitsExtensionClass;

GType nemo_fits_extension_get_type (void);

G_END_DECLS

#endif /* NEMO_FITS_EXTENSION_H */
