#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IFile.h"

#include <vector>
#include <memory>

class IDirectory;

namespace XFILE
{
  class CFileVDR: public IFile
  {
  public:
    CFileVDR();
    virtual ~CFileVDR();
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);

    virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();
    virtual int64_t GetPosition();
    virtual int64_t GetLength();

    virtual int IoControl(EIoControl request, void* param);

  private:

	CURL SwitchURL(const CURL &original) const;
    int FindFileAtPosition(int64_t pos) const;

	struct SubFileInfo {
		boost::shared_ptr<IFile> file;
		int64_t pos;
	};
	std::vector<SubFileInfo> m_tsFiles;

	unsigned int m_file;
	std::vector<int64_t> m_marks;
  };
}
