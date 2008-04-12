/**
 * Gtk.c  -- Framework initialization code.
 *
 * Copyright (C) 2007, 2008  Imendio AB
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach-o/dyld.h>
#include <Carbon/Carbon.h>

#include <glib.h>
#include <gtk/gtk.h>

/* Relative location of the framework in an application bundle */
#define FRAMEWORK_OFFSET "../Frameworks/Gtk.framework"

#define RELATIVE_PANGO_RC_FILE			"/Resources/etc/pango/pangorc"
#define RELATIVE_GTK_RC_FILE			"/Resources/etc/gtk-2.0/gtkrc"
#define RELATIVE_GTK_IMMODULE_FILE		"/Resources/etc/gtk-2.0/gtk.immodules"
#define RELATIVE_GDK_PIXBUF_MODULE_FILE		"/Resources/etc/gtk-2.0/gdk-pixbuf.loaders"

/* Define these private functions from ige-mac-integration here, so we don't
 * have to install a private header.
 */
gboolean _ige_mac_menu_is_quit_menu_item_handled (void);
gboolean _ige_mac_dock_is_quit_menu_item_handled (void);

/* Make the bundle prefix available to the outside */
static char *bundle_prefix = NULL;

const char *_gtk_framework_get_bundle_prefix (void)
{
  return bundle_prefix;
}


static void set_rc_environment (const char *variable,
                                const char *bundle_prefix,
                                const char *relative_file)
{
  char *rc_file;

  rc_file = g_strdup_printf ("%s%s", bundle_prefix, relative_file);
  setenv (variable, rc_file, 1);
  g_free (rc_file);
}

static int is_running_from_app_bundle (void)
{
  int i;

  /* Here we use some dyld magic to figure out whether we are running
   * against the system's Gtk.framework or a Gtk.framework found bundled
   * in the application bundle.
   */

  for (i = 0; i < _dyld_image_count (); i++)
    {
      const char *name = _dyld_get_image_name (i);

      if (strstr (name, "Gtk.framework/Gtk"))
        {
          if (!strncmp (name, "/Library/", 9))
            return 0;
          else
            return 1;
        }
    }

  /* shouldn't be reached ... */
  return 0;
}

static OSErr
handle_quit_cb (const AppleEvent *inAppleEvent,
                AppleEvent       *outAppleEvent,
                long              inHandlerRefcon)
{
  /* We only quit if there is no menu or dock setup. */
  if (!_ige_mac_menu_is_quit_menu_item_handled () &&
      !_ige_mac_dock_is_quit_menu_item_handled ())
    gtk_main_quit ();

  return noErr;
}

__attribute__((constructor))
static void initializer (int argc, char **argv, char **envp)
{
  int i;

  /* Figure out correct bundle prefix */
  if (!is_running_from_app_bundle ())
    {
      bundle_prefix = g_strdup ("/Library/Frameworks/Gtk.framework");
    }
  else
    {
      char *executable_path;

      executable_path = g_strdup (argv[0]);
      for (i = strlen (executable_path); i >= 0; i--)
        {
          if (executable_path[i] == '/')
            break;

          executable_path[i] = 0;
        }

      bundle_prefix = g_strdup_printf ("%s/%s", executable_path, FRAMEWORK_OFFSET);

      g_free (executable_path);
    }

  /* NOTE: leave the setting of the environment variables in this order,
   * otherwise things will fail in the Release builds for no obvious
   * reason.
   */
  set_rc_environment ("PANGO_RC_FILE", bundle_prefix, RELATIVE_PANGO_RC_FILE);
  set_rc_environment ("GTK2_RC_FILES", bundle_prefix, RELATIVE_GTK_RC_FILE);
  set_rc_environment ("GTK_IM_MODULE_FILE", bundle_prefix, RELATIVE_GTK_IMMODULE_FILE);
  set_rc_environment ("GDK_PIXBUF_MODULE_FILE", bundle_prefix, RELATIVE_GDK_PIXBUF_MODULE_FILE);

  set_rc_environment ("XDG_CONFIG_DIRS", bundle_prefix, "/Resources/etc/xdg");
  set_rc_environment ("XDG_DATA_DIRS", bundle_prefix, "/Resources/share");
  set_rc_environment ("GTK_DATA_PREFIX", bundle_prefix, "/Resources/share");
  set_rc_environment ("GTK_EXE_PREFIX", bundle_prefix, "/Resources");
  set_rc_environment ("GTK_PATH", bundle_prefix, "/Resources");

  /* We don't free bundle_prefix; it needs to be available until
   * program termination.
   */

  /* Handle quit menu item so that apps can be quit of of the box. */
  AEInstallEventHandler (kCoreEventClass, kAEQuitApplication,
                         handle_quit_cb,
                         0, true);
}
