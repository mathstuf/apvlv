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
/* @CPPFILE ApvlvDir.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2009/01/03 23:28:26 Alf*/

#include "ApvlvView.h"
#include "ApvlvParams.h"
#include "ApvlvDir.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>

#include <cerrno>
#include <sys/stat.h>
#include <iostream>

namespace apvlv
{
    ApvlvDirNode::ApvlvDirNode (ApvlvDir *dir, GtkTreeIter *ir, gint p, const char *title)
    {
      mDir = dir;
      mPagenum = p;
      realname = nullptr;

      GtkTreeIter nitr[1];
      gtk_tree_store_append (dir->mStore, itr, ir);
      *nitr = *itr;

      GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_size (iconreg.c_str (), 40,
                                                         20, nullptr);
      if (pix)
        {
          gtk_tree_store_set (dir->mStore, nitr, 0, this, 1, pix, 2,
                              title, -1);
          g_object_unref (pix);
        }
      else
        {
          gtk_tree_store_set (dir->mStore, nitr, 0, this, 2,
                              title, -1);
        }

    }

    ApvlvDirNode::ApvlvDirNode (ApvlvDir *dir, GtkTreeIter *ir, bool isdir,
                                const char *real, const char *file)
    {
      mDir = dir;
      mPagenum = isdir ? -1 : 0;
      g_snprintf (filename, sizeof filename, "%s", file);
      realname = g_strdup (real);

      if (isdir && gParams->valuei ("autoreload") > 0)
        {
          mGFile = g_file_new_for_path (realname);
          if (mGFile)
            {
              mGMonitor = g_file_monitor (mGFile, G_FILE_MONITOR_NONE, nullptr, nullptr);
            }
          else
            {
              mGMonitor = nullptr;
            }

          if (mGMonitor)
            {
              g_file_monitor_set_rate_limit (mGMonitor, gParams->valuei ("autoreload") * 1000);
              g_signal_connect (G_OBJECT (mGMonitor), "changed", G_CALLBACK (apvlv_dirnode_monitor_callback), this);
            }
        }

      GtkTreeIter nitr[1];
      gtk_tree_store_append (dir->mStore, nitr, ir);
      *itr = *nitr;

      GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_size (isdir ? icondir.c_str () : iconpdf.c_str (), 40,
                                                         20, nullptr);
      if (pix)
        {
          gtk_tree_store_set (dir->mStore, nitr, 0, this, 1, pix, 2,
                              file, -1);
          g_object_unref (pix);
        }
      else
        {
          gtk_tree_store_set (dir->mStore, nitr, 0, this, 2,
                              file, -1);
        }
    }

    ApvlvDirNode::~ApvlvDirNode ()
    {
      if (realname)
        {
          g_free (realname);
        }
    }

    void
    ApvlvDirNode::apvlv_dirnode_monitor_callback (__attribute__((unused)) GFileMonitor *mon, GFile *gf1, __attribute__((unused)) GFile *gf2, GFileMonitorEvent ev, ApvlvDirNode *node)
    {
      node->mDir->apvlv_dir_change_node (node, gf1, ev);
    }

    bool ApvlvDirNode::dest (const char **path, int *pn)
    {
      if (mPagenum == 0 && path != nullptr)
        {
          *path = realname;
          return true;
        }

      else if (mPagenum > 0 && pn != nullptr)
        {
          *pn = mPagenum;
          return true;
        }

      return false;
    }

    const char *ApvlvDirNode::phrase ()
    {
      return filename;
    }

    GtkTreeIter *ApvlvDirNode::iter ()
    {
      return itr;
    }

