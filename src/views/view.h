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
#ifndef DT_VIEW_H
#define DT_VIEW_H

#include "common/image.h"
#include <inttypes.h>
#include <gui/gtk.h>
#include <gmodule.h>
#include <cairo.h>
#include <sqlite3.h>

/** avilable views flags, a view should return it's type and
    is also used in modules flags available in src/libs to
    control which view the module should be available in also
    which placement in the panels the module have.
*/
enum dt_view_type_flags_t {
  DT_VIEW_LIGHTTABLE = 1,
  DT_VIEW_DARKROOM = 2,
  DT_VIEW_TETHERING = 4,
};

/**
 * main dt view module (as lighttable or darkroom)
 */
struct dt_view_t;
typedef struct dt_view_t
{
  char module_name[64];
  // dlopened module
  GModule *module;
  // custom data for module
  void *data;
  // width and height of allocation
  uint32_t width, height;
  // scroll bar control
  float vscroll_size, vscroll_viewport_size, vscroll_pos;
  float hscroll_size, hscroll_viewport_size, hscroll_pos;
  const char *(*name)     (struct dt_view_t *self); // get translatable name
  uint32_t (*view)        (struct dt_view_t *self); // get the view type
  void (*init)            (struct dt_view_t *self); // init *data
  void (*cleanup)         (struct dt_view_t *self); // cleanup *data
  void (*expose)          (struct dt_view_t *self, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery); // expose the module (gtk callback)
  int  (*try_enter)       (struct dt_view_t *self); // test if enter can succeed.
  void (*enter)           (struct dt_view_t *self); // mode entered, this module got focus. return non-null on failure.
  void (*leave)           (struct dt_view_t *self); // mode left (is called after the new try_enter has succeded).
  void (*reset)           (struct dt_view_t *self); // reset default appearance

  // event callbacks:
  int  (*mouse_enter)     (struct dt_view_t *self);
  int  (*mouse_leave)     (struct dt_view_t *self);
  int  (*mouse_moved)     (struct dt_view_t *self, double x, double y, int which);
  int  (*button_released) (struct dt_view_t *self, double x, double y, int which, uint32_t state);
  int  (*button_pressed)  (struct dt_view_t *self, double x, double y, int which, int type, uint32_t state);
  int  (*key_pressed)     (struct dt_view_t *self, guint key, guint state);
  int  (*key_released)    (struct dt_view_t *self, guint key, guint state);
  void (*configure)       (struct dt_view_t *self, int width, int height);
  void (*scrolled)        (struct dt_view_t *self, double x, double y, int up, int state); // mouse scrolled in view
  void (*border_scrolled) (struct dt_view_t *self, double x, double y, int which, int up); // mouse scrolled on left/right/top/bottom border (which 0123).
}
dt_view_t;

typedef enum dt_view_image_over_t
{
  DT_VIEW_DESERT = 0,
  DT_VIEW_STAR_1 = 1,
  DT_VIEW_STAR_2 = 2,
  DT_VIEW_STAR_3 = 3,
  DT_VIEW_STAR_4 = 4,
  DT_VIEW_STAR_5 = 5,
  DT_VIEW_REJECT = 6
}
dt_view_image_over_t;

/** expose an image, set image over flags. */
void dt_view_image_expose(dt_image_t *img, dt_view_image_over_t *image_over, int32_t index, cairo_t *cr, int32_t width, int32_t height, int32_t zoom, int32_t px, int32_t py);

/** Set the selection bit to a given value for the specified image */
void dt_view_set_selection(int imgid, int value);
/** toggle selection of given image. */
void dt_view_toggle_selection(int imgid);

#define DT_VIEW_MAX_MODULES 10
/**
 * holds all relevant data needed to manage the view
 * modules.
 */
typedef struct dt_view_manager_t
{
  dt_view_t film_strip;
  dt_view_t view[DT_VIEW_MAX_MODULES];
  int32_t current_view, num_views;
  int32_t film_strip_on;
  float film_strip_size;
  int32_t film_strip_dragging, film_strip_scroll_to, film_strip_active_image;
  void (*film_strip_activated)(const int imgid, void *data);
  void *film_strip_data;

  /* reusable db statements 
   * TODO: reconsider creating a common/database helper API
   *       instead of having this spread around in sources..
   */
  struct {
    /* select num from history where imgid = ?1*/
    sqlite3_stmt *have_history;
    /* select * from selected_images where imgid = ?1 */
    sqlite3_stmt *is_selected;
    /* delete from selected_images where imgid = ?1 */
    sqlite3_stmt *delete_from_selected;
    /* insert into selected_images values (?1) */
    sqlite3_stmt *make_selected;
    /* select color from color_labels where imgid=?1 */
    sqlite3_stmt *get_color;

  } statements;


  /*
   * Proxy
   */
  struct {

    /* view toolbox proxy object */
    struct {
      struct dt_lib_module_t *module;
      void (*add)(struct dt_lib_module_t *,GtkWidget *);
    } view_toolbox;

  } proxy;


}
dt_view_manager_t;

void dt_view_manager_init(dt_view_manager_t *vm);
void dt_view_manager_cleanup(dt_view_manager_t *vm);

/** return translated name. */
const char *dt_view_manager_name (dt_view_manager_t *vm);
/** switch to this module. returns non-null if the module fails to change. */
int dt_view_manager_switch(dt_view_manager_t *vm, int k);
/** expose current module. */
void dt_view_manager_expose(dt_view_manager_t *vm, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery);
/** reset current view. */
void dt_view_manager_reset(dt_view_manager_t *vm);
/** get current view of the view manager. */
const dt_view_t *dt_view_manager_get_current_view(dt_view_manager_t *vm);

void dt_view_manager_mouse_enter     (dt_view_manager_t *vm);
void dt_view_manager_mouse_leave     (dt_view_manager_t *vm);
void dt_view_manager_mouse_moved     (dt_view_manager_t *vm, double x, double y, int which);
int dt_view_manager_button_released  (dt_view_manager_t *vm, double x, double y, int which, uint32_t state);
int dt_view_manager_button_pressed   (dt_view_manager_t *vm, double x, double y, int which, int type, uint32_t state);
int dt_view_manager_key_pressed      (dt_view_manager_t *vm, guint key, guint state);
int dt_view_manager_key_released     (dt_view_manager_t *vm, guint key, guint state);
void dt_view_manager_configure       (dt_view_manager_t *vm, int width, int height);
void dt_view_manager_scrolled        (dt_view_manager_t *vm, double x, double y, int up, int state);
void dt_view_manager_border_scrolled (dt_view_manager_t *vm, double x, double y, int which, int up);

/** add widget to the current view toolbox */
void dt_view_manager_view_toolbox_add(dt_view_manager_t *vm,GtkWidget *tool);

/** load module to view managers list, if still space. return slot number on success. */
int dt_view_manager_load_module(dt_view_manager_t *vm, const char *mod);
/** load a view module */
int dt_view_load_module(dt_view_t *view, const char *module);
/** unload, cleanup */
void dt_view_unload_module(dt_view_t *view);
/** set scrollbar positions, gui method. */
void dt_view_set_scrollbar(dt_view_t *view, float hpos, float hsize, float hwinsize, float vpos, float vsize, float vwinsize);
/** open up the film strip view, with given callback on image activation. */
void dt_view_film_strip_open(dt_view_manager_t *vm, void (*activated)(const int, void*), void *data);
/** close the film strip view. */
void dt_view_film_strip_close(dt_view_manager_t *vm);
/** toggles the film strip. */
void dt_view_film_strip_toggle(dt_view_manager_t *vm, void (*activated)(const int imgid, void*), void *data);
/** advise the film strip to scroll to imgid at next expose. */
void dt_view_film_strip_scroll_to(dt_view_manager_t *vm, const int imgid);
/** prefetch the next few images in film strip, from selected on. */
void dt_view_film_strip_prefetch();
/** Clears all selection and selects the given image as active image in film strip. */
void dt_view_film_strip_set_active_image(dt_view_manager_t *vm,int iid);
/** Gets the active image id in filmstrip */
uint32_t dt_view_film_strip_get_active_image(dt_view_manager_t *vm);

#endif
