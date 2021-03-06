/* Copyright (C) 1992-1998 The Geometry Center
 * Copyright (C) 1998-2000 Geometry Technologies, Inc.
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

#if 0
static char copyright[] = "Copyright (C) 1992-1998 The Geometry Center\n\
Copyright (C) 1998-2000 Geometry Technologies, Inc.";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <tcl.h>
#include <tk.h>
#include <transformn.h>
#include <camera.h>
#include <string.h>

static IOBFILE *infile;

#define EPSILON 1e-8

int send_basis()
{
  return TCL_OK;
}

int set_update_mode()
{
  return TCL_OK;
}
enum update_style_enum { NONE, SINGLE, CONTINUOUS } update_style = NONE;
Tk_TimerToken update_timertoken;

void spanproc(int);

void update_basis_vectors_once()
{
  spanproc(0);
}

void update_basis_vectors(ClientData clientData)
{
  update_basis_vectors_once();
  update_timertoken = Tk_CreateTimerHandler(500, update_basis_vectors, 0);
}

int do_spanproc(ClientData clientData, Tcl_Interp *interp,
		int ac, const char **av)
{
  if (ac != 2) {
    Tcl_SetResult(interp, "usage: spanproc <value>", TCL_STATIC);
    return TCL_ERROR;
  }
  spanproc(atoi(av[1]));
  return TCL_OK;
}

int ndview_set_update(ClientData clientData, Tcl_Interp *interp,
		      int ac, const char **av)
{
  if (ac != 2 || (strcmp(av[1],"continuous") && strcmp(av[1],"single") && strcmp(av[1],"none"))) {
    Tcl_SetResult(interp, "usage: ndview_set_update continuous|single|none", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!strcmp(av[1],"continuous")) {
    if (update_style != CONTINUOUS)
      update_basis_vectors(0);
    update_style = CONTINUOUS;
  } else if (!strcmp(av[1],"none")) {
    if (update_style == CONTINUOUS)
      Tk_DeleteTimerHandler(update_timertoken);
    update_basis_vectors_once();
    update_style = SINGLE;
  } else {
    if (update_style == CONTINUOUS)
      Tk_DeleteTimerHandler(update_timertoken);
    update_style = NONE;
  }
  return TCL_OK;
}
Tcl_Interp *ourinterp = NULL;
int ndview_c_exists(ClientData clientData, Tcl_Interp *interp,
		    int ac, const char **av)
{
  ourinterp = interp;
  return TCL_OK;
}

void update_dimension();

int ndview_update_dimension(ClientData clientData, Tcl_Interp *interp,
			    int ac, const char **av)
{
  update_dimension();
  return TCL_OK;
}

int ndview_init(Tcl_Interp *interp)
{
  infile = iobfileopen(stdin);

  Tcl_CreateCommand(interp, "ndview_c_exists", ndview_c_exists, 0, 0);
  Tcl_CreateCommand(interp, "ndview_spanproc", do_spanproc, 0, 0);
  Tcl_CreateCommand(interp, "ndview_set_update", ndview_set_update, 0, 0);
  Tcl_CreateCommand(interp, "ndview_update_dimension", ndview_update_dimension, 0, 0);
  return TCL_OK;
}

/* now we have strange stuff ported from the old NDview. */

int dimension=0;
int pdim = 1;

void dotcl(char *command)
{
  if (!ourinterp)
    ourinterp = Tcl_CreateInterp();
  if (Tcl_GlobalEval(ourinterp, command)!=TCL_OK)
    fprintf(stderr,"%s\n",ourinterp->result);
}

char *get_entry(char *entry)
{
  char buf[1024];
  sprintf(buf,"%s get",entry);
  if (Tcl_GlobalEval(ourinterp,buf) != TCL_OK) {
    fprintf(stderr,"%s\n",ourinterp->result);
    return "";
  }
  return ourinterp->result;
}

void update_dimension()
{
  char buf[100];
  int newdimension;

  iobfrewind(infile);
  printf("(echo (dimension)\\n)\n"); fflush(stdout);
  iobfgets(buf, 10, infile); newdimension = atoi(buf);
  if(newdimension < 3) {
    newdimension = 3;   /* if gv in not ND yet */
    pdim = 4;
  }
  if(dimension != newdimension) {
    dimension = newdimension;
    pdim = dimension+1;
  }
  sprintf(buf,"set dimension %d",newdimension);
  dotcl(buf);
}

int gram_schmidt( TransformN *T, float **order )
{
  float *a, r, *point;
  int i, j, k, currentrow = 0;

  a = order[currentrow]+1;
  for ( k = 1; k < pdim; k++) {
    for( r = 0.0, i = dimension; i; i--, a++)
      r += *a * *a;
    if ( r < EPSILON ) return 0;
    r = sqrt(r);
    a--;
    for (  i = dimension; i; i--)
      *a-- /= r;
    point = order[currentrow]+1;
    for ( j = k + 1; j < pdim; j++) {
      r = 0.0;
      a = order[currentrow + j - k]+1;
      for ( i = dimension; i; i--)
	r += *point++ * *a++;
      for ( i = dimension; i; i--)
	*(--a) -= *(--point) * r;
    }
    a = order[++currentrow]+1;
  }
  return 1;
}

int return_new_matrix(TransformN *O2C, TransformN *O2W, TransformN *C2U, TransformN *W2U, int xdim, int ydim, int zdim, int objectflag)
{
  TransformN *U2W = NULL, *tmp = NULL;
  float **rearranged;
  int j = 0, k;

  rearranged = calloc(dimension, sizeof(float *));
  rearranged[j++] = &(O2C->a[xdim*pdim]);
  rearranged[j++] = &(O2C->a[ydim*pdim]);
  rearranged[j++] = &(O2C->a[zdim*pdim]);
  for ( k = 1; k < pdim; k++)
    if ( k != xdim && k != ydim && k != zdim ) 
      rearranged[j++] = &(O2C->a[k*pdim]);
  if ( gram_schmidt(O2C, rearranged) == 0) {
    /* got an almost zero matrix, don't return anything! */
    free(rearranged);
    return 0;
  }
  free(rearranged);

  U2W = TmNInvert(W2U, U2W);
  if ( objectflag ) {		/* compute a new O2W */
    /* we get this from O2C * C2U * U2W  */
    tmp = TmNConcat(O2C, C2U, tmp);
    O2W = TmNConcat(tmp, U2W, O2W);
    printf("(ND-xform-set targetgeom %d %d ",pdim,pdim);
    for ( j = 0; j < pdim; j++ )
      for ( k = 0; k < pdim; k++ )
	printf("%.2f ",O2W->a[j*pdim+k]);
    printf(")\n");
    fflush(stdout);
  } else {			/* compute a new C2U */
    /* this we get as the inverse of U2W * 
       O2C  (which is really W2C in this case)  */
    tmp = TmNConcat(U2W, O2C, tmp);
    C2U = TmNInvert(tmp, C2U);
    printf("(ND-xform-set focus %d %d ",pdim,pdim);
    for ( j = 0; j < pdim; j++ )
      for ( k = 0; k < pdim; k++ )
	printf("%.2f ",C2U->a[j*pdim+k]);
    printf(")\n");
    fflush(stdout);
  }
  return 1;
}

