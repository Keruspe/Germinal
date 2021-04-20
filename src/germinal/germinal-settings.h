/*
 * This file is part of Germinal.
 *
 * Copyright 2021 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#ifndef __GERMINAL_SETTINGS_H__
#define __GERMINAL_SETTINGS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

GSettings *germinal_settings_new (void);

G_END_DECLS

#endif /* __GERMINAL_SETTINGS_H__ */
