/*
    This file is part of darktable,
    copyright (c) 2009--2010 johannes hanika.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DT_LIB_H
#define DT_LIB_H

#include "views/view.h"
#include "common/darktable.h"
#include <gmodule.h>
#include <gtk/gtk.h>

struct dt_lib_module_t;
struct dt_colorpicker_sample_t;

/** struct responsible for all library related shared routines and plugins. */
typedef struct dt_lib_t
{
  GList *plugins;
  struct dt_lib_module_t *gui_module;

  /** Proxy functions for communication with views */
  struct
  {
    /** Colorpicker plugin hooks */
    struct
    {
      struct dt_lib_module_t *module;
      uint8_t *picked_color_rgb_mean;
      uint8_t *picked_color_rgb_min;
      uint8_t *picked_color_rgb_max;
      float *picked_color_lab_mean;
      float *picked_color_lab_min;
      float *picked_color_lab_max;
      GSList *live_samples;
      struct dt_colorpicker_sample_t *selected_sample;
      int size;
      int display_samples;
      int restrict_histogram;
      void (*update_panel)(struct dt_lib_module_t *self);
      void (*update_samples)(struct dt_lib_module_t *self);
    } colorpicker;

  } proxy;
}
dt_lib_t;

typedef struct dt_lib_module_t
{
  /** opened module. */
  GModule *module;
  /** reference for dlopened libs. */
  darktable_t *dt;
  /** other stuff that may be needed by the module, not only in gui mode. */
  void *data;
  /** string identifying this operation. */
  char plugin_name[128];
  /** child widget which is added to the GtkExpander. */
  GtkWidget *widget;
  /** expander containing the widget. */
  GtkExpander *expander;

  /** get name of the module, to be translated. */
  const char* (*name)     ();
  /** get the views which the module should be loaded in. */
  uint32_t (*views)       ();
  /** get the container which the module should be placed in */
  uint32_t (*container)   ();
  /** check if module should use a expander or not, default implementation
      will make the module expandable and storing the expanding state, 
      if not the module will always be shown without the expander. */
  int (*expandable) ();

  /** callback methods for gui. */
  /** construct widget. */
  void (*gui_init)        (struct dt_lib_module_t *self);
  /** destroy widget. */
  void (*gui_cleanup)     (struct dt_lib_module_t *self);
  /** reset to defaults. */
  void (*gui_reset)       (struct dt_lib_module_t *self);

  /** optional event callbacks for big center widget. */
  /** optional method called after lighttable expose. */
  void (*gui_post_expose) (struct dt_lib_module_t *self, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery);
  int  (*mouse_leave)     (struct dt_lib_module_t *self);
  int  (*mouse_moved)     (struct dt_lib_module_t *self, double x, double y, int which);
  int  (*button_released) (struct dt_lib_module_t *self, double x, double y, int which, uint32_t state);
  int  (*button_pressed)  (struct dt_lib_module_t *self, double x, double y, int which, int type, uint32_t state);
  int  (*scrolled)        (struct dt_lib_module_t *self, double x, double y, int up);
  void (*configure)       (struct dt_lib_module_t *self, int width, int height);
  int  (*position)        ();
  /** implement these three if you want customizable presets to be stored in db. */
  void* (*get_params)     (struct dt_lib_module_t *self, int *size);
  int   (*set_params)     (struct dt_lib_module_t *self, const void *params, int size);
  void  (*init_presets)   (struct dt_lib_module_t *self);
  /** Optional callback for keyboard accelerators */
  void (*init_key_accels) (struct dt_lib_module_t *self);
}
dt_lib_module_t;

void dt_lib_init(dt_lib_t *lib);
void dt_lib_cleanup(dt_lib_t *lib);

/** loads and inits the modules in the libs/ directory. */
int dt_lib_load_modules();
/** calls module->cleanup and closes the dl connection. */
void dt_lib_unload_module(dt_lib_module_t *module);
/** creates a label widget for the expander, with callback to enable/disable this module. */
GtkWidget *dt_lib_gui_get_expander(dt_lib_module_t *module);


/** preset stuff for lib */

/** add or replace a preset for this operation. */
void dt_lib_presets_add(const char *name, const char *plugin_name, const void *params, const int32_t params_size);

#endif
