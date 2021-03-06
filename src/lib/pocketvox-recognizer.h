#ifndef __POCKETVOX_RECOGNIZER_H__
#define __POCKETVOX_RECOGNIZER_H__

#include <pocketsphinx.h>
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>
#include "pocketvox-types.h"

G_BEGIN_DECLS

#define TYPE_POCKETVOX_RECOGNIZER                  (pocketvox_recognizer_get_type ())
#define POCKETVOX_RECOGNIZER(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_POCKETVOX_RECOGNIZER, PocketvoxRecognizer))
#define IS_POCKETVOX_RECOGNIZER(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_POCKETVOX_RECOGNIZER))
#define POCKETVOX_RECOGNIZER_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_POCKETVOX_RECOGNIZER, PocketvoxRecognizerClass))
#define IS_POCKETVOX_RECOGNIZER_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_POCKETVOX_RECOGNIZER))
#define POCKETVOX_RECOGNIZER_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_POCKETVOX_RECOGNIZER, PocketvoxRecognizerClass))

typedef struct _PocketvoxRecognizer                 PocketvoxRecognizer;
typedef struct _PocketvoxRecognizerPrivate          PocketvoxRecognizerPrivate;
typedef struct _PocketvoxRecognizerClass            PocketvoxRecognizerClass;

struct _PocketvoxRecognizer
{
  /*< private >*/
  GInitiallyUnowned parent_instance;

  PocketvoxRecognizerPrivate *priv;
};

struct _PocketvoxRecognizerClass
{
  /*< private >*/
  GInitiallyUnownedClass parent_class;

  /*public funcs and data here*/
};

GType pocketvox_recognizer_get_type (void) G_GNUC_CONST;

PocketvoxRecognizer* pocketvox_recognizer_new(gchar* hmm, gchar* lm, gchar* dic, gchar *key, gchar *mat, gchar *dev, gchar* host, gint port);

void pocketvox_recognizer_set_state(PocketvoxRecognizer *recognizer,PocketvoxState state);

void pocketvox_recognizer_set(PocketvoxRecognizer *recognizer, gchar* ppt, gchar *path);

void pocketvox_recognizer_set_keyword(PocketvoxRecognizer *recognizer, gchar *keyword);

G_END_DECLS
#endif
