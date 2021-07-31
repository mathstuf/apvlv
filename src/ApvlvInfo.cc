/*
 * This file is part of the apvlv package
 * Copyright (C) <2008>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE ApvlvInfo.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2010/02/23 15:00:42 Alf*/

#include "ApvlvInfo.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace apvlv
{
ApvlvInfo *gInfo = nullptr;

ApvlvInfo::ApvlvInfo (const char *filename)
{
  mFileName = filename;

  mFileHead = nullptr;
  mFileMax = 10;

  ifstream is (mFileName.c_str (), ios::in);
  if (is.is_open ())
    {
      string line;
      const char *p;

      while (getline (is, line))
        {
          p = line.c_str ();

          if (*p != '\''              /* the ' */
              || !isdigit (*(p + 1))) /* the digit */
            {
              continue;
            }

          ini_add_position (p);
        }

      mFileHead = g_slist_reverse (mFileHead);

      is.close ();
    }
}

ApvlvInfo::~ApvlvInfo ()
{
  for (GSList *list = mFileHead; list != nullptr; list = g_slist_next (list))
    {
      auto *fp = (infofile *)(list->data);
      delete fp;
    }
  g_slist_free (mFileHead);
}

bool
ApvlvInfo::update ()
{
  ofstream os (mFileName.c_str (), ios::out);
  if (!os.is_open ())
    {
      return false;
    }

  int i;
  GSList *lfp;
  infofile *fp;
  for (i = 0, lfp = mFileHead; i < mFileMax && lfp != nullptr;
       ++i, lfp = g_slist_next (lfp))
    {
      fp = (infofile *)(lfp->data);
      if (fp)
        {
          os << "'" << i << "\t";
          os << fp->page << ':' << fp->skip << "\t";
          os << fp->rate << "\t";
          os << fp->file << endl;
        }
    }

  os.close ();
  return true;
}

infofile *
ApvlvInfo::file (int id)
{
  auto *fp = (infofile *)g_slist_nth_data (mFileHead, id);
  return fp;
}

infofile *
ApvlvInfo::file (const char *filename)
{
  GSList *lfp;
  infofile *fp;

  for (lfp = mFileHead; lfp != nullptr; lfp = g_slist_next (lfp))
    {
      fp = (infofile *)(lfp->data);
      if (fp->file == filename)
        {
          break;
        }
    }

  if (lfp == nullptr)
    {
      fp = new infofile;
      fp->page = 0;
      fp->skip = 0;
      fp->rate = 0.0;
      fp->file = filename;
      mFileHead = g_slist_insert_before (mFileHead, mFileHead, fp);
    }
  else
    {
      mFileHead = g_slist_remove (mFileHead, fp);
      mFileHead = g_slist_insert_before (mFileHead, mFileHead, fp);
    }

  return fp;
}

bool
ApvlvInfo::file (int page, double rate, const char *filename, int skip)
{
  infofile *fp;

  fp = file (filename);

  fp->page = page;
  fp->rate = rate;
  fp->skip = skip;
  update ();

  return true;
}

bool
ApvlvInfo::ini_add_position (const char *str)
{
  const char *p, *s;

  p = strchr (str + 2, '\t'); /* Skip the ' and the digit */
  if (p == nullptr)
    {
      return false;
    }

  while (*p != '\0' && !isdigit (*p))
    {
      p++;
    }
  int page = int (strtol (p, nullptr, 10));
  int skip;

  s = strchr (p, ':');
  for (; s && p < s; ++p)
    {
      if (!isdigit (*p))
        {
          break;
        }
    }
  if (p == s)
    {
      ++p;
      skip = int (strtol (p, nullptr, 10));
    }
  else
    {
      skip = 0;
    }

  p = strchr (p, '\t');
  if (p == nullptr)
    {
      return false;
    }

  while (*p != '\0' && !isdigit (*p))
    {
      p++;
    }
  double rate = strtod (p, nullptr);

  p = strchr (p, '\t');
  if (p == nullptr)
    {
      return false;
    }

  while (*p != '\0' && isspace (*p))
    {
      p++;
    }
  if (*p == '\0')
    {
      return false;
    }

  auto *fp = new infofile;
  fp->page = page;
  fp->rate = rate;
  fp->skip = skip;
  fp->file = p;
  mFileHead = g_slist_insert_before (mFileHead, mFileHead, fp);
  return true;
}
};

// Local Variables:
// mode: c++
// End:
