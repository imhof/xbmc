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

#include "VDRDirectory.h"

#include "FactoryDirectory.h"

using namespace XFILE;

CVDRDirectory::CVDRDirectory()
    : IDirectory()
    , m_proxy(CFactoryDirectory::Create("smb://"))
{
}

CVDRDirectory::~CVDRDirectory()
{
}

bool CVDRDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
    ASSERT(m_proxy);
    return m_proxy->GetDirectory(strPath, items);
}

bool CVDRDirectory::Exists(const char* strPath)
{
    ASSERT(m_proxy);
    return m_proxy->Exists(strPath);
}

bool CVDRDirectory::IsAllowed(const CStdString &strFile) const
{
    ASSERT(m_proxy);
    return m_proxy->IsAllowed(strFile);
}

DIR_CACHE_TYPE CVDRDirectory::GetCacheType(const CStdString& strPath) const
{
    ASSERT(m_proxy);
    return m_proxy->GetCacheType(strPath);
}
