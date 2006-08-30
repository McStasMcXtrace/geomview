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

 /*
  * Load a NPolyList object from an "nOFF"-format file.
  * 
  */

#include <string.h>
#include "npolylistP.h"

#define	SIZEOF_CMAP	256
static ColorA	*colormap;

static void
LoadCmap(char *file)
{
    IOBFILE *fp;
    colormap = OOGLNewNE(ColorA, 256, "PolyList colormap");
    if((file = findfile(NULL, file)) != NULL &&
       (fp = iobfopen(file,"r")) != NULL) {
	iobfgetnf(fp, SIZEOF_CMAP*4, (float *)colormap, 0);
	iobfclose(fp);
    }
}

NPolyList *
NPolyListFLoad(IOBFILE *file, char *fname)
{
    NPolyList *pl;
    int edges;
    int i;
    float *v;
    int binary = 0;
    int headerseen = 0;
    int flags = 0;
    int makenorm = 0;
    char *token;
    int dimn = 3, pdim;
    static ColorA black = { 0,0,0,0 };
    vvec vi;

    if (file == NULL)
	return NULL;

    token = GeomToken(file);
    if(strncmp(token, "ST", 2) == 0) {
	flags |= PL_HASST;
	token += 2;
    }
    if(*token == 'C') {
	flags = PL_HASVCOL;	/* Per-vertex colors */
	token++;
    }
    if(*token == '4') {
	dimn = 4;
	token++;
    }
    if(*token == 'S') {		/* "smooth surface": we compute vtx normals */
	makenorm = 1;
	token++;
    }
    if(strcmp(token, "nOFF") == 0) {
	headerseen = 1;
	if(iobfgetni(file, 1, &pdim, binary) == 0) {
	    OOGLSyntax(file, "nOFF %s: expected dimension", fname);
	    return NULL;
	}
	if(iobfnextc(file, 1) == 'B' && iobfexpectstr(file, "BINARY") == 0) {
	    binary = 1;
	    if(iobfnextc(file, 1) != '\n')	/* Expect \n after BINARY */
		return NULL;
	    (void) iobfgetc(file);		/* Toss \n */
	}
    } else {
	return NULL;
    }


    pl = OOGLNewE(NPolyList, "NPolyListFLoad NPolyList");
    GGeomInit(pl, NPolyListMethods(), NPLMAGIC, NULL);
    pl->p = NULL;
    pl->v = NULL;
    pl->vi = NULL;
    pl->vcol = NULL;
    pl->st = NULL;

    pl->flags = flags;
    pl->geomflags = (dimn == 4) ? VERT_4D : 0;

    pl->pdim = pdim + (dimn == 4 ? 0 : 1);


    if(iobfgetni(file, 1, &pl->n_verts, binary) <= 0 ||
       iobfgetni(file, 1, &pl->n_polys, binary) <= 0 ||
       iobfgetni(file, 1, &edges, binary) <= 0) {
		if(headerseen)
		    OOGLSyntax(file, "PolyList %s: Bad vertex/face/edge counts", fname);
		goto bogus;
    }
    VVINIT(vi, int, 5*pl->n_polys);

    pl->v = OOGLNewNE(float, pl->n_verts*pl->pdim, "NPolyListFLoad vertices");
    if(pl->flags & PL_HASVCOL)
	pl->vcol = OOGLNewNE(ColorA, pl->n_verts, "NPolyListFLoad vertex colors");
    if(pl->flags & PL_HASST)
	pl->st = OOGLNewNE(float, 2*pl->n_verts, "NPolyListFLoad texture coords");

    for(v = pl->v, i = 0; i < pl->n_verts; v += pl->pdim, i++) {
	if(iobfgetnf(file, pdim, v, binary) < pdim ||
	   ((flags & PL_HASVCOL) &&
		iobfgetnf(file, 4, (float *)&pl->vcol[i], binary) < 4) ||
	   ((flags & PL_HASST) &&
		iobfgetnf(file, 2, (float *)&pl->st[2*i], binary) < 2))
	{
		OOGLSyntax(file, "nOFF %s: Bad vertex %d (of %d)",
			fname, i, pl->n_verts);
		goto bogus;
	}
	if (dimn == 3) v[pl->pdim-1] = 1.0;
    }

    pl->p = OOGLNewNE(NPoly, pl->n_polys, "NPolyListFLoad polygons");
    for(i = 0; i < pl->n_polys; ) {
	NPoly *p;
	int k,index;

	p = &pl->p[i];
	if(iobfgetni(file, 1, &p->n_vertices, binary) <= 0 || p->n_vertices <= 0) {
	   OOGLSyntax(file, "PolyList %s: bad %d'th polygon (of %d)",
		fname, i, pl->n_polys);
	   goto bogus_face;
	}
	p->vi0 = VVCOUNT(vi);
	VVCOUNT(vi) += p->n_vertices;
	vvneeds(&vi, VVCOUNT(vi));
	i++;

	for(k = 0; k < p->n_vertices; k++) {
	    int index;

	    if(iobfgetni(file, 1, &index, binary) <= 0 ||
	       index < 0 || index >= pl->n_verts) {
		    OOGLSyntax(file, "PolyList: %s: bad index %d on %d'th polygon (of %d)", 
			fname, index, i, p->n_vertices);
		    goto bogus_face;
	    }
	    VVEC(vi, int)[k + p->vi0] = index;
	}

	/* Pick up the color, if any.
	 * In ASCII format, take whatever's left before end-of-line
	 */
	p->pcol = black;
	if(binary) {
	    int ncol;

	    if(iobfgetni(file, 1, &ncol, 1) <= 0
	       || iobfgetnf(file, ncol, (float *)&p->pcol, 1) < ncol)
		goto bogus_face_color;
	    k = ncol;
	} else {
	    for(k = 0; k < 4 && iobfnextc(file, 1) != '\n'; k++) {
		if(iobfgetnf(file, 1, ((float *)(&p->pcol))+k, 0) < 1)
		    goto bogus_face_color;
	    }
	}

	if((flags & PL_HASVCOL) == 0) {
	    if(k > 0)
		pl->flags |= PL_HASPCOL;

	    if(k != 1 && (p->pcol.r>1||p->pcol.g>1||p->pcol.b>1||p->pcol.a>1)) {
		p->pcol.r /= 255, p->pcol.g /= 255,
		p->pcol.b /= 255, p->pcol.a /= 255;
	    }

	    switch(k) {
	    case 0:
		p->pcol.r = 170/255.0;		/* Gray */
		p->pcol.g = p->pcol.r;
	    case 2:
		p->pcol.b = p->pcol.g;
	    case 3:
		p->pcol.a = 170/255.0;		/* Semitransparent */
		break;
	    case 1:				/* use colormap */
		if ( colormap == NULL )
			LoadCmap("cmap.fmap");
		index = p->pcol.r;
		if((unsigned)index >= SIZEOF_CMAP) index = 0;
		p->pcol = colormap[index];
	    }				/* case 4, all components supplied */
	}
    }

	/* Hand the list of vertex-indices on to the new object */
    vvtrim(&vi);
    pl->nvi = VVCOUNT(vi);
    pl->vi = VVEC(vi, int);
    return pl;

  
  bogus_face_color:
    OOGLSyntax(file, "PolyList %s: bad face color on face %d (of %d)",
	fname, i, pl->n_polys);
  bogus_face:
    pl->n_polys = i;
  bogus:
    GeomDelete((Geom *)pl);
    return NULL;
}

/*
 * Local Variables: ***
 * c-basic-offset: 4 ***
 * End: ***
 */
