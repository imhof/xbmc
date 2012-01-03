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
#include "FileItem.h"

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
    if (!m_proxy->GetDirectory(strPath, items)) {
        return false;
    }

    // update information so that .rec directories are logically handled as single files
    for (int i = items.GetObjectCount()-1; i >= 0; --i) {
        CFileItemPtr current = items[i];

        // change protocol
        CStdString path = current->GetPath();
        path = "vdr:" + path.Mid(4);

        // check if a .rec folder is hiding one level below, if yes -> turn this folder into a "file"
        if (current->m_bIsFolder) {
            CFileItemList sub_items;
            m_proxy->GetDirectory(current->GetPath(), sub_items);

            for (int j = 0; j < sub_items.GetObjectCount(); ++j) {
                CFileItemPtr current_sub = sub_items[j];
                if (current_sub->GetPath().Right(4).ToUpper() == ".REC" || current_sub->GetPath().Right(5).ToUpper() == ".REC/") {
                    // cutoff trailing slash
                    path = path.Left(path.size()-1);
                    current->m_bIsFolder = false;

                    // basic title fixes, FIXME: fix mangled SMB paths
                    CStdString label = current->GetLabel();
                    label.Replace('_',' ');
                    label.Replace('&','/');
                    current->SetLabel(label);

                    break;
                }
            }
        } else {
            // normal files are always invisible in vdr://
            items.Remove(i);
        }

        current->SetPath(path);
    }

    return true;
}

bool CVDRDirectory::Exists(const char* strPath)
{
    ASSERT(m_proxy);
    return m_proxy->Exists(strPath);
}

bool CVDRDirectory::IsAllowed(const CStdString &strFile) const
{
    ASSERT(m_proxy);
    return  m_proxy->IsAllowed(strFile);
}

DIR_CACHE_TYPE CVDRDirectory::GetCacheType(const CStdString& strPath) const
{
    ASSERT(m_proxy);
    return m_proxy->GetCacheType(strPath);
}