    ApvlvDir::ApvlvDir (ApvlvView *view) : ApvlvCore (view)
    {
      mReady = false;

      mProCmd = 0;

      mRotatevalue = 0;

      mDirNodes = nullptr;

      mIndex = nullptr;

      mFile = nullptr;

      mStore =
          gtk_tree_store_new (3, G_TYPE_POINTER, G_TYPE_OBJECT, G_TYPE_STRING);
      mDirView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mStore));
      gtk_container_add (GTK_CONTAINER (mScrollwin), mDirView);
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mDirView), FALSE);

      mSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mDirView));
      g_signal_connect (G_OBJECT (mSelection), "changed",
                        G_CALLBACK (apvlv_dir_on_changed), this);

      /* Title Column */
      GtkCellRenderer *renderer0 = gtk_cell_renderer_pixbuf_new ();
      GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
      GtkTreeViewColumn *column = gtk_tree_view_column_new ();
      gtk_tree_view_column_pack_start (column, renderer0, FALSE);
      gtk_tree_view_column_pack_start (column, renderer, FALSE);
      gtk_tree_view_column_add_attribute (column, renderer0, "pixbuf", 1);
      gtk_tree_view_column_add_attribute (column, renderer, "text", 2);
      //gtk_tree_view_column_set_sort_column_id (column, 2);
      //gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
      gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);
      gtk_tree_view_column_clicked (column);

      mStatus = new ApvlvDirStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, TRUE, TRUE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      gtk_widget_show_all (mVbox);
    }

    bool ApvlvDir::reload ()
    {
      gtk_tree_store_clear (mStore);

      if (mDirNodes)
        {
          for (GList *list = mDirNodes; list; list = g_list_next (list))
            {
              auto *info = (ApvlvDirNode *) list->data;
              delete info;
            }
          g_list_free (mDirNodes);
          mDirNodes = nullptr;
        }

      if (mIndex != nullptr)
        {
          mFile->free_index (mIndex);
          mIndex = nullptr;
        }

      if (mFile != nullptr)
        {
          delete mFile;
          mFile = nullptr;
        }

      loadfile (mFilestr.c_str (), FALSE);

      return true;
    }

    bool ApvlvDir::loadfile (const char *path, bool check)
    {
      gchar *rpath;

      if (path == nullptr
          || *path == '\0'
          || (rpath = g_locale_from_utf8 (path, -1, nullptr, nullptr, nullptr)) == nullptr)
        {
          mView->errormessage ("path error: %s", path ? path : "No path");
          return false;
        }

      struct stat buf[1];
      int ret = stat (rpath, buf);
      g_free (rpath);
      if (ret < 0)
        {
          mView->errormessage ("stat error: %d:%s", errno, strerror (errno));
          return false;
        }

      if (S_ISDIR (buf->st_mode))
        {
          mType = CORE_DIR;
          mReady = walk_dir_path_index (nullptr, path);
        }
      else
        {
          mType = CORE_CONTENT;
          mFile = ApvlvFile::newFile (path);

          if (mFile != nullptr && (mIndex = mFile->new_index ()) != nullptr)
            {
              for (auto itr = mIndex->children.begin ();
                   itr != mIndex->children.end (); ++itr)
                {
                  bool ready = walk_file_index (nullptr, itr);
                  if (!mReady)
                    {
                      mReady = ready;
                    }
                }
            }
          else
            {
              mReady = false;
            }
        }

      if (mReady)
        {
          mFilestr = path;
          g_timeout_add (50,
                         (gboolean (*) (gpointer)) apvlv_dir_first_select_cb,
                         this);

          if (gParams->valuei ("autoreload") > 0)
            {
              mGFile = g_file_new_for_path (path);
              if (mGFile)
                {
                  mGMonitor = g_file_monitor (mGFile, G_FILE_MONITOR_NONE, nullptr, nullptr);
                }
              else
                {
                  mGMonitor = nullptr;
                }

              if (mGMonitor)
                {
                  g_file_monitor_set_rate_limit (mGMonitor, gParams->valuei ("autoreload") * 1000);
                  g_signal_connect (G_OBJECT (mGMonitor), "changed", G_CALLBACK (apvlv_dir_monitor_callback), this);
                }
            }
        }

      return mReady;
    }

    ApvlvDir::~ApvlvDir ()
    {
      if (mDirNodes)
        {
          for (GList *list = mDirNodes; list; list = g_list_next (list))
            {
              auto *info = (ApvlvDirNode *) list->data;
              delete info;
            }
          g_list_free (mDirNodes);
        }

      if (mIndex != nullptr)
        {
          mFile->free_index (mIndex);
        }

      delete mFile;

      delete mStatus;
    }

    returnType ApvlvDir::subprocess (__attribute__((unused)) int ct, __attribute__((unused)) guint key)
    {
      return NO_MATCH;
    }

    returnType ApvlvDir::process (int has, int ct, guint key)
    {
      if (ct == 0)
        {
          ct++;
        }

      if (mProCmd != 0)
        {
          return subprocess (ct, key);
        }

      switch (key)
        {
          case ':':
          case '/':
          case '?':
            mView->promptcommand (char (key));
          return NEED_MORE;

          case 'n':
            markposition ('\'');
          search ("", false);
          break;
          case 'N':
            markposition ('\'');
          search ("", true);
          break;

          case 't':
          case 'o':
          case GDK_KEY_Return:
            enter (key);
          break;

          case 'H':
            scrollto (0.0);
          break;
          case 'M':
            scrollto (0.5);
          break;
          case 'L':
            scrollto (1.0);
          break;

          case GDK_KEY_Up:
          case 'k':
            scrollup (ct);
          break;

          case CTRL ('n'):
          case CTRL ('j'):
          case GDK_KEY_Down:
          case 'j':
            scrolldown (ct);
          break;

          case GDK_KEY_BackSpace:
          case GDK_KEY_Left:
          case CTRL ('h'):
          case 'h':
            scrollleft (ct);
          break;

          case GDK_KEY_space:
          case GDK_KEY_Right:
          case CTRL ('l'):
          case 'l':
            scrollright (ct);
          break;

          case 'R':
            reload ();
          break;

          default:
            return NO_MATCH;
        }

      return MATCH;
    }

    bool ApvlvDir::enter (guint key)
    {
      ApvlvDirNode *node;

      gtk_tree_model_get (GTK_TREE_MODEL (mStore), &mCurrentIter, 0, &node, -1);
      if (node == nullptr)
        {
          return false;
        }

      const char *name = nullptr;
      int pn = -1;
      if (!node->dest (&name, &pn))
        {
          return false;
        }

      if (key == GDK_KEY_Return)
        {
          bool disable_content = false;
          if (name == nullptr)
            {
              name = filename ();
              disable_content = true;
            }
          return mView->newview (name, pn, disable_content);
        }

      ApvlvCore *ndoc = nullptr;
      if (name != nullptr)
        {
          if (gParams->valueb ("content"))
            {
              ndoc = new ApvlvDir (mView);
              if (!ndoc->loadfile (name, true))
                {
                  delete ndoc;
                  ndoc = nullptr;
                }
            }

          if (ndoc == nullptr)
            {
              DISPLAY_TYPE type = get_display_type_by_filename (name);
              ndoc =
                  new ApvlvDoc (mView, type, gParams->values ("zoom"),
                                gParams->valueb ("cache"));
              if (!ndoc->loadfile (name, true))
                {
                  delete ndoc;
                  ndoc = nullptr;
                }
            }
        }
      else
        {
          name = filename ();
          DISPLAY_TYPE type = get_display_type_by_filename (name);
          ndoc =
              new ApvlvDoc (mView, type, gParams->values ("zoom"),
                            gParams->valueb ("cache"));
          if (!ndoc->loadfile (filename (), true))
            {
              delete ndoc;
              ndoc = nullptr;
            }

          if (ndoc != nullptr)
            {
              ((ApvlvDoc *) ndoc)->showpage (pn, 0.0);
            }
        }

      if (ndoc == nullptr)
        {
          return false;
        }

      mView->regloaded (ndoc);

      switch (key)
        {
          case 'o':
            mView->currentWindow ()->birth (false, ndoc);
          break;

          case 't':
            mView->newtab (ndoc);
          break;

          default:
            return false;
        }

      return true;
    }

    void ApvlvDir::scrollup (int times)
    {
      GtkTreePath *path;

      if (!mReady
          || (path =
                  gtk_tree_model_get_path (GTK_TREE_MODEL (mStore),
                                           &mCurrentIter)) == nullptr)
        {
          return;
        }

      for (gboolean ret = TRUE; times > 0 && ret; times--)
        {
          ret = gtk_tree_path_prev (path);
        }

      gtk_tree_model_get_iter (GTK_TREE_MODEL (mStore), &mCurrentIter, path);
      gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, nullptr, TRUE,
                                    0.5, 0.0);
      gtk_tree_path_free (path);

      mStatus->show (false);
    }

    void ApvlvDir::scrolldown (int times)
    {
      if (!mReady)
        return;

      GtkTreeIter itr;
      gboolean ret;

      for (ret = TRUE, itr = mCurrentIter; times > 0 && ret; times--)
        {
          mCurrentIter = itr;
          ret = gtk_tree_model_iter_next (GTK_TREE_MODEL (mStore), &itr);
          if (ret)
            {
              mCurrentIter = itr;
            }
        }

      gtk_tree_selection_select_iter (mSelection, &mCurrentIter);

      GtkTreePath *path =
          gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
      if (path)
        {
          gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, nullptr,
                                        TRUE, 0.5, 0.0);
          gtk_tree_path_free (path);
        }

      mStatus->show (false);
    }

    void ApvlvDir::scrollleft (int times)
    {
      if (!mReady)
        return;

      GtkTreeIter itr;
      for (gboolean ret = TRUE; times > 0 && ret; times--)
        {
          ret =
              gtk_tree_model_iter_parent (GTK_TREE_MODEL (mStore), &itr,
                                          &mCurrentIter);
          if (ret)
            {
              mCurrentIter = itr;
            }
        }

      gtk_tree_selection_select_iter (mSelection, &mCurrentIter);

      GtkTreePath *path =
          gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
      if (path)
        {
          gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, nullptr,
                                        TRUE, 0.5, 0.0);
          gtk_tree_view_collapse_row (GTK_TREE_VIEW (mDirView), path);
          gtk_tree_path_free (path);
        }

      mStatus->show (false);
    }

    void ApvlvDir::scrollright (int times)
    {
      if (!mReady)
        return;

      GtkTreeIter itr;
      for (gboolean ret = TRUE; times > 0 && ret; times--)
        {
          ret =
              gtk_tree_model_iter_children (GTK_TREE_MODEL (mStore), &itr,
                                            &mCurrentIter);
          if (ret)
            {
              mCurrentIter = itr;
            }
        }

      GtkTreePath *path =
          gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
      if (path)
        {
          gtk_tree_view_expand_to_path (GTK_TREE_VIEW (mDirView), path);
          gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
          gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, nullptr,
                                        TRUE, 0.5, 0.0);
          gtk_tree_path_free (path);
        }

      mStatus->show (false);
    }

    bool ApvlvDir::search (const char *str, bool reverse)
    {
      bool next;

      if (!mReady)
        return false;

      if (*str == '\0' && mSearchStr.empty ())
        {
          return false;
        }

      next = true;
      if (*str != '\0')
        {
          mSearchStr = str;
          next = false;
        }

      ApvlvDirNode *info = nullptr;
      gtk_tree_model_get (GTK_TREE_MODEL (mStore), &mCurrentIter, 0, &info, -1);
      if (info == nullptr || mDirNodes == nullptr)
        {
          mView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
          return false;
        }

      GList *list;
      for (list = mDirNodes; list; list = g_list_next (list))
        {
          if (info == list->data)
            {
              break;
            }
        }

      if (list == nullptr)
        {
          mView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
          return false;
        }

      if (next)
        {
          if (reverse)
            {
              list = g_list_previous (list);
            }
          else
            {
              list = g_list_next (list);
            }
        }

      bool wrap = gParams->valueb ("wrapscan");

      for (GList *origin = list; list != nullptr;)
        {
          info = (ApvlvDirNode *) list->data;
          if (strstr (info->phrase (), mSearchStr.c_str ()) != nullptr)
            {
              break;
            }

          if (reverse)
            {
              list = g_list_previous (list);
            }
          else
            {
              list = g_list_next (list);
            }

          if (list == origin)
            {
              list = nullptr;
              break;
            }

          if (list == nullptr && wrap)
            {
              if (reverse)
                {
                  list = g_list_last (mDirNodes);
                }
              else
                {
                  list = mDirNodes;
                }
            }
        }

      if (list == nullptr)
        {
          mView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
          return false;
        }

      mCurrentIter = *info->iter ();
      GtkTreePath *path =
          gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
      if (path)
        {
          gtk_tree_view_expand_to_path (GTK_TREE_VIEW (mDirView), path);
          gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
          gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, nullptr,
                                        TRUE, 0.5, 0.0);
          gtk_tree_path_free (path);
        }

      mStatus->show (false);
      return true;
    }

    void ApvlvDir::setactive (bool act)
    {
      mStatus->active (act);
      mActive = act;
    }

    bool ApvlvDir::walk_file_index (GtkTreeIter *titr, ApvlvFileIndexIter iter)
    {
      bool has;

      has = true;
      auto *node = new ApvlvDirNode (this, titr, iter->page, iter->title.c_str ());
      mDirNodes = g_list_append (mDirNodes, node);

      for (auto itr = iter->children.begin ();
           itr != iter->children.end (); ++itr)
        {
          has = walk_file_index (has ? node->iter () : titr, itr);
        }

      return has;
    }

    void
    ApvlvDir::apvlv_dir_on_changed (GtkTreeSelection *selection,
                                    ApvlvDir *dir)
    {
      GtkTreeModel *model;
      gtk_tree_selection_get_selected (selection, &model, &dir->mCurrentIter);
    }

    ApvlvDirStatus::ApvlvDirStatus (__attribute__((unused)) ApvlvDir *doc)
    {
      for (auto &i : mStlab)
        {
          i = gtk_label_new ("");
          gtk_box_pack_start (GTK_BOX (mHbox), i, TRUE, TRUE, 0);
        }
    }

    ApvlvDirStatus::~ApvlvDirStatus ()
    = default;

    void ApvlvDirStatus::active (bool act)
    {
      for (auto &i : mStlab)
        {
#if GTK_CHECK_VERSION(3, 0, 0)
          gtk_widget_set_state_flags (i,
                                      (act) ? GTK_STATE_FLAG_ACTIVE :
                                      GTK_STATE_FLAG_INSENSITIVE, TRUE);
#else
          gtk_widget_modify_fg (mStlab[i],
                                (act) ? GTK_STATE_ACTIVE:
                                GTK_STATE_INSENSITIVE, nullptr);
#endif
        }
    }

    gboolean ApvlvDir::apvlv_dir_first_select_cb (ApvlvDir *dir)
    {
      GtkTreeIter gtir;
      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dir->mStore), &gtir))
        {
          gtk_tree_selection_select_iter (dir->mSelection, &gtir);
        }
      return FALSE;
    }

    void ApvlvDir::apvlv_dir_change_node (ApvlvDirNode *node, GFile *gf1, GFileMonitorEvent ev)
    {
      if (ev != G_FILE_MONITOR_EVENT_DELETED
          && ev != G_FILE_MONITOR_EVENT_CREATED)
        {
          return;
        }

      gchar *name = g_file_get_path (gf1);
      if (name == nullptr)
        {
          debug ("Can't get path name.\n");
          return;
        }

      gchar *basename = g_file_get_basename (gf1);
      if (basename == nullptr)
        {
          basename = g_path_get_basename (name);
        }
      if (basename == nullptr)
        {
          basename = g_strdup (name);
        }

      if (g_ascii_strncasecmp (basename + strlen (basename) - 4, ".pdf", 4)
          != 0
          && g_ascii_strncasecmp (basename + strlen (basename) - 4,
                                  ".htm", 4) != 0
          && g_ascii_strncasecmp (basename + strlen (basename) - 5,
                                  ".html", 5) != 0
          && g_ascii_strncasecmp (basename + strlen (basename) - 5,
                                  ".epub", 5) != 0
#ifdef APVLV_WITH_DJVU
        && g_ascii_strncasecmp (basename + strlen (basename) - 5,
                    ".djvu", 5) != 0
        && g_ascii_strncasecmp (basename + strlen (basename) - 4, ".djv",
                    4) != 0
#endif
#ifdef APVLV_WITH_TXT
        && g_ascii_strncasecmp (basename + strlen (basename) - 4,
                    ".txt", 4) != 0
#endif
          )
        {
          g_free (name);
          g_free (basename);
          return;
        }

      if (ev == G_FILE_MONITOR_EVENT_DELETED)
        {
          debug ("delete file: %s", name);

          GList *listnode;
          ApvlvDirNode *nnode = nullptr;
          for (listnode = g_list_first (mDirNodes);
               listnode != nullptr;
               listnode = g_list_next (listnode))
            {
              nnode = (ApvlvDirNode *) listnode->data;
              if (strcmp (nnode->phrase (), basename) == 0)
                {
                  break;
                }
            }

          if (nnode != nullptr)
            {
              gtk_tree_store_remove (mStore, nnode->iter ());
              mDirNodes = g_list_remove (mDirNodes, nnode);
              delete nnode;
            }
        }

      else
        {
          debug ("add file: %s", name);

          auto *nnode = new ApvlvDirNode (this, node ? node->iter () : nullptr, false, name, basename);
          mDirNodes = g_list_append (mDirNodes, nnode);
        }

      g_free (basename);
      g_free (name);
    }

    void
    ApvlvDir::apvlv_dir_monitor_callback (__attribute__((unused)) GFileMonitor *gfm, GFile *gf1, __attribute__((unused)) GFile *gf2, GFileMonitorEvent ev, ApvlvDir *dir)
    {
      if (dir->mType == CORE_CONTENT
          && ev == G_FILE_MONITOR_EVENT_CHANGED)
        {
          dir->mView->errormessage ("Contents is modified, apvlv reload it automatically");
          dir->reload ();
        }
      else if (dir->mType == CORE_DIR
               && (ev == G_FILE_MONITOR_EVENT_DELETED
                   || ev == G_FILE_MONITOR_EVENT_CREATED)
          )
        {
          dir->apvlv_dir_change_node (nullptr, gf1, ev);
        }
    }

    bool ApvlvDir::walk_dir_path_index (GtkTreeIter *itr, const char *path)
    {
      bool has = false;
      GDir *dir = g_dir_open (path, 0, nullptr);
      if (dir != nullptr)
        {
          const gchar *name;
          while ((name = g_dir_read_name (dir)) != nullptr)
            {
              if (strcmp (name, ".") == 0)
                {
                  debug ("avoid hidden file: %s", name);
                  continue;
                }

              gchar *realname = g_strjoin (PATH_SEP_S, path, name, nullptr);
              //          debug ("add a item: %s[%s]", name, realname);

              ApvlvDirNode *node;
              struct stat buf[1];
              char *wrealname =
                  g_locale_from_utf8 (realname, -1, nullptr, nullptr, nullptr);
              if (wrealname == nullptr)
                {
                  g_free (realname);
                  continue;
                }

              int ret = stat (wrealname, buf);
              g_free (wrealname);

              if (ret < 0)
                {
                  g_free (realname);
                  continue;
                }

              if (S_ISDIR (buf->st_mode))
                {
                  node = new ApvlvDirNode (this, itr, true, realname, name);

                  if (!walk_dir_path_index (node->iter (), realname))
                    {
                      gtk_tree_store_remove (mStore, node->iter ());
                      delete node;
                      node = nullptr;
                    }

                  if (node != nullptr)
                    {
                      mDirNodes = g_list_append (mDirNodes, node);
                      has = true;
                    }
                }
              else if (g_ascii_strncasecmp (name + strlen (name) - 4, ".pdf", 4)
                       == 0
                       || g_ascii_strncasecmp (name + strlen (name) - 4,
                                               ".htm", 4) == 0
                       || g_ascii_strncasecmp (name + strlen (name) - 5,
                                               ".html", 5) == 0
                       || g_ascii_strncasecmp (name + strlen (name) - 5,
                                               ".epub", 5) == 0
#ifdef APVLV_WITH_DJVU
                || g_ascii_strncasecmp (name + strlen (name) - 5,
                            ".djvu", 5) == 0
                || g_ascii_strncasecmp (name + strlen (name) - 4, ".djv",
                            4) == 0
#endif
#ifdef APVLV_WITH_TXT
                || g_ascii_strncasecmp (name + strlen (name) - 4,
                            ".txt", 4) == 0
#endif
                  )
                {
                  node = new ApvlvDirNode (this, itr, false, realname, name);
                  mDirNodes = g_list_append (mDirNodes, node);

                  has = true;
                }

              g_free (realname);
            }
        }
      g_dir_close (dir);

      return has;
    }
}

// Local Variables:
// mode: c++
// End:
