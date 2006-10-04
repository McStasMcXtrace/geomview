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


/* Authors: Charlie Gunn, Stuart Levy, Tamara Munzner, Mark Phillips */

#ifndef	POLYLISTDEFS
#define	POLYLISTDEFS
/*
 * Public definitions for PolyList's
 */

#include "bbox.h"

typedef struct PolyList PolyList;

extern PolyList *PolyListDraw( PolyList * );
extern BBox     *PolyListBound(PolyList *polylist,
			       Transform T, TransformN *TN, int *axes);
extern Geom 	*PolyListSphere( PolyList *,
				 Transform, TransformN *TN, int *axes, int );
extern PolyList *PolyListEvert( PolyList * );
extern PolyList *PolyListLoad( char * );	/* Loads "off" files */
extern PolyList *PolyListFLoad( IOBFILE *, char *fname );
extern char    *PolyListName( void );
extern PolyList *PolyListSave( PolyList *, char * );
extern PolyList *PolyListFSave( PolyList *, FILE *, char * );
extern PolyList *PolyListTransform( PolyList *, Transform, TransformN * );

#endif/*POLYLISTDEFS*/
