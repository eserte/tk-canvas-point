/*
 * tkCanvLine.c --
 *
 *	This file implements line items for canvas widgets.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: ptkCanvPoint.c,v 1.1 2002/07/24 14:16:04 eserte Exp $
 */

#include "tkPort.h"
#include "tkInt.h"
#include "tkCanvases.h"

/*
 * The structure below defines the record for each line item.
 */

typedef struct PointItem  {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types.  MUST BE FIRST IN STRUCTURE. */
    Tk_Outline outline;		/* Outline structure */
    int capStyle;		/* Cap style for line. */
    double x, y;		/* X- and y-coord of point */
} PointItem;

/*
 * Prototypes for procedures defined in this file:
 */

static void		ComputePointBbox _ANSI_ARGS_((Tk_Canvas canvas,
			    PointItem *pointPtr));
static int		ConfigurePoint _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int argc,
			    Tcl_Obj **objv, int flags));
static int		CreatePoint _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int argc, Tcl_Obj **objv));
static void		DeletePoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display));
static void		DisplayPoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height));
static int		GetPointIndex _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr,
			    Tcl_Obj *obj, int *indexPtr));
static int		PointCoords _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr,
			    int argc, Tcl_Obj **objv));
static int		PointToArea _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *rectPtr));
static double		PointToPoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *coordPtr));
static int		PointToPostscript _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass));
static void		ScalePoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY));
static void		TranslatePoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double deltaX, double deltaY));

/*
 * Information used for parsing configuration specs.  If you change any
 * of the default strings, be sure to change the corresponding default
 * values in CreatePoint.
 */

static Tk_CustomOption stateOption = {
    Tk_StateParseProc,
    Tk_StatePrintProc, (ClientData) 2
};
static Tk_CustomOption tagsOption = {
    Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
};
static Tk_CustomOption tileOption = {
    Tk_TileParseProc,
    Tk_TilePrintProc, (ClientData) NULL
};
static Tk_CustomOption offsetOption = {
    Tk_OffsetParseProc,
    Tk_OffsetPrintProc,
    (ClientData) (TK_OFFSET_RELATIVE|TK_OFFSET_INDEX)
};
static Tk_CustomOption pixelOption = {
    Tk_PixelParseProc,
    Tk_PixelPrintProc, (ClientData) NULL
};

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-activefill",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.activeColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-activestipple",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.activeStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-activetile",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.activeTile),
	TK_CONFIG_NULL_OK, &tileOption},
    {TK_CONFIG_CUSTOM, "-activewidth",          NULL,          NULL,
	"0.0", Tk_Offset(PointItem, outline.activeWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_CAP_STYLE, "-capstyle",          NULL,          NULL,
	"round", Tk_Offset(PointItem, capStyle), TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_COLOR, "-fill",          NULL,          NULL,
	"black", Tk_Offset(PointItem, outline.color), TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-disabledfill",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.disabledColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-disabledstipple",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.disabledStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-disabledtile",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.disabledTile),
	TK_CONFIG_NULL_OK, &tileOption},
    {TK_CONFIG_CUSTOM, "-disabledwidth",          NULL,          NULL,
	"0.0", Tk_Offset(PointItem, outline.disabledWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_CUSTOM, "-offset",          NULL,          NULL,
	"0 0", Tk_Offset(PointItem, outline.tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_CUSTOM, "-state",          NULL,          NULL,
	         NULL, Tk_Offset(Tk_Item, state), TK_CONFIG_NULL_OK,
	&stateOption},
    {TK_CONFIG_BITMAP, "-stipple",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.stipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-tags",          NULL,          NULL,
	         NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_CUSTOM, "-tile",          NULL,          NULL,
	         NULL, Tk_Offset(PointItem, outline.tile),
	TK_CONFIG_NULL_OK, &tileOption},
    {TK_CONFIG_CUSTOM, "-width",          NULL,          NULL,
	"1.0", Tk_Offset(PointItem, outline.width),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_CALLBACK, "-updatecommand",          NULL,          NULL,
	         NULL, Tk_Offset(Tk_Item, updateCmd), TK_CONFIG_NULL_OK},
    {TK_CONFIG_END,          NULL,          NULL,          NULL,
	         NULL, 0, 0}
};

/*
 * The structures below defines the line item type by means
 * of procedures that can be invoked by generic item code.
 */

Tk_ItemType tkPointType = {
    "point",				/* name */
    sizeof(PointItem),			/* itemSize */
    CreatePoint,			/* createProc */
    configSpecs,			/* configSpecs */
    ConfigurePoint,			/* configureProc */
    PointCoords,			/* coordProc */
    DeletePoint,			/* deleteProc */
    DisplayPoint,			/* displayProc */
    TK_CONFIG_OBJS,			/* flags, no TK_ITEM_VISITOR_SUPPORT */
NULL,//    PointToPoint,			/* pointProc */
NULL,//    PointToArea,			/* areaProc */
NULL,//    PointToPostscript,			/* postscriptProc */
NULL,//    ScalePoint,				/* scaleProc */
NULL,//    TranslatePoint,			/* translateProc */
NULL,//    GetPointIndex,			/* indexProc */
    (Tk_ItemCursorProc *) NULL,		/* icursorProc */
    (Tk_ItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_ItemInsertProc *) NULL,		/* insertProc */
    (Tk_ItemDCharsProc *) NULL,		/* dTextProc */
    (Tk_ItemType *) NULL,		/* nextPtr */
    (Tk_ItemBboxProc *) ComputePointBbox,/* bboxProc */
    (Tk_VisitorItemProc *) NULL,	/* acceptProc */
    (Tk_ItemGetCoordProc *) NULL,	/* getCoordProc */
    (Tk_ItemSetCoordProc *) NULL	/* setCoordProc */
};

/*
 *--------------------------------------------------------------
 *
 * CreatePoint --
 *
 *	This procedure is invoked to create a new point item in
 *	a canvas.
 *
 * Results:
 *	A standard Tcl return value.  If an error occurred in
 *	creating the item, then an error message is left in
 *	Tcl_GetResult(interp);  in this case itemPtr is left uninitialized,
 *	so it can be safely freed by the caller.
 *
 * Side effects:
 *	A new point item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreatePoint(interp, canvas, itemPtr, argc, objv)
    Tcl_Interp *interp;			/* Interpreter for error reporting. */
    Tk_Canvas canvas;			/* Canvas to hold new item. */
    Tk_Item *itemPtr;			/* Record to hold new item;  header
					 * has been initialized by caller. */
    int argc;				/* Number of arguments in objv. */
    Tcl_Obj **objv;			/* Arguments describing line. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    int i;

    /*
     * Carry out initialization that is needed to set defaults and to
     * allow proper cleanup after errors during the the remainder of
     * this procedure.
     */

    Tk_CreateOutline(&(pointPtr->outline));
    pointPtr->capStyle = CapRound;

    /*
     * Count the number of points and then parse them into a point
     * array.  Leading arguments are assumed to be points if they
     * start with a digit or a minus sign followed by a digit.
     */

    for (i = 0; i < argc; i++) {
	char *arg = Tcl_GetStringFromObj(objv[i], NULL);
	if ((arg[0] == '-') && (arg[1] >= 'a')
		&& (arg[1] <= 'z')) {
	    break;
	}
    }
    if (i && (PointCoords(interp, canvas, itemPtr, i, objv) != TCL_OK)) {
	goto error;
    }
    if (ConfigurePoint(interp, canvas, itemPtr, argc-i, objv+i, 0) == TCL_OK) {
	return TCL_OK;
    }

    error:
    DeletePoint(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * PointCoords --
 *
 *	This procedure is invoked to process the "coords" widget
 *	command on points.  See the user documentation for details
 *	on what it does.
 *
 * Results:
 *	Returns TCL_OK or TCL_ERROR, and sets Tcl_GetResult(interp).
 *
 * Side effects:
 *	The coordinates for the given item may be changed.
 *
 *--------------------------------------------------------------
 */

static int
PointCoords(interp, canvas, itemPtr, argc, objv)
    Tcl_Interp *interp;			/* Used for error reporting. */
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item whose coordinates are to be
					 * read or modified. */
    int argc;				/* Number of coordinates supplied in
					 * objv. Should be 2. */
    Tcl_Obj **objv;			/* Array of coordinates: x1, y1,
					 * x2, y2, ... */
{
    PointItem *pointPtr = (PointItem *) itemPtr;

    if (argc == 0) {
	Tcl_Obj *subobj, *obj = Tcl_NewObj();
	subobj = Tcl_NewDoubleObj(pointPtr->x);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(pointPtr->y);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	Tcl_SetObjResult(interp, obj);
	return TCL_OK;
    }
    //XXX kann weg?
/*      if (argc == 1) { */
/*  	if (Tcl_ListObjGetElements(interp, objv[0], &argc, &objv) != TCL_OK) { */
/*  	    return TCL_ERROR; */
/*  	} */
/*      } */
    if (argc != 2) {
	Tcl_AppendResult(interp,
		"not two coordinates specified for point",
		         NULL);
	return TCL_ERROR;
    } else {
	if (Tk_CanvasGetCoordFromObj(interp, canvas, objv[0],
				     &(pointPtr->x)) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
				     &(pointPtr->y)) != TCL_OK) {
	    return TCL_ERROR;
	}

	ComputePointBbox(canvas, pointPtr);
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigurePoint --
 *
 *	This procedure is invoked to configure various aspects
 *	of a point item such as its background color.
 *
 * Results:
 *	A standard Tcl result code.  If an error occurs, then
 *	an error message is left in Tcl_GetResult(interp).
 *
 * Side effects:
 *	Configuration information, such as colors and stipple
 *	patterns, may be set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigurePoint(interp, canvas, itemPtr, argc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Canvas canvas;		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Point item to reconfigure. */
    int argc;			/* Number of elements in objv.  */
    Tcl_Obj **objv;		/* Arguments describing things to configure. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Window tkwin;
    Tk_State state;

    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs, argc, objv,
	    (char *) pointPtr, flags|TK_CONFIG_OBJS) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing, such as
     * graphics contexts.
     */

    state = Tk_GetItemState(canvas, itemPtr);

    if (pointPtr->outline.activeWidth > pointPtr->outline.width ||
	    pointPtr->outline.activeTile != None ||
	    pointPtr->outline.activeColor != NULL ||
	    pointPtr->outline.activeStipple != None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }
    mask = Tk_ConfigOutlineGC(&gcValues, canvas, itemPtr,
	    &(pointPtr->outline));
    if (mask) {
	gcValues.cap_style = pointPtr->capStyle;
	mask |= GCCapStyle;
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
	gcValues.line_width = 0;
    } else {
	newGC = None;
    }
    if (pointPtr->outline.gc != None) {
	Tk_FreeGC(Tk_Display(tkwin), pointPtr->outline.gc);
    }
    pointPtr->outline.gc = newGC;

    /*
     * Recompute bounding box for point.
     */

    ComputePointBbox(canvas, pointPtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeletePoint --
 *
 *	This procedure is called to clean up the data structure
 *	associated with a point item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with itemPtr are released.
 *
 *--------------------------------------------------------------
 */

static void
DeletePoint(canvas, itemPtr, display)
    Tk_Canvas canvas;			/* Info about overall canvas widget. */
    Tk_Item *itemPtr;			/* Item that is being deleted. */
    Display *display;			/* Display containing window for
					 * canvas. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;

    Tk_DeleteOutline(display, &(pointPtr->outline));
}

/*
 *--------------------------------------------------------------
 *
 * ComputePointBbox --
 *
 *	This procedure is invoked to compute the bounding box of
 *	all the pixels that may be drawn as part of a point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header
 *	for itemPtr.
 *
 *--------------------------------------------------------------
 */

static void
ComputePointBbox(canvas, pointPtr)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    PointItem *pointPtr;		/* Item whose bbox is to be
					 * recomputed. */
{
    int intWidth;
    double width;
    Tk_State state = Tk_GetItemState(canvas, &pointPtr->header);
    Tk_TSOffset *tsoffset;

    if (state==TK_STATE_HIDDEN) {
	pointPtr->header.x1 = -1;
	pointPtr->header.x2 = -1;
	pointPtr->header.y1 = -1;
	pointPtr->header.y2 = -1;
	return;
    }

    width = pointPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)pointPtr) {
	if (pointPtr->outline.activeWidth>width) {
	    width = pointPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (pointPtr->outline.disabledWidth>0) {
	    width = pointPtr->outline.disabledWidth;
	}
    }

    pointPtr->header.x1 = pointPtr->header.x2 = (int) pointPtr->x;
    pointPtr->header.y1 = pointPtr->header.y2 = (int) pointPtr->y;

    /*XXX change comment
     * Compute the bounding box of all the points in the line,
     * then expand in all directions by the line's width to take
     * care of butting or rounded corners and projecting or
     * rounded caps.  This expansion is an overestimate (worst-case
     * is square root of two over two) but it's simple.  Don't do
     * anything special for curves.  This causes an additional
     * overestimate in the bounding box, but is faster.
     */

    if (width < 1.0) {
	width = 1.0;
    }

    intWidth = (int) (width + 0.5);
    pointPtr->header.x1 -= intWidth - 1;
    pointPtr->header.x2 += intWidth + 1;
    pointPtr->header.y1 -= intWidth - 1;
    pointPtr->header.y2 += intWidth + 1;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayPoint --
 *
 *	This procedure is invoked to draw a point item in a given
 *	drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation
 *	information in canvas.
 *
 *--------------------------------------------------------------
 */

static void
DisplayPoint(canvas, itemPtr, display, drawable, x, y, width, height)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    Tk_Item *itemPtr;			/* Item to be displayed. */
    Display *display;			/* Display on which to draw item. */
    Drawable drawable;			/* Pixmap or window in which to draw
					 * item. */
    int x, y, width, height;		/* Describes region of canvas that
					 * must be redisplayed (not used). */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    XPoint staticPoint;
    double pointwidth;
    int intwidth;
    Tk_State state = Tk_GetItemState(canvas, itemPtr);
    Tk_Tile tile = pointPtr->outline.tile;
    Pixmap stipple = pointPtr->outline.stipple;

    if (pointPtr->outline.gc==None) {
	return;
    }

    pointwidth = pointPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (pointPtr->outline.activeWidth>pointwidth) {
	    pointwidth = pointPtr->outline.activeWidth;
	}
	if (pointPtr->outline.activeTile!=NULL) {
	    tile = pointPtr->outline.activeTile;
	}
	if (pointPtr->outline.activeStipple!=None) {
	    stipple = pointPtr->outline.activeStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (pointPtr->outline.disabledWidth>pointwidth) {
	    pointwidth = pointPtr->outline.disabledWidth;
	}
	if (pointPtr->outline.disabledTile!=NULL) {
	    tile = pointPtr->outline.disabledTile;
	}
	if (pointPtr->outline.disabledStipple!=None) {
	    stipple = pointPtr->outline.disabledStipple;
	}
    }
    /*
     * Build up an array of points in screen coordinates.  Use a
     * static array unless the line has an enormous number of points;
     * in this case, dynamically allocate an array.  For smoothed lines,
     * generate the curve points on each redisplay.
     */

    Tk_CanvasDrawableCoords(canvas, pointPtr->x, pointPtr->y,
			    &(staticPoint.x), &(staticPoint.y));

    /*
     * Display point, the free up line storage if it was dynamically
     * allocated.  If we're stippling, then modify the stipple offset
     * in the GC.  Be sure to reset the offset when done, since the
     * GC is supposed to be read-only.
     */

    //XXX heißt das, stipple etc. wird bei lines eh nur im arrow gemalt?
/*      if (Tk_ChangeOutlineGC(canvas, itemPtr, &(pointPtr->outline))) { */
/*  	Tk_CanvasSetOffset(canvas, pointPtr->arrowGC, &pointPtr->outline.tsoffset); */
/*      } */
    intwidth = (int) (pointwidth + 0.5);
    XFillArc(display, drawable, pointPtr->outline.gc, staticPoint.x - intwidth/2,
	     staticPoint.y - intwidth/2, intwidth+1, intwidth+1, 0, 64*360);

}

#if 0
/*
 *--------------------------------------------------------------
 *
 * LineInsert --
 *
 *	Insert coords into a line item at a given index.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The coords in the given item is modified.
 *
 *--------------------------------------------------------------
 */

static void
LineInsert(canvas, itemPtr, beforeThis, obj)
    Tk_Canvas canvas;		/* Canvas containing text item. */
    Tk_Item *itemPtr;		/* Line item to be modified. */
    int beforeThis;		/* Index before which new coordinates
				 * are to be inserted. */
    Tcl_Obj *obj;		/* New coordinates to be inserted. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    int length, argc, i;
    double *new, *coordPtr;
    Tk_State state = Tk_GetItemState(canvas, itemPtr);

    Tcl_Obj **objv;

    if (!obj || (Tcl_ListObjGetElements((Tcl_Interp *) NULL, obj, &argc, &objv) != TCL_OK)
	    || !argc || argc&1) {
	return;
    }
    length = 2*pointPtr->numPoints;
    if (beforeThis < 0) {
	beforeThis = 0;
    }
    if (beforeThis > length) {
	beforeThis = length;
    }
    if (pointPtr->firstArrowPtr != NULL) {
	pointPtr->coordPtr[0] = pointPtr->firstArrowPtr[0];
	pointPtr->coordPtr[1] = pointPtr->firstArrowPtr[1];
    }
    if (pointPtr->lastArrowPtr != NULL) {
	pointPtr->coordPtr[length-2] = pointPtr->lastArrowPtr[0];
	pointPtr->coordPtr[length-1] = pointPtr->lastArrowPtr[1];
    }
    new = (double *) ckalloc((unsigned)(sizeof(double) * (length + argc)));
    for(i=0; i<beforeThis; i++) {
	new[i] = pointPtr->coordPtr[i];
    }
    for(i=0; i<argc; i++) {
	if (Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,objv[i],
		new+(i+beforeThis))!=TCL_OK) {
	    Tcl_ResetResult(((TkCanvas *)canvas)->interp);
	    ckfree((char *) new);
	    return;
	}
    }

    for(i=beforeThis; i<length; i++) {
	new[i+argc] = pointPtr->coordPtr[i];
    }
    if(pointPtr->coordPtr) ckfree((char *)pointPtr->coordPtr);
    pointPtr->coordPtr = new;
    pointPtr->numPoints = (length + argc)/2;

    if ((length>3) && (state != TK_STATE_HIDDEN)) {
	/*
	 * This is some optimizing code that will result that only the part
	 * of the polygon that changed (and the objects that are overlapping
	 * with that part) need to be redrawn. A special flag is set that
	 * instructs the general canvas code not to redraw the whole
	 * object. If this flag is not set, the canvas will do the redrawing,
	 * otherwise I have to do it here.
	 */
	itemPtr->redraw_flags |= TK_ITEM_DONT_REDRAW;

	if (beforeThis>0) {beforeThis -= 2; argc+=2; }
	if ((beforeThis+argc)<length) argc+=2;
	if (pointPtr->smooth) {
	    if(beforeThis>0) {
		beforeThis-=2; argc+=2;
	    }
	    if((beforeThis+argc+2)<length) {
		argc+=2;
	    }
	}
	itemPtr->x1 = itemPtr->x2 = (int) pointPtr->coordPtr[beforeThis];
	itemPtr->y1 = itemPtr->y2 = (int) pointPtr->coordPtr[beforeThis+1];
	if ((pointPtr->firstArrowPtr != NULL) && (beforeThis<1)) {
	    /* include old first arrow */
	    for (i = 0, coordPtr = pointPtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((pointPtr->lastArrowPtr != NULL) && ((beforeThis+argc)>=length)) {
		/* include old last arrow */
	    for (i = 0, coordPtr = pointPtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	coordPtr = pointPtr->coordPtr+beforeThis+2;
	for(i=2; i<argc; i+=2) {
	    TkIncludePoint(itemPtr, coordPtr);
		coordPtr+=2;
	}
    }
    if (pointPtr->firstArrowPtr != NULL) {
	ckfree((char *) pointPtr->firstArrowPtr);
	pointPtr->firstArrowPtr = NULL;
    }
    if (pointPtr->lastArrowPtr != NULL) {
	ckfree((char *) pointPtr->lastArrowPtr);
	pointPtr->lastArrowPtr = NULL;
    }
    if (pointPtr->arrow != ARROWS_NONE) {
	    ConfigureArrows(canvas, pointPtr);
    }

    if(itemPtr->redraw_flags & TK_ITEM_DONT_REDRAW) {
	double width;
	int intWidth;
	if ((pointPtr->firstArrowPtr != NULL) && (beforeThis>2)) {
	    /* include new first arrow */
	    for (i = 0, coordPtr = pointPtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((pointPtr->lastArrowPtr != NULL) && ((beforeThis+argc)<(length-2))) {
	    /* include new right arrow */
	    for (i = 0, coordPtr = pointPtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	width = pointPtr->outline.width;
	if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
		if (pointPtr->outline.activeWidth>width) {
		    width = pointPtr->outline.activeWidth;
		}
	} else if (state==TK_STATE_DISABLED) {
		if (pointPtr->outline.disabledWidth>0) {
		    width = pointPtr->outline.disabledWidth;
		}
	}
	intWidth = (int) (width + 0.5);
	if (intWidth < 1) {
	    intWidth = 1;
	}
	itemPtr->x1 -= intWidth; itemPtr->y1 -= intWidth;
	itemPtr->x2 += intWidth; itemPtr->y2 += intWidth;
	Tk_CanvasEventuallyRedraw(canvas, itemPtr->x1, itemPtr->y1,
		itemPtr->x2, itemPtr->y2);
    }

    ComputeLineBbox(canvas, pointPtr);
}

/*
 *--------------------------------------------------------------
 *
 * LineDeleteCoords --
 *
 *	Delete one or more coordinates from a line item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters between "first" and "last", inclusive, get
 *	deleted from itemPtr.
 *
 *--------------------------------------------------------------
 */

static void
LineDeleteCoords(canvas, itemPtr, first, last)
    Tk_Canvas canvas;		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Item in which to delete characters. */
    int first;			/* Index of first character to delete. */
    int last;			/* Index of last character to delete. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    int count, i, first1, last1;
    int length = 2*pointPtr->numPoints;
    double *coordPtr;
    Tk_State state = Tk_GetItemState(canvas, itemPtr);

    first &= -2;
    last &= -2;

    if (first < 0) {
	first = 0;
    }
    if (last >= length) {
	last = length-2;
    }
    if (first > last) {
	return;
    }
    if (pointPtr->firstArrowPtr != NULL) {
	pointPtr->coordPtr[0] = pointPtr->firstArrowPtr[0];
	pointPtr->coordPtr[1] = pointPtr->firstArrowPtr[1];
    }
    if (pointPtr->lastArrowPtr != NULL) {
	pointPtr->coordPtr[length-2] = pointPtr->lastArrowPtr[0];
	pointPtr->coordPtr[length-1] = pointPtr->lastArrowPtr[1];
    }
    first1 = first; last1 = last;
    if(first1>0) first1 -= 2;
    if(last1<length-2) last1 += 2;
    if (pointPtr->smooth) {
	if(first1>0) first1 -= 2;
	if(last1<length-2) last1 += 2;
    }

    if((first1<2) && (last1 >= length-2)) {
	/*
	 * This is some optimizing code that will result that only the part
	 * of the line that changed (and the objects that are overlapping
	 * with that part) need to be redrawn. A special flag is set that
	 * instructs the general canvas code not to redraw the whole
	 * object. If this flag is set, the redrawing has to be done here,
	 * otherwise the general Canvas code will take care of it.
	 */

	itemPtr->redraw_flags |= TK_ITEM_DONT_REDRAW;
	itemPtr->x1 = itemPtr->x2 = (int) pointPtr->coordPtr[first1];
	itemPtr->y1 = itemPtr->y2 = (int) pointPtr->coordPtr[first1+1];
	if ((pointPtr->firstArrowPtr != NULL) && (first1<2)) {
	    /* include old first arrow */
	    for (i = 0, coordPtr = pointPtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((pointPtr->lastArrowPtr != NULL) && (last1>=length-2)) {
		/* include old last arrow */
	    for (i = 0, coordPtr = pointPtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	coordPtr = pointPtr->coordPtr+first1+2;
	for(i=first1+2; i<=last1; i+=2) {
	    TkIncludePoint(itemPtr, coordPtr);
		coordPtr+=2;
	}
    }

    count = last + 2 - first;
    for(i=last+2; i<length; i++) {
	pointPtr->coordPtr[i-count] = pointPtr->coordPtr[i];
    }
    pointPtr->numPoints -= count/2;
    if (pointPtr->firstArrowPtr != NULL) {
	ckfree((char *) pointPtr->firstArrowPtr);
	pointPtr->firstArrowPtr = NULL;
    }
    if (pointPtr->lastArrowPtr != NULL) {
	ckfree((char *) pointPtr->lastArrowPtr);
	pointPtr->lastArrowPtr = NULL;
    }
    if (pointPtr->arrow != ARROWS_NONE) {
	    ConfigureArrows(canvas, pointPtr);
    }
    if(itemPtr->redraw_flags & TK_ITEM_DONT_REDRAW) {
	double width;
	int intWidth;
	if ((pointPtr->firstArrowPtr != NULL) && (first1<4)) {
	    /* include new first arrow */
	    for (i = 0, coordPtr = pointPtr->firstArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	if ((pointPtr->lastArrowPtr != NULL) && (last1>(length-4))) {
	    /* include new right arrow */
	    for (i = 0, coordPtr = pointPtr->lastArrowPtr; i < PTS_IN_ARROW;
		    i++, coordPtr += 2) {
		TkIncludePoint(itemPtr, coordPtr);
	    }
	}
	width = pointPtr->outline.width;
	if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
		if (pointPtr->outline.activeWidth>width) {
		    width = pointPtr->outline.activeWidth;
		}
	} else if (state==TK_STATE_DISABLED) {
		if (pointPtr->outline.disabledWidth>0) {
		    width = pointPtr->outline.disabledWidth;
		}
	}
	intWidth = (int) (width + 0.5);
	if (intWidth < 1) {
	    intWidth = 1;
	}
	itemPtr->x1 -= intWidth; itemPtr->y1 -= intWidth;
	itemPtr->x2 += intWidth; itemPtr->y2 += intWidth;
	Tk_CanvasEventuallyRedraw(canvas, itemPtr->x1, itemPtr->y1,
		itemPtr->x2, itemPtr->y2);
    }
    ComputeLineBbox(canvas, pointPtr);
}

/*
 *--------------------------------------------------------------
 *
 * LineToPoint --
 *
 *	Computes the distance from a given point to a given
 *	line, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are pointPtr[0] and pointPtr[1] is inside the line.  If the
 *	point isn't inside the line then the return value is the
 *	distance from the point to the line.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
LineToPoint(canvas, itemPtr, pointPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against point. */
    double *pointPtr;		/* Pointer to x and y coordinates. */
{
    Tk_State state = Tk_GetItemState(canvas, itemPtr);
    PointItem *pointPtr = (PointItem *) itemPtr;
    double *coordPtr, *linePoints;
    double staticSpace[2*MAX_STATIC_POINTS];
    double poly[10];
    double bestDist, dist, width;
    int numPoints, count;
    int changedMiterToBevel;	/* Non-zero means that a mitered corner
				 * had to be treated as beveled after all
				 * because the angle was < 11 degrees. */

    bestDist = 1.0e36;

    /*
     * Handle smoothed lines by generating an expanded set of points
     * against which to do the check.
     */

    width = pointPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (pointPtr->outline.activeWidth>width) {
	    width = pointPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (pointPtr->outline.disabledWidth>0) {
	    width = pointPtr->outline.disabledWidth;
	}
    }

    if ((pointPtr->smooth) && (pointPtr->numPoints > 2)) {
	numPoints = pointPtr->smooth->coordProc(canvas, (double *) NULL,
		pointPtr->numPoints, pointPtr->splineSteps, (XPoint *) NULL,
		(double *) NULL);
	if (numPoints <= MAX_STATIC_POINTS) {
	    linePoints = staticSpace;
	} else {
	    linePoints = (double *) ckalloc((unsigned)
		    (2*numPoints*sizeof(double)));
	}
	numPoints = pointPtr->smooth->coordProc(canvas, pointPtr->coordPtr,
		pointPtr->numPoints, pointPtr->splineSteps, (XPoint *) NULL,
		linePoints);
    } else {
	numPoints = pointPtr->numPoints;
	linePoints = pointPtr->coordPtr;
    }

    if (width < 1.0) {
	width = 1.0;
    }

    if (!numPoints || itemPtr->state==TK_STATE_HIDDEN) {
	return bestDist;
    } else if (numPoints == 1) {
	bestDist = hypot(linePoints[0] - pointPtr[0], linePoints[1] - pointPtr[1])
		    - width/2.0;
	if (bestDist < 0) bestDist = 0;
	return bestDist;
    }

    /*
     * The overall idea is to iterate through all of the edges of
     * the line, computing a polygon for each edge and testing the
     * point against that polygon.  In addition, there are additional
     * tests to deal with rounded joints and caps.
     */

    changedMiterToBevel = 0;
    for (count = numPoints, coordPtr = linePoints; count >= 2;
	    count--, coordPtr += 2) {

	/*
	 * If rounding is done around the first point then compute
	 * the distance between the point and the point.
	 */

	if (((pointPtr->capStyle == CapRound) && (count == numPoints))
		|| ((pointPtr->joinStyle == JoinRound)
			&& (count != numPoints))) {
	    dist = hypot(coordPtr[0] - pointPtr[0], coordPtr[1] - pointPtr[1])
		    - width/2.0;
	    if (dist <= 0.0) {
		bestDist = 0.0;
		goto done;
	    } else if (dist < bestDist) {
		bestDist = dist;
	    }
	}

	/*
	 * Compute the polygonal shape corresponding to this edge,
	 * consisting of two points for the first point of the edge
	 * and two points for the last point of the edge.
	 */

	if (count == numPoints) {
	    TkGetButtPoints(coordPtr+2, coordPtr, width,
		    pointPtr->capStyle == CapProjecting, poly, poly+2);
	} else if ((pointPtr->joinStyle == JoinMiter) && !changedMiterToBevel) {
	    poly[0] = poly[6];
	    poly[1] = poly[7];
	    poly[2] = poly[4];
	    poly[3] = poly[5];
	} else {
	    TkGetButtPoints(coordPtr+2, coordPtr, width, 0,
		    poly, poly+2);

	    /*
	     * If this line uses beveled joints, then check the distance
	     * to a polygon comprising the last two points of the previous
	     * polygon and the first two from this polygon;  this checks
	     * the wedges that fill the mitered joint.
	     */

	    if ((pointPtr->joinStyle == JoinBevel) || changedMiterToBevel) {
		poly[8] = poly[0];
		poly[9] = poly[1];
		dist = TkPolygonToPoint(poly, 5, pointPtr);
		if (dist <= 0.0) {
		    bestDist = 0.0;
		    goto done;
		} else if (dist < bestDist) {
		    bestDist = dist;
		}
		changedMiterToBevel = 0;
	    }
	}
	if (count == 2) {
	    TkGetButtPoints(coordPtr, coordPtr+2, width,
		    pointPtr->capStyle == CapProjecting, poly+4, poly+6);
	} else if (pointPtr->joinStyle == JoinMiter) {
	    if (TkGetMiterPoints(coordPtr, coordPtr+2, coordPtr+4,
		    width, poly+4, poly+6) == 0) {
		changedMiterToBevel = 1;
		TkGetButtPoints(coordPtr, coordPtr+2, width,
			0, poly+4, poly+6);
	    }
	} else {
	    TkGetButtPoints(coordPtr, coordPtr+2, width, 0,
		    poly+4, poly+6);
	}
	poly[8] = poly[0];
	poly[9] = poly[1];
	dist = TkPolygonToPoint(poly, 5, pointPtr);
	if (dist <= 0.0) {
	    bestDist = 0.0;
	    goto done;
	} else if (dist < bestDist) {
	    bestDist = dist;
	}
    }

    /*
     * If caps are rounded, check the distance to the cap around the
     * final end point of the line.
     */

    if (pointPtr->capStyle == CapRound) {
	dist = hypot(coordPtr[0] - pointPtr[0], coordPtr[1] - pointPtr[1])
		- width/2.0;
	if (dist <= 0.0) {
	    bestDist = 0.0;
	    goto done;
	} else if (dist < bestDist) {
	    bestDist = dist;
	}
    }

    /*
     * If there are arrowheads, check the distance to the arrowheads.
     */

    if (pointPtr->arrow != ARROWS_NONE) {
	if (pointPtr->arrow != ARROWS_LAST) {
	    dist = TkPolygonToPoint(pointPtr->firstArrowPtr, PTS_IN_ARROW,
		    pointPtr);
	    if (dist <= 0.0) {
		bestDist = 0.0;
		goto done;
	    } else if (dist < bestDist) {
		bestDist = dist;
	    }
	}
	if (pointPtr->arrow != ARROWS_FIRST) {
	    dist = TkPolygonToPoint(pointPtr->lastArrowPtr, PTS_IN_ARROW,
		    pointPtr);
	    if (dist <= 0.0) {
		bestDist = 0.0;
		goto done;
	    } else if (dist < bestDist) {
		bestDist = dist;
	    }
	}
    }

    done:
    if ((linePoints != staticSpace) && (linePoints != pointPtr->coordPtr)) {
	ckfree((char *) linePoints);
    }
    return bestDist;
}

/*
 *--------------------------------------------------------------
 *
 * LineToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangular area.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the
 *	area, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
LineToArea(canvas, itemPtr, rectPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against line. */
    double *rectPtr;
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    double staticSpace[2*MAX_STATIC_POINTS];
    double *linePoints;
    int numPoints, result;
    double radius, width;
    Tk_State state = Tk_GetItemState(canvas, itemPtr);

    width = pointPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (pointPtr->outline.activeWidth>width) {
	    width = pointPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (pointPtr->outline.disabledWidth>0) {
	    width = pointPtr->outline.disabledWidth;
	}
    }

    radius = (width+1.0)/2.0;

    if ((state==TK_STATE_HIDDEN) || !pointPtr->numPoints) {
	return -1;
    } else if (pointPtr->numPoints == 1) {
	double oval[4];
	oval[0] = pointPtr->coordPtr[0]-radius;
	oval[1] = pointPtr->coordPtr[1]-radius;
	oval[2] = pointPtr->coordPtr[0]+radius;
	oval[3] = pointPtr->coordPtr[1]+radius;
	return TkOvalToArea(oval, rectPtr);
    }

    /*
     * Handle smoothed lines by generating an expanded set of points
     * against which to do the check.
     */

    if ((pointPtr->smooth) && (pointPtr->numPoints > 2)) {
	numPoints = pointPtr->smooth->coordProc(canvas, (double *) NULL,
		pointPtr->numPoints, pointPtr->splineSteps, (XPoint *) NULL,
		(double *) NULL);
	if (numPoints <= MAX_STATIC_POINTS) {
	    linePoints = staticSpace;
	} else {
	    linePoints = (double *) ckalloc((unsigned)
		    (2*numPoints*sizeof(double)));
	}
	numPoints = pointPtr->smooth->coordProc(canvas, pointPtr->coordPtr,
		pointPtr->numPoints, pointPtr->splineSteps, (XPoint *) NULL,
		linePoints);
    } else {
	numPoints = pointPtr->numPoints;
	linePoints = pointPtr->coordPtr;
    }

    /*
     * Check the segments of the line.
     */

    if (width < 1.0) {
	width = 1.0;
    }

    result = TkThickPolyLineToArea(linePoints, numPoints,
	    width, pointPtr->capStyle, pointPtr->joinStyle,
	    rectPtr);
    if (result == 0) {
	goto done;
    }

    /*
     * Check arrowheads, if any.
     */

    if (pointPtr->arrow != ARROWS_NONE) {
	if (pointPtr->arrow != ARROWS_LAST) {
	    if (TkPolygonToArea(pointPtr->firstArrowPtr, PTS_IN_ARROW,
		    rectPtr) != result) {
		result = 0;
		goto done;
	    }
	}
	if (pointPtr->arrow != ARROWS_FIRST) {
	    if (TkPolygonToArea(pointPtr->lastArrowPtr, PTS_IN_ARROW,
		    rectPtr) != result) {
		result = 0;
		goto done;
	    }
	}
    }

    done:
    if ((linePoints != staticSpace) && (linePoints != pointPtr->coordPtr)) {
	ckfree((char *) linePoints);
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleLine --
 *
 *	This procedure is invoked to rescale a line item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The line referred to by itemPtr is rescaled so that the
 *	following transformation is applied to all point
 *	coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleLine(canvas, itemPtr, originX, originY, scaleX, scaleY)
    Tk_Canvas canvas;			/* Canvas containing line. */
    Tk_Item *itemPtr;			/* Line to be scaled. */
    double originX, originY;		/* Origin about which to scale rect. */
    double scaleX;			/* Amount to scale in X direction. */
    double scaleY;			/* Amount to scale in Y direction. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    double *coordPtr;
    int i;

    /*
     * Delete any arrowheads before scaling all the points (so that
     * the end-points of the line get restored).
     */

    if (pointPtr->firstArrowPtr != NULL) {
	pointPtr->coordPtr[0] = pointPtr->firstArrowPtr[0];
	pointPtr->coordPtr[1] = pointPtr->firstArrowPtr[1];
	ckfree((char *) pointPtr->firstArrowPtr);
	pointPtr->firstArrowPtr = NULL;
    }
    if (pointPtr->lastArrowPtr != NULL) {
	int i;

	i = 2*(pointPtr->numPoints-1);
	pointPtr->coordPtr[i] = pointPtr->lastArrowPtr[0];
	pointPtr->coordPtr[i+1] = pointPtr->lastArrowPtr[1];
	ckfree((char *) pointPtr->lastArrowPtr);
	pointPtr->lastArrowPtr = NULL;
    }
    for (i = 0, coordPtr = pointPtr->coordPtr; i < pointPtr->numPoints;
	    i++, coordPtr += 2) {
	coordPtr[0] = originX + scaleX*(*coordPtr - originX);
	coordPtr[1] = originY + scaleY*(coordPtr[1] - originY);
    }
    if (pointPtr->arrow != ARROWS_NONE) {
	ConfigureArrows(canvas, pointPtr);
    }
    ComputeLineBbox(canvas, pointPtr);
}

/*
 *--------------------------------------------------------------
 *
 * GetLineIndex --
 *
 *	Parse an index into a line item and return either its value
 *	or an error.
 *
 * Results:
 *	A standard Tcl result.  If all went well, then *indexPtr is
 *	filled in with the index (into itemPtr) corresponding to
 *	string.  Otherwise an error message is left in
 *	Tcl_GetResult(interp).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetLineIndex(interp, canvas, itemPtr, obj, indexPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item for which the index is being
				 * specified. */
    Tcl_Obj *obj;		/* Specification of a particular coord
				 * in itemPtr's line. */
    int *indexPtr;		/* Where to store converted index. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    int length;
    char *string;
    int i;
    double x ,y, bestDist, dist, *coordPtr;
    char *end, *p;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, obj, &i, &objv) == TCL_OK && i == 2
	&& Tcl_GetDoubleFromObj(interp, objv[0], &x) == TCL_OK
	&& Tcl_GetDoubleFromObj(interp, objv[1], &y) == TCL_OK) {
	goto doxy;
    }

    string = Tcl_GetStringFromObj(obj, &length);
    if (string[0] == 'e') {
	if (strncmp(string, "end", length) == 0) {
	    *indexPtr = 2*pointPtr->numPoints;
	} else {
	    badIndex:

	    /*
	     * Some of the paths here leave messages in Tcl_GetResult(interp),
	     * so we have to clear it out before storing our own message.
	     */

	    Tcl_SetResult(interp,          NULL, TCL_STATIC);
	    Tcl_AppendResult(interp, "bad index \"", string, "\"",
		             NULL);
	    return TCL_ERROR;
	}
    } else if (string[0] == '@') {
	p = string+1;
	x = strtod(p, &end);
	if ((end == p) || (*end != ',')) {
	    goto badIndex;
	}
	p = end+1;
	y = strtod(p, &end);
	if ((end == p) || (*end != 0)) {
	    goto badIndex;
	}
     doxy:
	bestDist = 1.0e36;
	coordPtr = pointPtr->coordPtr;
	*indexPtr = 0;
	for(i=0; i<pointPtr->numPoints; i++) {
	    dist = hypot(coordPtr[0] - x, coordPtr[1] - y);
	    if (dist<bestDist) {
		bestDist = dist;
		*indexPtr = 2*i;
	    }
	    coordPtr += 2;
	}
    } else {
	if (Tcl_GetIntFromObj(interp, obj, indexPtr) != TCL_OK) {
	    goto badIndex;
	}
	*indexPtr &= -2; /* if index is odd, make it even */
	if (*indexPtr < 0){
	    *indexPtr = 0;
	} else if (*indexPtr > (2*pointPtr->numPoints)) {
	    *indexPtr = (2*pointPtr->numPoints);
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TranslateLine --
 *
 *	This procedure is called to move a line by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the line is offset by (xDelta, yDelta), and
 *	the bounding box is updated in the generic part of the item
 *	structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateLine(canvas, itemPtr, deltaX, deltaY)
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item that is being moved. */
    double deltaX, deltaY;		/* Amount by which item is to be
					 * moved. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    double *coordPtr;
    int i;

    for (i = 0, coordPtr = pointPtr->coordPtr; i < pointPtr->numPoints;
	    i++, coordPtr += 2) {
	coordPtr[0] += deltaX;
	coordPtr[1] += deltaY;
    }
    if (pointPtr->firstArrowPtr != NULL) {
	for (i = 0, coordPtr = pointPtr->firstArrowPtr; i < PTS_IN_ARROW;
		i++, coordPtr += 2) {
	    coordPtr[0] += deltaX;
	    coordPtr[1] += deltaY;
	}
    }
    if (pointPtr->lastArrowPtr != NULL) {
	for (i = 0, coordPtr = pointPtr->lastArrowPtr; i < PTS_IN_ARROW;
		i++, coordPtr += 2) {
	    coordPtr[0] += deltaX;
	    coordPtr[1] += deltaY;
	}
    }
    ComputeLineBbox(canvas, pointPtr);
}

/*
 *--------------------------------------------------------------
 *
 * ParseArrowShape --
 *
 *	This procedure is called back during option parsing to
 *	parse arrow shape information.
 *
 * Results:
 *	The return value is a standard Tcl result:  TCL_OK means
 *	that the arrow shape information was parsed ok, and
 *	TCL_ERROR means it couldn't be parsed.
 *
 * Side effects:
 *	Arrow information in recordPtr is updated.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ParseArrowShape(clientData, interp, tkwin, value, recordPtr, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Window tkwin;		/* Not used. */
    Arg value;			/* Textual specification of arrow shape. */
    char *recordPtr;		/* Pointer to item record in which to
				 * store arrow information. */
    int offset;			/* Offset of shape information in widget
				 * record. */
{
    PointItem *pointPtr = (PointItem *) recordPtr;
    double a, b, c;
    int argc;
    Tcl_Obj **objv = NULL;

    if (offset != Tk_Offset(PointItem, arrowShapeA)) {
	panic("ParseArrowShape received bogus offset");
    }

    if (Tcl_ListObjGetElements(interp, value, &argc, &objv) != TCL_OK) {
	syntaxError:
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "bad arrow shape \"", LangString(value),
		"\": must be list with three numbers",          NULL);
	return TCL_ERROR;
    }
    if (argc != 3) {
	goto syntaxError;
    }
    if ((Tk_CanvasGetCoordFromObj(interp, pointPtr->canvas, objv[0], &a) != TCL_OK)
	    || (Tk_CanvasGetCoordFromObj(interp, pointPtr->canvas, objv[1], &b)
		!= TCL_OK)
	    || (Tk_CanvasGetCoordFromObj(interp, pointPtr->canvas, objv[2], &c)
		!= TCL_OK)) {
	goto syntaxError;
    }
    pointPtr->arrowShapeA = (float) a;
    pointPtr->arrowShapeB = (float) b;
    pointPtr->arrowShapeC = (float) c;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * PrintArrowShape --
 *
 *	This procedure is a callback invoked by the configuration
 *	code to return a printable value describing an arrow shape.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

    /* ARGSUSED */
static Arg
PrintArrowShape(clientData, tkwin, recordPtr, offset, freeProcPtr)
    ClientData clientData;	/* Not used. */
    Tk_Window tkwin;		/* Window associated with pointPtr's widget. */
    char *recordPtr;		/* Pointer to item record containing current
				 * shape information. */
    int offset;			/* Offset of arrow information in record. */
    Tcl_FreeProc **freeProcPtr;	/* Store address of procedure to call to
				 * free string here. */
{
    PointItem *pointPtr = (PointItem *) recordPtr;
    Arg result = Tcl_NewListObj(0,NULL);
    Tcl_ListObjAppendElement(NULL,result,Tcl_NewDoubleObj(pointPtr->arrowShapeA));
    Tcl_ListObjAppendElement(NULL,result,Tcl_NewDoubleObj(pointPtr->arrowShapeB));
    Tcl_ListObjAppendElement(NULL,result,Tcl_NewDoubleObj(pointPtr->arrowShapeC));
    return result;
}


/*
 *--------------------------------------------------------------
 *
 * ArrowParseProc --
 *
 *	This procedure is invoked during option processing to handle
 *	the "-arrow" option.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The arrow for a given item gets replaced by the arrow
 *	indicated in the value argument.
 *
 *--------------------------------------------------------------
 */

static int
ArrowParseProc(clientData, interp, tkwin, ovalue, widgRec, offset)
    ClientData clientData;		/* some flags.*/
    Tcl_Interp *interp;			/* Used for reporting errors. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    Arg ovalue;				/* Value of option. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
{
    int c;
    size_t length;
    char *value = LangString(ovalue);

    register Arrows *arrowPtr = (Arrows *) (widgRec + offset);

    if(value == NULL || *value == 0) {
	*arrowPtr = ARROWS_NONE;
	return TCL_OK;
    }

    c = value[0];
    length = strlen(value);

    if ((c == 'n') && (strncmp(value, "none", length) == 0)) {
	*arrowPtr = ARROWS_NONE;
	return TCL_OK;
    }
    if ((c == 'f') && (strncmp(value, "first", length) == 0)) {
	*arrowPtr = ARROWS_FIRST;
	return TCL_OK;
    }
    if ((c == 'l') && (strncmp(value, "last", length) == 0)) {
	*arrowPtr = ARROWS_LAST;
	return TCL_OK;
    }
    if ((c == 'b') && (strncmp(value, "both", length) == 0)) {
	*arrowPtr = ARROWS_BOTH;
	return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad arrow spec \"", value,
	    "\": must be none, first, last, or both",
	             NULL);
    *arrowPtr = ARROWS_NONE;
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * ArrowPrintProc --
 *
 *	This procedure is invoked by the Tk configuration code
 *	to produce a printable string for the "-arrow"
 *	configuration option.
 *
 * Results:
 *	The return value is a string describing the arrows for
 *	the item referred to by "widgRec".  In addition, *freeProcPtr
 *	is filled in with the address of a procedure to call to free
 *	the result string when it's no longer needed (or NULL to
 *	indicate that the string doesn't need to be freed).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static Arg
ArrowPrintProc(clientData, tkwin, widgRec, offset, freeProcPtr)
    ClientData clientData;		/* Ignored. */
    Tk_Window tkwin;			/* Window containing canvas widget. */
    char *widgRec;			/* Pointer to record for item. */
    int offset;				/* Offset into item. */
    Tcl_FreeProc **freeProcPtr;		/* Pointer to variable to fill in with
					 * information about how to reclaim
					 * storage for return string. */
{
    register Arrows *arrowPtr = (Arrows *) (widgRec + offset);
    Arg result = NULL;

    switch (*arrowPtr) {
      case ARROWS_FIRST:
	return LangStringArg("first");
      case ARROWS_LAST:
	return LangStringArg("last");
      case ARROWS_BOTH:
	return LangStringArg("both");
      default:
	return LangStringArg("none");
  }
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureArrows --
 *
 *	If arrowheads have been requested for a line, this
 *	procedure makes arrangements for the arrowheads.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	Information in pointPtr is set up for one or two arrowheads.
 *	the firstArrowPtr and lastArrowPtr polygons are allocated
 *	and initialized, if need be, and the end points of the line
 *	are adjusted so that a thick line doesn't stick out past
 *	the arrowheads.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConfigureArrows(canvas, pointPtr)
    Tk_Canvas canvas;			/* Canvas in which arrows will be
					 * displayed (interp and tkwin
					 * fields are needed). */
    PointItem *pointPtr;			/* Item to configure for arrows. */
{
    double *poly, *coordPtr;
    double dx, dy, length, sinTheta, cosTheta, temp;
    double fracHeight;			/* Line width as fraction of
					 * arrowhead width. */
    double backup;			/* Distance to backup end points
					 * so the line ends in the middle
					 * of the arrowhead. */
    double vertX, vertY;		/* Position of arrowhead vertex. */
    double shapeA, shapeB, shapeC;	/* Adjusted coordinates (see
					 * explanation below). */
    double width;
    Tk_State state = Tk_GetItemState(canvas, &pointPtr->header);

    if (pointPtr->numPoints <2) {
	return TCL_OK;
    }

    width = pointPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)pointPtr) {
	if (pointPtr->outline.activeWidth>width) {
	    width = pointPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (pointPtr->outline.disabledWidth>0) {
	    width = pointPtr->outline.disabledWidth;
	}
    }

    /*
     * The code below makes a tiny increase in the shape parameters
     * for the line.  This is a bit of a hack, but it seems to result
     * in displays that more closely approximate the specified parameters.
     * Without the adjustment, the arrows come out smaller than expected.
     */

    shapeA = pointPtr->arrowShapeA + 0.001;
    shapeB = pointPtr->arrowShapeB + 0.001;
    shapeC = pointPtr->arrowShapeC + width/2.0 + 0.001;

    /*
     * If there's an arrowhead on the first point of the line, compute
     * its polygon and adjust the first point of the line so that the
     * line doesn't stick out past the leading edge of the arrowhead.
     */

    fracHeight = (width/2.0)/shapeC;
    backup = fracHeight*shapeB + shapeA*(1.0 - fracHeight)/2.0;
    if (pointPtr->arrow != ARROWS_LAST) {
	poly = pointPtr->firstArrowPtr;
	if (poly == NULL) {
	    poly = (double *) ckalloc((unsigned)
		    (2*PTS_IN_ARROW*sizeof(double)));
	    poly[0] = poly[10] = pointPtr->coordPtr[0];
	    poly[1] = poly[11] = pointPtr->coordPtr[1];
	    pointPtr->firstArrowPtr = poly;
	}
	dx = poly[0] - pointPtr->coordPtr[2];
	dy = poly[1] - pointPtr->coordPtr[3];
	length = hypot(dx, dy);
	if (length == 0) {
	    sinTheta = cosTheta = 0.0;
	} else {
	    sinTheta = dy/length;
	    cosTheta = dx/length;
	}
	vertX = poly[0] - shapeA*cosTheta;
	vertY = poly[1] - shapeA*sinTheta;
	temp = shapeC*sinTheta;
	poly[2] = poly[0] - shapeB*cosTheta + temp;
	poly[8] = poly[2] - 2*temp;
	temp = shapeC*cosTheta;
	poly[3] = poly[1] - shapeB*sinTheta - temp;
	poly[9] = poly[3] + 2*temp;
	poly[4] = poly[2]*fracHeight + vertX*(1.0-fracHeight);
	poly[5] = poly[3]*fracHeight + vertY*(1.0-fracHeight);
	poly[6] = poly[8]*fracHeight + vertX*(1.0-fracHeight);
	poly[7] = poly[9]*fracHeight + vertY*(1.0-fracHeight);

	/*
	 * Polygon done.  Now move the first point towards the second so
	 * that the corners at the end of the line are inside the
	 * arrowhead.
	 */

	pointPtr->coordPtr[0] = poly[0] - backup*cosTheta;
	pointPtr->coordPtr[1] = poly[1] - backup*sinTheta;
    }

    /*
     * Similar arrowhead calculation for the last point of the line.
     */

    if (pointPtr->arrow != ARROWS_FIRST) {
	coordPtr = pointPtr->coordPtr + 2*(pointPtr->numPoints-2);
	poly = pointPtr->lastArrowPtr;
	if (poly == NULL) {
	    poly = (double *) ckalloc((unsigned)
		    (2*PTS_IN_ARROW*sizeof(double)));
	    poly[0] = poly[10] = coordPtr[2];
	    poly[1] = poly[11] = coordPtr[3];
	    pointPtr->lastArrowPtr = poly;
	}
	dx = poly[0] - coordPtr[0];
	dy = poly[1] - coordPtr[1];
	length = hypot(dx, dy);
	if (length == 0) {
	    sinTheta = cosTheta = 0.0;
	} else {
	    sinTheta = dy/length;
	    cosTheta = dx/length;
	}
	vertX = poly[0] - shapeA*cosTheta;
	vertY = poly[1] - shapeA*sinTheta;
	temp = shapeC*sinTheta;
	poly[2] = poly[0] - shapeB*cosTheta + temp;
	poly[8] = poly[2] - 2*temp;
	temp = shapeC*cosTheta;
	poly[3] = poly[1] - shapeB*sinTheta - temp;
	poly[9] = poly[3] + 2*temp;
	poly[4] = poly[2]*fracHeight + vertX*(1.0-fracHeight);
	poly[5] = poly[3]*fracHeight + vertY*(1.0-fracHeight);
	poly[6] = poly[8]*fracHeight + vertX*(1.0-fracHeight);
	poly[7] = poly[9]*fracHeight + vertY*(1.0-fracHeight);
	coordPtr[2] = poly[0] - backup*cosTheta;
	coordPtr[3] = poly[1] - backup*sinTheta;
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * LineToPostscript --
 *
 *	This procedure is called to generate Postscript for
 *	line items.
 *
 * Results:
 *	The return value is a standard Tcl result.  If an error
 *	occurs in generating Postscript then an error message is
 *	left in Tcl_GetResult(interp), replacing whatever used
 *	to be there.  If no error occurs, then Postscript for the
 *	item is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
LineToPostscript(interp, canvas, itemPtr, prepass)
    Tcl_Interp *interp;			/* Leave Postscript or error message
					 * here. */
    Tk_Canvas canvas;			/* Information about overall canvas. */
    Tk_Item *itemPtr;			/* Item for which Postscript is
					 * wanted. */
    int prepass;			/* 1 means this is a prepass to
					 * collect font information;  0 means
					 * final Postscript is being created. */
{
    PointItem *pointPtr = (PointItem *) itemPtr;
    char buffer[200];
    char *style;

    double width;
    XColor *color;
    Pixmap stipple;
    Tk_State state = Tk_GetItemState(canvas, itemPtr);

    width = pointPtr->outline.width;
    color = pointPtr->outline.color;
    stipple = pointPtr->outline.stipple;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (pointPtr->outline.activeWidth>width) {
	    width = pointPtr->outline.activeWidth;
	}
	if (pointPtr->outline.activeColor!=NULL) {
	    color = pointPtr->outline.activeColor;
	}
	if (pointPtr->outline.activeStipple!=None) {
	    stipple = pointPtr->outline.activeStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (pointPtr->outline.disabledWidth>0) {
	    width = pointPtr->outline.disabledWidth;
	}
	if (pointPtr->outline.disabledColor!=NULL) {
	    color = pointPtr->outline.disabledColor;
	}
	if (pointPtr->outline.disabledStipple!=None) {
	    stipple = pointPtr->outline.disabledStipple;
	}
    }

    if (color == NULL || pointPtr->numPoints<1 || pointPtr->coordPtr==NULL) {
	return TCL_OK;
    }

    if (pointPtr->numPoints==1) {
	sprintf(buffer, "%.15g %.15g translate %.15g %.15g",
		pointPtr->coordPtr[0], Tk_CanvasPsY(canvas, pointPtr->coordPtr[1]),
		width/2.0, width/2.0);
	Tcl_AppendResult(interp, "matrix currentmatrix\n",buffer,
		" scale 1 0 moveto 0 0 1 0 360 arc\nsetmatrix\n",          NULL);
	if (Tk_CanvasPsColor(interp, canvas, color)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	if (stipple != None) {
	    Tcl_AppendResult(interp, "clip ",          NULL);
	    if (Tk_CanvasPsStipple(interp, canvas, stipple) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    Tcl_AppendResult(interp, "fill\n",          NULL);
	}
	return TCL_OK;
    }
    /*
     * Generate a path for the line's center-line (do this differently
     * for straight lines and smoothed lines).
     */

    if ((!pointPtr->smooth) || (pointPtr->numPoints < 3)) {
	Tk_CanvasPsPath(interp, canvas, pointPtr->coordPtr, pointPtr->numPoints);
    } else {
	if ((stipple == None) && pointPtr->smooth->postscriptProc) {
	    pointPtr->smooth->postscriptProc(interp, canvas,
		    pointPtr->coordPtr, pointPtr->numPoints, pointPtr->splineSteps);
	} else {
	    /*
	     * Special hack: Postscript printers don't appear to be able
	     * to turn a path drawn with "curveto"s into a clipping path
	     * without exceeding resource limits, so TkMakeBezierPostscript
	     * won't work for stippled curves.  Instead, generate all of
	     * the intermediate points here and output them into the
	     * Postscript file with "lineto"s instead.
	     */

	    double staticPoints[2*MAX_STATIC_POINTS];
	    double *pointPtr;
	    int numPoints;

	    numPoints = pointPtr->smooth->coordProc(canvas, (double *) NULL,
		    pointPtr->numPoints, pointPtr->splineSteps, (XPoint *) NULL,
		    (double *) NULL);
	    pointPtr = staticPoints;
	    if (numPoints > MAX_STATIC_POINTS) {
		pointPtr = (double *) ckalloc((unsigned)
			(numPoints * 2 * sizeof(double)));
	    }
	    numPoints = pointPtr->smooth->coordProc(canvas, pointPtr->coordPtr,
		    pointPtr->numPoints, pointPtr->splineSteps, (XPoint *) NULL,
		    pointPtr);
	    Tk_CanvasPsPath(interp, canvas, pointPtr, numPoints);
	    if (pointPtr != staticPoints) {
		ckfree((char *) pointPtr);
	    }
	}
    }

    /*
     * Set other line-drawing parameters and stroke out the line.
     */

    style = "0 setlinecap\n";
    if (pointPtr->capStyle == CapRound) {
	style = "1 setlinecap\n";
    } else if (pointPtr->capStyle == CapProjecting) {
	style = "2 setlinecap\n";
    }
    Tcl_AppendResult(interp, style,          NULL);
    style = "0 setlinejoin\n";
    if (pointPtr->joinStyle == JoinRound) {
	style = "1 setlinejoin\n";
    } else if (pointPtr->joinStyle == JoinBevel) {
	style = "2 setlinejoin\n";
    }
    Tcl_AppendResult(interp, style,          NULL);

    if (Tk_CanvasPsOutline(canvas, itemPtr,
	    &(pointPtr->outline)) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Output polygons for the arrowheads, if there are any.
     */

    if (pointPtr->firstArrowPtr != NULL) {
	if (stipple != None) {
	    Tcl_AppendResult(interp, "grestore gsave\n",
		             NULL);
	}
	if (ArrowheadPostscript(interp, canvas, pointPtr,
		pointPtr->firstArrowPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (pointPtr->lastArrowPtr != NULL) {
	if (stipple != None) {
	    Tcl_AppendResult(interp, "grestore gsave\n",          NULL);
	}
	if (ArrowheadPostscript(interp, canvas, pointPtr,
		pointPtr->lastArrowPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ArrowheadPostscript --
 *
 *	This procedure is called to generate Postscript for
 *	an arrowhead for a line item.
 *
 * Results:
 *	The return value is a standard Tcl result.  If an error
 *	occurs in generating Postscript then an error message is
 *	left in Tcl_GetResult(interp), replacing whatever used
 *	to be there.  If no error occurs, then Postscript for the
 *	arrowhead is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
ArrowheadPostscript(interp, canvas, pointPtr, arrowPtr)
    Tcl_Interp *interp;			/* Leave Postscript or error message
					 * here. */
    Tk_Canvas canvas;			/* Information about overall canvas. */
    PointItem *linePtr;			/* Line item for which Postscript is
					 * being generated. */
    double *arrowPtr;			/* Pointer to first of five points
					 * describing arrowhead polygon. */
{
    Pixmap stipple;
    Tk_State state = Tk_GetItemState(canvas, &linePtr->header);

    stipple = linePtr->outline.stipple;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)linePtr) {
	if (linePtr->outline.activeStipple!=None) {
	    stipple = linePtr->outline.activeStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (linePtr->outline.activeStipple!=None) {
	    stipple = linePtr->outline.disabledStipple;
	}
    }

    Tk_CanvasPsPath(interp, canvas, arrowPtr, PTS_IN_ARROW);
    if (stipple != None) {
	Tcl_AppendResult(interp, "clip ",          NULL);
	if (Tk_CanvasPsStipple(interp, canvas, stipple)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "fill\n",          NULL);
    }
    return TCL_OK;
}
#endif

/* Local variables: */
/* c-basic-offset: 4 */
/* End. */
