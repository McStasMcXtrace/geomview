/* Copyright (C) 1992-1998 The Geometry Center
 * Copyright (C) 1998-2000 Stuart Levy, Tamara Munzner, Mark Phillips
 *
 * This file is part of Geomview.
 * 
 * Geomview is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * Geomview is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Geomview; see the file COPYING.  If not, write
 * to the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139,
 * USA, or visit http://www.gnu.org.
 */

#include <ooglutil.h>
#include "3d.h"

extern int onlyverts;
extern char *headname, *tailname, *headxformname, *tailxformname, *graffitiname;
extern void AddVertex(Point3 *p);
extern int newvect;
extern int wrap;
extern short nverts;
extern Point3 verts[];
extern void Input(void);
extern void Close(void);
extern void DisplayPickInfoPanel(void);
extern void RemoveVertex(void);
extern void NewLine(void);
extern void DeleteHeadAndTail(void);
extern void EraseHeadAndTail(void);
extern void EraseHeadTail(char *xformname);
extern void EraseHead(void);
extern void StartNewVector(void);
extern void LangInit(IOBFILE *inf, FILE *fp);
extern void progn(void);
extern void ShowTailAt(Point3 *p);
extern void ShowHeadAt(Point3 *p, Point3 *prev);
extern void Geometry(void);
extern void endprogn(void);
extern void gui_init(void);
extern void gui_main_loop(IOBFILE *inf);