void spanproc(int i)
{
  TransformN *O2W=NULL, *W2U=NULL, *C2U=NULL, *U2C=NULL;
  TransformN *O2C=NULL, *C2O=NULL, *tmp=NULL;
  Transform3 C2W3, W2C3;
  HPointN *camvect=NULL, *translation=NULL;
  char *buf, buf1[256], buf2[256], ch, *inputstring, whatcamera[80];
  int objectflag = 0;
  int dims[4] = { -1, -1, -1, -1 };
#define xdim dims[0]
#define ydim dims[1]
#define zdim dims[2]
#define wdim dims[3]
  int j;
  Camera *focus = NULL;

  update_dimension();
  if (dimension <= 3)
    return;
  printf("(dimension %d)\n",dimension);

  /* Flush stdin */
  iobfrewind(infile);

  printf("(echo x\\n)\n");
  fflush(stdout);
  ch = '\0';
  /*
    fprintf(stderr,"buffer contents=");
  */
  while (ch != 'x')
    {
      ch = (char)iobfgetc(infile);
      /*
	fprintf(stderr,"%c", ch);
      */
    }
  iobfgetc(infile);
  /*
    fprintf(stderr,"\n");
  */

  /* Find out if the target is the world */
  printf("(echo (real-id targetgeom)\\n)\n");
  fflush(stdout);
  if( iobfgets(buf1, sizeof(buf1), infile) == NULL) {
    fprintf(stderr, "Please check source: %s: %d\n", __FILE__, __LINE__);
  }
  if( strcmp(buf1, "\"World\" \n") != 0) {
    /* the world is NOT selected */
    fprintf(stderr, "The world is NOT selected: %s\n",buf1);
    objectflag = 1;
    printf("(echo (ND-xform-get targetgeom))\n");
    fflush(stdout);
    O2W = TmNRead(infile, 0);
  }

  printf("(echo (ND-xform-get world))\n");
  fflush(stdout);
  W2U = TmNRead(infile, 0);
  printf("(echo (if (real-id focus) "
	 "(ND-xform-get focus) (ND-xform-get g0)))\n");
  fflush(stdout);
  C2U = TmNRead(infile, 0);

  /* Then multiply the right ones together */
  U2C = TmNInvert(C2U, U2C);
  if ( objectflag ) {
    tmp = TmNConcat(O2W, W2U, tmp);
    O2C = TmNConcat(tmp, U2C, O2C);
  } else {
    O2C = TmNConcat(W2U, U2C, O2C);
  }

  /* next find out what camera this is */
  printf("(echo (real-id focus)\\n)\n"); fflush(stdout);
  ch = iobfnextc(infile, 0);
#if 0
  if ( scanf("\"%[^\"]\"",buf1) == 0) {
    /* Ok, we don't have a focus.  We were probably started
       without a camera window.  Just return now and cut our
       losses. */
    goto error;
    /*	fprintf(stderr, "Please check source: #2388433\n"); */
  }
  sprintf(whatcamera,"%s",buf1);
#else
  /* no scanf with iobuffered files */
  if (ch != '"' || (buf = iobftoken(infile, 0)) == NULL || strlen(buf) == 0) {
    goto error;
  }
  sprintf(whatcamera,"%s",buf);
#endif

  /* next find out what the x, y, z, projections are */
  printf("(echo (ND-axes focus)\\n)\n");
  fflush(stdout);
  iobfnextc(infile, 0);
#if 0
  if ( scanf("(\"%[^\"]\"%d%d%d%[^)])",buf1,&xdim,&ydim,&zdim,buf2) == 0) {
    fprintf(stderr, "Please check source: %s: %d\n", __FILE__, __LINE__);
  }
#else
  /* no scanf with iobuffered files */
  if (iobfgetc(infile) != '(') {
    fprintf(stderr, "Please check source: %s: %d\n", __FILE__, __LINE__);
    goto error;
  }
  if ((buf = iobfdelimtok(")", infile, 0)) == NULL) {
    fprintf(stderr, "Please check source: %s: %d\n", __FILE__, __LINE__);
    goto error;
  }
  strncpy(buf1, buf, sizeof(buf1));
  buf1[sizeof(buf1)-1] = '\0';
  {
    if (iobfgetni(infile, 4, dims, 0) != 4) {
      fprintf(stderr,
	      "buf1=%s, xdim=%d ydim=%d zdim=%d wdim=%d dimension=%d\n", 
	      buf1, xdim, ydim, zdim, wdim, dimension);
      fprintf(stderr, "Please check source: %s: %d (%d %d %d %d)\n",
	      __FILE__, __LINE__, xdim, ydim, zdim, wdim);
      goto error;
    }
  }
  while ((buf = iobfdelimtok(")", infile, 0)) && *buf != ')');
#endif

  if ( (xdim <= 0) || (ydim <= 0) || (zdim <= 0) || (xdim > dimension) || 
       (ydim > dimension) || (zdim > dimension) ) {
    /* something is wrong */
    fprintf(stderr, "buf1=%s, xdim=%d ydim=%d zdim=%d dimension=%d\n", 
	    buf1, xdim, ydim, zdim, dimension);
    fprintf(stderr, "Please check source: %s: %d (%d %d %d %d)\n",
	    __FILE__, __LINE__,
	    xdim, ydim, zdim, wdim);
    goto error;
  }
  /*
    fprintf(stderr, "The axes for this camera are: %d %d %d\n", xdim, ydim, zdim);
  */

  /* The camera has now its own 3D transform, the focallen stuff was
   * a kludge anyway. The kludge below is even more kludgy.
   */ 
  printf("(write camera - focus)\n"); 
  fflush(stdout);
  focus = CamFLoad(NULL, infile, NULL);
  CamGet(focus, CAM_C2W, C2W3);
  CamGet(focus, CAM_W2C, W2C3);

  /* Give the camera position as it is seen in the projected space? */
  C2O = TmNInvert(O2C, NULL);
  translation = HPtNCreate(pdim, NULL);
  camvect = HPtNTransform3(C2W3, dims, translation, NULL);
  HPtNTransform(C2O, camvect, camvect);

  switch(i) {
  case 0:			/* just update */
    strcpy(buf1,"setentry .basis.l.right.s1 {(");
    for ( j = 1; j < dimension; j++)  {
      sprintf(buf2, "%.2f", O2C->a[xdim*pdim+j]);
      strcat(buf1, buf2);
      strcat(buf1, ",");
    }
    sprintf(buf2, "%.2f", O2C->a[xdim*pdim+dimension]);
    strcat(buf1, buf2);
    strcat(buf1, ")}");
	
    dotcl(buf1);

    strcpy(buf1,"setentry .basis.l.right.s2 {(");
    for ( j = 1; j < dimension; j++)  {
      sprintf(buf2, "%.2f", O2C->a[ydim*pdim+j]);
      strcat(buf1, buf2);
      strcat(buf1, ",");
    }
    sprintf(buf2, "%.2f", O2C->a[ydim*pdim+dimension]);
    strcat(buf1, buf2);
    strcat(buf1, ")}");
	
    dotcl(buf1);

    strcpy(buf1,"setentry .basis.l.right.s3 {(");
    for ( j = 1; j < dimension; j++)  {
      sprintf(buf2, "%.2f", O2C->a[zdim*pdim+j]);
      strcat(buf1, buf2);
      strcat(buf1, ",");
    }
    sprintf(buf2, "%.2f", O2C->a[zdim*pdim+dimension]);
    strcat(buf1, buf2);
    strcat(buf1, ")}");

    dotcl(buf1);

    strcpy(buf1,"setentry .basis.r.disp {(");
    for (j = 0; j < pdim; j++) {
      translation->v[j] = O2C->a[0*pdim+j];
    }
    /* add in the adjustment of the camera for printing purposes */
    HPtNTransform3(W2C3, dims, translation, translation);

    for ( j = 1; j< dimension; j++)  {
      sprintf(buf2, "%.1f", -translation->v[j]);
      strcat(buf1, buf2);
      strcat(buf1, ",");
    }
    sprintf(buf2, "%.1f", -translation->v[j]);
    strcat(buf1, buf2);
    strcat(buf1, ")}");

    dotcl(buf1);

    strcpy(buf1, "setentry .basis.r.view {(");
    sprintf(buf2, "%.2f", camvect->v[xdim]);
    strcat(buf1, buf2); strcat(buf1, ",");
    sprintf(buf2, "%.2f", camvect->v[ydim]);
    strcat(buf1, buf2); strcat(buf1, ",");
    sprintf(buf2, "%.2f", camvect->v[zdim]);
    strcat(buf1, buf2); strcat(buf1, ")}");

    dotcl(buf1);

    sprintf(buf1,"set basis_target {%s}",whatcamera);
    dotcl(buf1);

    break;
  case 1:			/* x direction */
    buf = strdup(get_entry(".basis.l.right.s1"));
    if ( buf[0] != '(' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    if ( buf[strlen(buf)-1] != ')' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    buf[strlen(buf)-1] = '\0'; /* chop off last char */
    if ( (inputstring = strtok(&(buf[1]),",")  ) == NULL ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    O2C->a[xdim*pdim+1] = atof(inputstring);
    for( j = 2; j < pdim; j++ ) {
      if ( (inputstring = strtok(NULL,",")  ) == NULL ) {
	fprintf(stderr,
		"Format of input string must be (a, b, . . . )\n");
	goto error;
      }
      O2C->a[xdim*pdim+j] = atof(inputstring);
    }
    if ( return_new_matrix(O2C, O2W, C2U, W2U, xdim, ydim, zdim, objectflag) == 0 ) {
      /* reinstate old values */
      ;
    }
    spanproc(0);
    break;
  case 2:			/* y direction */
    buf = strdup(get_entry(".basis.l.right.s2"));
    if ( buf[0] != '(' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    if ( buf[strlen(buf)-1] != ')' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    buf[strlen(buf)-1] = '\0';; /* chop off last char */
    if ( (inputstring = strtok(&(buf[1]),",")  ) == NULL ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    O2C->a[ydim*pdim+1] = atof(inputstring);
    for( j = 2; j < pdim; j++ ) {
      if ( (inputstring = strtok(NULL,",")  ) == NULL ) {
	fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
	goto error;
      }
      O2C->a[ydim*pdim+j] = atof(inputstring);
    }
    if ( return_new_matrix(O2C, O2W, C2U, W2U, ydim, xdim, zdim, objectflag) == 0 ) {
      /* reinstate old values */
      ;
    }
    spanproc(0);
    break;
  case 3:			/* z direction */
    buf = strdup(get_entry(".basis.l.right.s3"));
    if ( buf[0] != '(' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    if ( buf[strlen(buf)-1] != ')' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    buf[strlen(buf)-1] = '\0'; /* chop off last char */
    if ( (inputstring = strtok(&(buf[1]),",")  ) == NULL ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    O2C->a[zdim*pdim+1] = atof(inputstring);
    for( j = 2; j < pdim; j++ ) {
      if ( (inputstring = strtok(NULL,",")  ) == NULL ) {
	fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
	goto error;
      }
      O2C->a[zdim*pdim+j] = atof(inputstring);
    }
    if ( return_new_matrix(O2C, O2W, C2U, W2U, zdim, xdim, ydim, objectflag) == 0 ) {
      /* reinstate old values */
      ;
    }
    spanproc(0);
    break;
  case 4:			/* displacement */
    buf = strdup(get_entry(".basis.r.disp"));
    if ( buf[0] != '(' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    if ( buf[strlen(buf)-1] != ')' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    buf[strlen(buf)-1] = '\0'; /* chop off last char */
    if ( (inputstring = strtok(&(buf[1]),",")  ) == NULL ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    translation->v[0] = 1.0;
    translation->v[1] = -(atof(inputstring));
    for( j = 2; j < pdim; j++ ) {
      if ( (inputstring = strtok(NULL,",")  ) == NULL ) {
	fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
	goto error;
      }
      translation->v[j] = -(atof(inputstring));
    }
    HPtNTransform3(C2W3, dims, translation, translation);
    for( j = 0; j < pdim; j++ ) {
      O2C->a[pdim*0+j] = translation->v[j];
    }

    if ( return_new_matrix(O2C, O2W, C2U, W2U, xdim, ydim, zdim, objectflag) == 0) {
      /* reinstate old values */
      ;
    }
    spanproc(0);
    break;
  case 5:			/* camera viewpoint */
    buf = strdup(get_entry(".basis.r.view"));
    if ( buf[0] != '(' ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    if ( buf[strlen(buf)-1] != ')' ) {
      fprintf(stderr, "Format of input string must be (num1, num2, num3)\n");
      goto error;
    }
    buf[strlen(buf)-1] = '\0'; /* chop off last char */
    if ( (inputstring = strtok(&(buf[1]),",")  ) == NULL ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    camvect->v[xdim] = atof(inputstring);
    if ( (inputstring = strtok(NULL,",")  ) == NULL ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    camvect->v[ydim] = atof(inputstring);
    if ( (inputstring = strtok(NULL,",")  ) == NULL ) {
      fprintf(stderr, "Format of input string must be (a, b, . . . )\n");
      goto error;
    }
    camvect->v[zdim] = atof(inputstring);

    /* ok, we have the new elements of camvect. We want to install
     * a translation in O2C such that 0 C2W3 C2O = camvect. Do that.
     */
    camvect->v[0] = 0.0;
    HPtNTransform(O2C, camvect, translation);
    for ( j = 1; j < pdim; j++) {
      O2C->a[0*pdim + j] = -translation->v[j];
    }
    for ( j = 0; j < 3; j++) {
      O2C->a[0*pdim + dims[j]] += C2W3[3][j];
    }
    if ( return_new_matrix(O2C, O2W, C2U, W2U, xdim, ydim, zdim, objectflag) == 0) {
      /* reinstate old values */
      ;
    }
    spanproc(0);
    break;
  default:
    fprintf(stderr, "Please check source: %s: %d\n", __FILE__, __LINE__);
  }
  TmNDelete(O2C); TmNDelete(O2W); TmNDelete(W2U);
  TmNDelete(C2U); TmNDelete(U2C); TmNDelete(tmp);
  HPtNDelete(translation);
  HPtNDelete(camvect);
  return;
 error:
  TmNDelete(O2C); TmNDelete(O2W); TmNDelete(W2U);
  TmNDelete(C2U); TmNDelete(U2C); TmNDelete(tmp);
  HPtNDelete(translation);
  HPtNDelete(camvect);
  spanproc(0);
  return;
}

#undef xdim
#undef ydim
#undef zdim
#undef wdim


