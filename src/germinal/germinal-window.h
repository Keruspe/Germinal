/*
 * This file is part of Germinal.
 *
 * Copyright 2018 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
 *
 * Germinal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Germinal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Germinal.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GERMINAL_WINDOW_H__
#define __GERMINAL_WINDOW_H__

#include "germinal-terminal.h"

G_BEGIN_DECLS

#define GERMINAL_TYPE_WINDOW germinal_window_get_type ()
G_DECLARE_FINAL_TYPE (GerminalWindow, germinal_window, GERMINAL, WINDOW, GtkApplicationWindow)

GtkWidget *germinal_window_new (GtkApplication *application);

G_END_DECLS

#endif /* __GERMINAL_WINDOW_H__ */
