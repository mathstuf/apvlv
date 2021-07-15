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
/* @CPPFILE ApvlvWindow.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvView.h"
#include "ApvlvUtil.h"
#include "ApvlvParams.h"
#include "ApvlvWindow.h"

#include <gtk/gtk.h>

namespace apvlv
{
    ApvlvWindow *ApvlvWindow::m_curWindow = nullptr;

    ApvlvWindow::ApvlvWindow (ApvlvCore *doc)
    {
      mIsClose = false;

      type = AW_CORE;
      if (doc == nullptr)
        {
          mCore = new ApvlvDoc (mCore->mView, DISPLAY_TYPE_IMAGE, 0, 0, gParams->values ("zoom"));
        }
      else
        {
          mCore = doc;
        }
      m_son = m_daughter = m_parent = nullptr;
    }

    ApvlvWindow::~ApvlvWindow ()
    {
      if (mIsClose)
        {
          return;
        }

      debug ("delete window: %p", this);

      mIsClose = true;

      if (m_parent != nullptr)
        {
          if (m_parent->m_son == this)
            {
              m_parent->m_son = nullptr;
            }
          else if (m_parent->m_daughter == this)
            {
              m_parent->m_daughter = nullptr;
            }
        }

      if (type == AW_CORE)
        {
          mCore->inuse (false);
        }

      else if (type == AW_SP || type == AW_VSP)
        {
          if (m_son != nullptr)
            {
              ApvlvWindow *win = m_son;
              m_son = nullptr;
              delete win;
            }
          if (m_daughter != nullptr)
            {
              ApvlvWindow *win = m_daughter;
              m_daughter = nullptr;
              delete win;
            }

          g_object_unref (mPaned);
        }
    }

    GtkWidget *ApvlvWindow::widget ()
    {
      if (type == AW_CORE)
        {
          return mCore->widget ();
        }
      else if (type == AW_SP || type == AW_VSP)
        {
          return mPaned;
        }
      else
        {
          debug ("type error: %d", type);
          return nullptr;
        }
    }

    void ApvlvWindow::setcurrentWindow (ApvlvWindow *pre, ApvlvWindow *win)
    {
      if (pre != nullptr && pre->type == AW_CORE)
        {
          pre->mCore->setactive (false);
        }

      if (win->type == AW_CORE)
        {
          win->mCore->setactive (true);
        }

      m_curWindow = win;
    }

    void ApvlvWindow::delcurrentWindow ()
    {
      asst (currentWindow ()->istop () == false);

      ApvlvWindow *crwin = currentWindow ();

      ApvlvWindow *pwin = crwin->m_parent;
      ApvlvWindow *child =
          crwin == pwin->m_son ? pwin->m_daughter : pwin->m_son;
      ApvlvWindow *cwin = pwin->unbirth (crwin, child);
      setcurrentWindow (nullptr, cwin);
    }

    ApvlvWindow *ApvlvWindow::currentWindow ()
    {
      return m_curWindow;
    }

    returnType ApvlvWindow::process (int ct, guint key)
    {
      ApvlvWindow *nwin;
      debug ("input [%d]", key);

      switch (key)
        {
          case CTRL ('w'):
          case 'k':
          case 'j':
          case 'h':
          case 'l':
            nwin = getneighbor (ct, key);
          if (nwin != nullptr)
            {
              setcurrentWindow (this, nwin);
            }
          break;

          case '-':
            smaller (ct);
          break;

          case '+':
            bigger (ct);
          break;

          default:
            break;
        }
      return MATCH;
    }

    ApvlvWindow *ApvlvWindow::getneighbor (int ct, guint key)
    {
      switch (key)
        {
          case CTRL ('w'):
            return getnext (ct);
          case 'k':
            return getkj (1, false);
          case 'j':
            return getkj (1, true);
          case 'h':
            return gethl (1, false);
          case 'l':
            return gethl (1, true);
          default:
            break;
        }

      return nullptr;
    }

    inline ApvlvWindow *ApvlvWindow::getkj (__attribute__((unused)) int num, bool down)
    {
      ApvlvWindow *cw, *w, *nw, *fw;
      bool right = false;

      asst (type == AW_CORE);
      for (cw = fw = nullptr, w = this; w != nullptr; cw = w, w = w->m_parent)
        {
          if (w->type == AW_SP)
            {
              if ((cw == w->m_daughter && down)
                  || (cw == w->m_son && !down))
                {
                  continue;
                }
              else
                {
                  fw = down ? w->m_daughter : w->m_son;
                  break;
                }
            }
          else if (w->type == AW_VSP)
            {
              if (cw != nullptr && cw == w->m_daughter)
                {
                  right = true;
                }
              else
                {
                  right = false;
                }
            }
        }

      for (nw = w = fw; w != nullptr;)
        {
          if (w->type == AW_CORE)
            {
              nw = w;
              break;
            }
          else if (w->type == AW_SP)
            {
              w = down ? w->m_son : w->m_daughter;
            }
          else if (w->type == AW_VSP)
            {
              w = right ? w->m_daughter : w->m_son;
            }
          else
            {
              debug ("error type: %d", w->type);
              return nullptr;
            }
        }

      return nw;
    }

    inline ApvlvWindow *ApvlvWindow::gethl (__attribute__((unused)) int num, bool right)
    {
      ApvlvWindow *cw, *w, *nw, *fw;
      bool down = false;

      asst (type == AW_CORE);
      for (cw = fw = nullptr, w = this; w != nullptr; cw = w, w = w->m_parent)
        {
          if (w->type == AW_VSP)
            {
              if ((cw == w->m_daughter && right)
                  || (cw == w->m_son && !right))
                {
                  continue;
                }
              else
                {
                  fw = right ? w->m_daughter : w->m_son;
                  break;
                }
            }
          else if (w->type == AW_SP)
            {
              if (cw != nullptr && cw == w->m_daughter)
                {
                  down = true;
                }
              else
                {
                  down = false;
                }
            }
        }

      for (nw = w = fw; w != nullptr;)
        {
          if (w->type == AW_CORE)
            {
              nw = w;
              break;
            }
          else if (w->type == AW_VSP)
            {
              w = right ? w->m_son : w->m_daughter;
            }
          else if (w->type == AW_SP)
            {
              w = down ? w->m_daughter : w->m_son;
            }
          else
            {
              debug ("error type: %d", w->type);
              return nullptr;
            }
        }

      return nw;
    }

    ApvlvWindow *ApvlvWindow::getnext (int num)
    {
      ApvlvWindow *n = getkj (num, true);
      if (n == nullptr)
        {
          n = gethl (num, true);
          if (n == nullptr)
            {
              n = gethl (num, false);
              if (n == nullptr)
                n = getkj (num, false);
            }
        }
      return n;
    }

    // birth a new AW_CORE window, and the new window beyond the input doc
    // this made a AW_CORE window to AW_SP|AW_VSP
    ApvlvWindow *ApvlvWindow::birth (bool vsp, ApvlvCore *doc)
    {
      asst (type == AW_CORE);

      if (doc == mCore)
        {
          debug ("can't birth with orign doc, copy it");
          doc = nullptr;
        }

      if (doc == nullptr)
        {
          doc = mCore->copy ();
          mCore->mView->regloaded (doc);
        }

      if (doc == nullptr)
        {
          mCore->mView->errormessage ("can't split");
          return this;
        }

      auto *nwindow = new ApvlvWindow (doc);
      nwindow->m_parent = this;
      m_son = nwindow;

      auto *nwindow2 = new ApvlvWindow (mCore);
      nwindow2->m_parent = this;
      m_daughter = nwindow2;

#if GTK_CHECK_VERSION (3, 0, 0)
      mPaned = gtk_paned_new (!vsp ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
#else
      mPaned = vsp == false ? gtk_vpaned_new () : gtk_hpaned_new ();
#endif
      g_object_ref (mPaned);
      g_signal_connect (G_OBJECT (mPaned), "button-release-event",
                        G_CALLBACK (apvlv_window_paned_resized_cb), this);

      if (m_parent)
        {
          void (*panedcb) (GtkPaned *, GtkWidget *);
          GtkWidget *parent = m_parent->mPaned;
          if (gtk_paned_get_child1 (GTK_PANED (parent)) == widget ())
            {
              panedcb = gtk_paned_add1;
            }
          else
            {
              panedcb = gtk_paned_add2;
            }

          gtk_container_remove (GTK_CONTAINER (parent), widget ());
          panedcb (GTK_PANED (parent), mPaned);
        }
      else
        {
          replace_widget (widget (), mPaned);
        }

      gtk_paned_pack1 (GTK_PANED (mPaned), nwindow->widget (), TRUE, TRUE);
      gtk_paned_pack2 (GTK_PANED (mPaned), nwindow2->widget (), TRUE, TRUE);

      type = !vsp ? AW_SP : AW_VSP;
      if (type == AW_SP)
        {
          nwindow->setsize (mWidth, mHeight / 2);
          nwindow2->setsize (mWidth, mHeight / 2);
        }
      else if (type == AW_VSP)
        {
          nwindow->setsize (mWidth / 2, mHeight);
          nwindow2->setsize (mWidth / 2, mHeight);
        }

      gtk_widget_show_all (mPaned);

      setcurrentWindow (nwindow2, nwindow);
      return nwindow;
    }

    // unbirth a child
    // @param 1, be delete
    // @param 2, be unbirth, that is up to the parent
    // return the new child
    ApvlvWindow *ApvlvWindow::unbirth (ApvlvWindow *dead, ApvlvWindow *child)
    {
      asst (type == AW_SP || type == AW_VSP);

      if (m_parent)
        {
          void (*panedcb) (GtkPaned *, GtkWidget *);
          GtkWidget *parent = m_parent->mPaned;
          if (gtk_paned_get_child1 (GTK_PANED (parent)) == mPaned)
            {
              panedcb = gtk_paned_add1;
            }
          else
            {
              panedcb = gtk_paned_add2;
            }

          gtk_container_remove (GTK_CONTAINER (mPaned), child->widget ());
          gtk_container_remove (GTK_CONTAINER (parent), mPaned);
          panedcb (GTK_PANED (parent), child->widget ());
        }
      else
        {
          gtk_container_remove (GTK_CONTAINER (mPaned), child->widget ());
          replace_widget (mPaned, child->widget ());
        }

      if (child->type == AW_CORE)
        {
          ApvlvCore *doc = child->getCore ();
          type = AW_CORE;
          mCore = doc;
        }
      else if (child->type == AW_SP || child->type == AW_VSP)
        {
          type = child->type;
          mPaned = child->mPaned;
          m_son = child->m_son;
          m_son->m_parent = this;
          m_daughter = child->m_daughter;
          m_daughter->m_parent = this;
          child->type = AW_NONE;
        }

      gtk_widget_show_all (widget ());

      delete dead;
      delete child;

      ApvlvWindow *win;
      for (win = this; win->type != AW_CORE; win = win->m_son);

      return win;
    }

    bool ApvlvWindow::istop () const
    {
      return m_parent == nullptr;
    }

    void ApvlvWindow::getsize (int *width, int *height) const
    {
      if (width)
        {
          *width = mWidth;
        }
      if (height)
        {
          *height = mHeight;
        }
    }

    void ApvlvWindow::setsize (int width, int height)
    {
      mWidth = width - 2;
      mHeight = height - 2;
      //    debug ("mWidth: %d, mHeight: %d", mWidth, mHeight);

      if (type == AW_CORE)
        {
          mCore->setsize (mWidth, mHeight);
        }
      else if (type == AW_SP || type == AW_VSP)
        {
          g_timeout_add (50, apvlv_window_resize_children_cb, this);
        }
    }

    void ApvlvWindow::setCore (ApvlvCore *doc)
    {
      debug ("widget (): %p, doc->widget (): %p", widget (), doc->widget ());
      if (type == AW_CORE)
        {
          mCore->inuse (false);
        }
      replace_widget (widget (), doc->widget ());
      doc->inuse (true);
      type = AW_CORE;
      mCore = doc;
    }

    ApvlvCore *ApvlvWindow::getCore ()
    {
      asst (type == AW_CORE);
      ApvlvCore *rdoc = mCore;
      return rdoc;
    }

    void ApvlvWindow::smaller (int times)
    {
      if (m_parent == nullptr)
        return;

      int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
      int len = 20 * times;
      m_parent->m_son == this ? val -= len : val += len;
      gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);

      m_parent->resize_children ();
    }

    void ApvlvWindow::bigger (int times)
    {
      if (m_parent == nullptr)
        return;

      int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
      int len = 20 * times;
      m_parent->m_son == this ? val += len : val -= len;
      gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);

      m_parent->resize_children ();
    }

    gboolean
    ApvlvWindow::apvlv_window_paned_resized_cb (__attribute__((unused)) GtkWidget *wid,
                                                __attribute__((unused)) GdkEventButton *but,
                                                ApvlvWindow *win)
    {
      win->resize_children ();
      return FALSE;
    }

    gboolean ApvlvWindow::resize_children ()
    {
      int mw1 = mWidth, mw2 = mWidth, mh1 = mHeight, mh2 = mHeight;
      int mi, ma;
      int mv = gtk_paned_get_position (GTK_PANED (mPaned));

      gtk_widget_style_get (mPaned, "min-position", &mi, "max-position", &ma, nullptr);
      int ms = ma - mi;
      if (ms != 0)
        {
          if (type == AW_SP)
            {
              mh1 = (mHeight * (mv - mi)) / ms - 1;
              mh2 = mHeight - mh1 - 1;
            }
          else if (type == AW_VSP)
            {
              mw1 = (mWidth * (mv - mi)) / ms - 1;
              mw2 = mWidth - mw1 - 1;
            }

          m_son->setsize (mw1, mh1);
          m_daughter->setsize (mw2, mh2);

          return TRUE;
        }
      else
        {
          return FALSE;
        }
    }

    gboolean ApvlvWindow::apvlv_window_resize_children_cb (gpointer data)
    {
      auto *win = (ApvlvWindow *) data;
      return win->resize_children () != TRUE;
    }
}

// Local Variables:
// mode: c++
// End:
