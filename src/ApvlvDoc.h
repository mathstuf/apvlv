/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @PPCPPFILE ApvlvDoc.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_DOC_H_
#define _APVLV_DOC_H_

#include "ApvlvFile.h"
#include "ApvlvCore.h"
#include "ApvlvUtil.h"

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <iostream>
#include <vector>
#include <map>
#include <list>

using namespace std;

namespace apvlv
{
  struct PrintData
  {
    ApvlvFile *file;
    guint frmpn, endpn;
  };

  struct ApvlvDocPosition
  {
    int pagenum;
    double scrollrate;
  };

  typedef map < char, ApvlvDocPosition > ApvlvDocPositionMap;

  struct ApvlvWord
  {
    ApvlvPos pos;
    string word;
  };

  struct ApvlvLine
  {
    ApvlvPos pos;
    vector < ApvlvWord > mWords;
  };

  typedef enum {
                DISPLAY_TYPE_IMAGE = 0,
                DISPLAY_TYPE_HTML = 1,
                DISPLAY_TYPE_COUNT,
  } DISPLAY_TYPE;

  DISPLAY_TYPE get_display_type_by_filename (const char *filename);

  class ApvlvDoc;
  class ApvlvDocCache
  {
  public:
    ApvlvDocCache (ApvlvFile *);

    ~ApvlvDocCache ();

    void set (guint p, double zm, guint rot, bool delay = true);

    static void load (ApvlvDocCache *);

    guint getpagenum ();

    guchar *getdata (bool wait);

    GdkPixbuf *getbuf (bool wait);

    double getwidth ();

    double getheight ();

    ApvlvLinks *getlinks ();

    bool canselect ();
    bool mInverted;

    ApvlvWord *getword (int x, int y);

    ApvlvLine *getline (double x, double y);

  private:
    ApvlvFile * mFile;
    ApvlvLinks *mLinks;
    double mZoom;
    double mRotate;
    gint mPagenum;
    guchar *mData;
    gint mSize;
    GdkPixbuf *mBuf;
    gint mWidth;
    gint mHeight;

    vector < ApvlvLine > *mLines;

    void preparelines (double x1, double y1, double x2, double y2);
    ApvlvPos prepare_add (ApvlvPos & last, ApvlvPoses * results,
			  const char *word);
  };

  class ApvlvDocStatus:public ApvlvCoreStatus
  {
  public:
    ApvlvDocStatus (ApvlvDoc *);

    ~ApvlvDocStatus ();

    void active (bool act);

    void setsize (int, int);

    void show (bool mContinuous = false);

  private:
    ApvlvDoc * mDoc;
#define AD_STATUS_SIZE   4
    GtkWidget *mStlab[AD_STATUS_SIZE];
  };

  class ApvlvDoc:public ApvlvCore
  {
  public:
    ApvlvDoc (ApvlvView *, DISPLAY_TYPE type, int w, int h, const char *zm = "NORMAL", bool cache = false);

    ~ApvlvDoc ();

    void setactive (bool act);

    ApvlvDoc *copy ();

    bool usecache ();

    void usecache (bool use);

    bool loadfile (string & filename, bool check = true);

    bool loadfile (const char *src, bool check = true);

    int pagenumber ();

    bool print (int ct);

    bool totext (const char *name);

    bool rotate (int ct = 90);

    void markposition (const char s);

    void setzoom (const char *z);

    void jump (const char s);

    void showpage (int p, double s = 0.00);

    void nextpage (int times = 1);

    void prepage (int times = 1);

    void halfnextpage (int times = 1);

    void halfprepage (int times = 1);

    void scrollup (int times);
    void scrolldown (int times);
    void scrollleft (int times);
    void scrollright (int times);

    void scrollupweb (int times);
    void scrolldownweb (int times);
    void scrollleftweb (int times);
    void scrollrightweb (int times);

    bool search (const char *str, bool reverse = false);

    bool continuous ();

    returnType process (int hastimes, int times, guint keyval);

    void gotolink (int ct);

    void returnlink (int ct);

    void goto_newer_cursor_position(int ct);
    void goto_older_cursor_position(int ct);
    void set_cursor_position(int pagenum, double scrollrate);
    void push_cursor_position(int pagenum, double scrollrate);

    void srtranslate(int &rtimes, double &sr, bool single2continuous);

    static void webview_resource_load_started_cb (WebKitWebView *web_view,
                                                  WebKitWebResource *resource,
                                                  WebKitURIRequest *request,
                                                  ApvlvDoc *doc);
    static void webview_load_changed_cb (WebKitWebView *web_view,
                                         WebKitLoadEvent event,
                                         ApvlvDoc *doc);
    static gboolean webview_context_menu_cb (WebKitWebView       *web_view,
                                             WebKitContextMenu   *context_menu,
                                             GdkEvent            *event,
                                             WebKitHitTestResult *hit_test_result,
                                             ApvlvDoc *doc);

  private:
    void blank (int x, int y);

    void blankarea (int x1, int y1, int x2, int y2, guchar *, int width,
		    int height);

    void blankaction (double x, double y);

    void togglevisual (int type);

    void scrollweb (int times, int w, int h);
    void scrollwebto (double xrate, double yrate);

    int yank (int times);

    void eventpos (double x, double y, double *rx, double *ry);

    returnType subprocess (int ct, guint key);

    bool status_show ();

    int convertindex (int p);

    void markselection ();

    bool needsearch (const char *str, bool reverse = false);

    void refresh ();

    bool reload ();

    bool savelastposition (const char *filename);

    bool loadlastposition (const char *filename);

    static void apvlv_doc_on_mouse (GtkAdjustment *, ApvlvDoc *);

    static gboolean apvlv_doc_first_copy_cb (gpointer);

    static void apvlv_doc_button_event (GtkEventBox * box,
					GdkEventButton * ev, ApvlvDoc *);

    static void apvlv_doc_motion_event (GtkWidget *, GdkEventMotion *,
					ApvlvDoc *);

    static void apvlv_doc_copytoclipboard_cb (GtkMenuItem * item, ApvlvDoc *);

    static void begin_print (GtkPrintOperation * operation,
			     GtkPrintContext * context, PrintData * data);
    static void draw_page (GtkPrintOperation * operation,
			   GtkPrintContext * context,
			   gint page_nr, PrintData * data);
    static void end_print (GtkPrintOperation * operation,
			   GtkPrintContext * context, PrintData * data);

    static void apvlv_doc_monitor_callback (GFileMonitor *, GFile *, GFile *, GFileMonitorEvent, ApvlvDoc *);

    enum
      { VISUAL_NONE, VISUAL_V, VISUAL_CTRL_V };
    gint mInVisual;
    gint mBlankx1, mBlanky1;
    gint mBlankx2, mBlanky2;
    gint mLastpress;
    gint mCurx, mCury;

    ApvlvDocPositionMap mPositions;
    vector < ApvlvDocPosition > mLinkPositions;

    int mCursorPosition;
    vector < ApvlvDocPosition > mOlderCursorPositions;

    ApvlvDocCache *mCurrentCache1, *mCurrentCache2, *mCurrentCache3;
    ApvlvDocCache *newcache (int pagenum);
    void deletecache (ApvlvDocCache * ac);

    DISPLAY_TYPE mDisplayType;
    // image viewer
    GtkWidget *mImg1, *mImg2, *mImg3;
    GtkWidget *mWeb1, *mWeb2, *mWeb3;
  };

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */
