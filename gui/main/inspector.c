/*

  Copyright (C) 2018 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include <string.h>

#define SU_LOG_DOMAIN "gui-inspector"

#include <confdb.h>
#include "modemctl.h"
#include "gui.h"

/************************** Inspector actions ********************************/
SUPRIVATE void
suscan_gui_on_open_inspector(
    SuGtkSpectrum *spect,
    gsufloat freq,
    const struct sigutils_channel *channel,
    gpointer data)
{
  struct suscan_gui_spectrum_action *action =
      (struct suscan_gui_spectrum_action *) data;

  /* Send open message. We will open new tab on response */
  SU_TRYCATCH(
      suscan_analyzer_open_async(
          action->gui->analyzer,
          action->insp_iface->name,
          channel,
          rand()),
      return);
}

SUPRIVATE SUBOOL
suscan_gui_add_inspector_action(
    suscan_gui_t *gui,
    const struct suscan_inspector_interface *insp_iface)
{
  char *action_text = NULL;
  struct suscan_gui_spectrum_action *action = NULL;
  SUBOOL ok = SU_FALSE;

  SU_TRYCATCH(action_text = strbuild("Open %s", insp_iface->desc), goto done);

  SU_TRYCATCH(
      action = calloc(1, sizeof(struct suscan_gui_spectrum_action)),
      goto done);

  action->gui = gui;
  action->insp_iface = insp_iface;

  SU_TRYCATCH(PTR_LIST_APPEND_CHECK(gui->action, action) != -1, goto done);

  (void) sugtk_spectrum_add_menu_action(
      gui->spectrum,
      action_text,
      suscan_gui_on_open_inspector,
      action);

  action = NULL;

  ok = SU_TRUE;

done:
  if (action_text != NULL)
    free(action_text);

  if (action != NULL)
    free(action);

  return ok;
}

SUBOOL
suscan_gui_add_all_inspector_actions(suscan_gui_t *gui)
{
  const struct suscan_inspector_interface **iface_list;
  unsigned int iface_count;
  unsigned int i;

  suscan_inspector_interface_get_list(&iface_list, &iface_count);

  for (i = 0; i < iface_count; ++i)
    SU_TRYCATCH(
        suscan_gui_add_inspector_action(gui, iface_list[i]),
        return SU_FALSE);

  return SU_TRUE;
}

/************************ Inspector handling methods *************************/
SUBOOL
suscan_gui_remove_inspector(
    suscan_gui_t *gui,
    suscan_gui_inspector_t *insp)
{
  gint num;
  int index = insp->index;
  if (index < 0 || index >= gui->inspector_count)
    return SU_FALSE;

  SU_TRYCATCH(gui->inspector_list[index] == insp, return SU_FALSE);

  SU_TRYCATCH(
      (num = gtk_notebook_page_num(
          gui->analyzerViewsNotebook,
          GTK_WIDGET(insp->channelInspectorGrid))) != -1,
      return SU_FALSE);

  gtk_notebook_remove_page(gui->analyzerViewsNotebook, num);

  gui->inspector_list[index] = NULL;

  return SU_TRUE;
}

SUBOOL
suscan_gui_add_inspector(
    suscan_gui_t *gui,
    suscan_gui_inspector_t *insp)
{
  gint page;
  SUBOOL inspector_added = SU_FALSE;

  SU_TRYCATCH(
      (insp->index = PTR_LIST_APPEND_CHECK(gui->inspector, insp)) != -1,
      goto fail);

  inspector_added = SU_TRUE;
  insp->_parent.gui = gui;

  SU_TRYCATCH(
      (page = gtk_notebook_append_page_menu(
          gui->analyzerViewsNotebook,
          GTK_WIDGET(insp->channelInspectorGrid),
          GTK_WIDGET(insp->pageLabelEventBox),
          NULL)) >= 0,
      goto fail);

  gtk_notebook_set_tab_reorderable(
      gui->analyzerViewsNotebook,
      GTK_WIDGET(insp->pageLabelEventBox),
      TRUE);

  gtk_notebook_set_current_page(gui->analyzerViewsNotebook, page);

  return TRUE;

fail:
  if (inspector_added)
    (void) suscan_gui_remove_inspector(gui, insp);

  return FALSE;
}

suscan_gui_inspector_t *
suscan_gui_get_inspector(const suscan_gui_t *gui, uint32_t inspector_id)
{
  if (inspector_id >= gui->inspector_count)
    return NULL;

  return gui->inspector_list[inspector_id];
}

void
suscan_gui_detach_all_inspectors(suscan_gui_t *gui)
{
  unsigned int i;

  for (i = 0; i < gui->inspector_count; ++i)
    if (gui->inspector_list[i] != NULL)
      suscan_gui_inspector_detach(gui->inspector_list[i]);
}

