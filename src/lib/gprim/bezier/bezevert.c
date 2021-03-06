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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#if 0
static char copyright[] = "Copyright (C) 1992-1998 The Geometry Center\n\
Copyright (C) 1998-2000 Stuart Levy, Tamara Munzner, Mark Phillips";
#endif


/* Authors: Charlie Gunn, Stuart Levy, Tamara Munzner, Mark Phillips */

#include "bezierP.h"

/*
 * evert patch by transposing control points.
 * If mesh has been computed, also call MeshEvert
 * Next time mesh is computed, its normals will be correct.
 */
Bezier *
BezierEvert( bezier )
Bezier *bezier;
{
#ifdef FOO
    int index0, index1, i,j,k;

    /* index0 will be linear index of control point... */
    for( i = 0; i< bezier->degree+1; ++i)
        for( j = i+1; j< bezier->degree+1; ++j)
		{
		/* ... compute index1 for transpose control point */
		index1 = bezier->dimn * (j * (bezier->degree+1) + i);
		index0 = bezier->dimn * (i * (bezier->degree+1) + j);
        	for( k = 0; k< bezier->dimn; ++k)
		    {
		    tmp = bezier->CtrlPnts[index0+k];
		    bezier->CtrlPnts[index0+k] = bezier->CtrlPnts[index1+k];
		    bezier->CtrlPnts[index1+k] = tmp;
		    }
		}
#endif
    if (bezier->geomflags & BEZ_REMESH || bezier->mesh == NULL)
	BezierReDice(bezier);
    MeshEvert(bezier->mesh);

    return bezier;
}

