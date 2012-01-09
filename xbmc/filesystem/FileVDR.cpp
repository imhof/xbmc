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

#include "FileVDR.h"

#include "IDirectory.h"
#include "FileFactory.h"
#include "FactoryDirectory.h"
#include "FileItem.h"

using namespace XFILE;
using namespace std;

CFileVDR::CFileVDR()
	: IFile()
	, m_file(-1)
{
}

CFileVDR::~CFileVDR()
{
	Close();
}

bool CFileVDR::Exists(const CURL& url)
{
	CURL proxy_url = SwitchURL(url);
	IFile* smbDummy = CFileFactory::CreateLoader(proxy_url);
	ASSERT(smbDummy);
	return smbDummy->Exists(proxy_url);
}

int64_t CFileVDR::Seek(int64_t iFilePosition, int iWhence)
{
	int64_t new_position = 0;

	// make position absolute
	switch( iWhence ) {
	case SEEK_SET:
		new_position = iFilePosition;
		break;
	case SEEK_CUR:
		new_position = GetPosition() + iFilePosition;
		break;
	case SEEK_END:
		new_position = GetLength() - iFilePosition;
		break;
	}

    // find sub file at position
    int file_no = FindFileAtPosition(new_position);
    if (file_no < 0) {
        return -1;
    }

    m_file = file_no;

    // seek and update internal position
    m_tsFiles[m_file].file->Seek(new_position - m_tsFiles[m_file].pos, SEEK_SET);
    return GetPosition();
}

int CFileVDR::IoControl(EIoControl request, void* param)
{
	if (request == IOCTRL_SEEK_POSSIBLE) {
		return 1;
	}
	return -1;
}


int CFileVDR::Stat(const CURL& url, struct __stat64* buffer)
{
	// is this ok? FIXME
	CURL proxy_url = SwitchURL(url);
	IFile* smbDummy = CFileFactory::CreateLoader(proxy_url);
	ASSERT(smbDummy);
	return smbDummy->Stat(proxy_url, buffer);
}

int64_t CFileVDR::GetPosition()
{
    if(m_file < 0) {
        return -1;
    }

    ASSERT(m_file < m_tsFiles.size());
    return m_tsFiles[m_file].pos + m_tsFiles[m_file].file->GetPosition();
}

int64_t CFileVDR::GetLength()
{
	if (m_tsFiles.size() == 0) return 0;
    return  m_tsFiles.back().pos + m_tsFiles.back().file->GetLength();
}

bool CFileVDR::Open(const CURL &url)
{
	CURL smb_url = SwitchURL(url);
    boost::shared_ptr<IDirectory> base_dir(CFactoryDirectory::Create( smb_url.Get() ));
	if (!base_dir) return false;

    // find .rec subdirectory
	CFileItemList items;
	base_dir->GetDirectory( smb_url.Get(), items );

    CStdString rec_path;
    for (int i = 0; i < items.Size(); ++i) {
        if (items[i]->GetPath().Right(5).ToUpper() == ".REC/") {
            rec_path = items[i]->GetPath();
            break;
        }
    }

    if (rec_path.empty()) {
        return false;
    }

    items.Clear();
    base_dir->GetDirectory( rec_path, items );

    int64_t pos_sum = 0;

	// gather TS files first
	for (int i = 0; i < items.Size(); ++i) {
		CStdString path = items[i]->GetPath();
		if (path.Right(3).ToUpper() == ".TS" || (path.Right(6)[0] == '0' && path.Right(4).ToUpper() == ".VDR")) {
			SubFileInfo info;
			info.file.reset(CFileFactory::CreateLoader("smb://dummy"));
			if (info.file.get()) {
				info.file->Open(items[i]->GetAsUrl());
				info.pos = pos_sum;
                pos_sum += info.file->GetLength();
				m_tsFiles.push_back(info);
			} else {
				// FIXME: log something?!
			}
		}
	}

	if (m_tsFiles.size() == 0) {
		// no TS files found, probably old .vdr recording. This is not supported at the moment
		return false;
	}

	m_file = 0;
	return true;
}

unsigned int CFileVDR::Read(void* lpBuf, int64_t uiBufSize)
{
	if (m_tsFiles.empty()) return -1;

    unsigned int bytes_read = m_tsFiles[m_file].file->Read(lpBuf, uiBufSize);

	if (bytes_read < uiBufSize) {
        if (m_tsFiles[m_file].file->GetPosition() == m_tsFiles[m_file].file->GetLength()) {
            // file exhausted, do we have another?
            if (m_file < m_tsFiles.size()-1) {
                // skip to it, next Read() will take the new subfile
                ++m_file;
            }
        }
	}

	return bytes_read;
}

void CFileVDR::Close()
{
	for (size_t i = 0; i < m_tsFiles.size(); ++i) {
		ASSERT(m_tsFiles[i].file.get());
		m_tsFiles[i].file->Close();
	}

	m_tsFiles.clear();
}

CURL CFileVDR::SwitchURL(const CURL &original) const
{
	CURL proxy_url(original);
	proxy_url.SetProtocol("smb");
	return proxy_url;
}

int CFileVDR::FindFileAtPosition(int64_t pos) const
{
	// find file that contains the position
	for (unsigned int i = 0; i < m_tsFiles.size(); ++i) {
        if (m_tsFiles[i].pos <= pos && (m_tsFiles[i].file->GetLength() + m_tsFiles[i].pos) > pos) {
			return i;
		}
	}

    return -1;
}

bool CFileVDR::GetCutList(const CStdString &strPath, float fps, std::vector<int64_t> & cut_list_in_ms)
{
    int mspf = int(1000.0 / fps);

    // find REC dir
	CURL smb_url = SwitchURL(strPath);
    boost::shared_ptr<IDirectory> base_dir(CFactoryDirectory::Create( smb_url.Get() ));
	if (!base_dir) return false;

	CFileItemList items;
	base_dir->GetDirectory( smb_url.Get(), items );

    CStdString rec_path;
    for (int i = 0; i < items.Size(); ++i) {
        if (items[i]->GetPath().Right(5).ToUpper() == ".REC/") {
            rec_path = items[i]->GetPath();
            break;
        }
    }

    if (rec_path.empty()) {
        return false;
    }

	// read in cut marks, compute millisecond values
	boost::shared_ptr<IFile> marks( CFileFactory::CreateLoader(rec_path + "/marks") );

	if (!marks || (!marks->Open(rec_path + "/marks") && !marks->Open(rec_path + "/marks.vdr")) ) {
        return false;
    }

	char line[256] = {0};
	while( marks->ReadString(line, 255) ) {
		int h,m,s,f;
		sscanf(line,"%1d:%2d:%2d.%2d", &h, &m, &s, &f);
		int64_t milli_secs = (int64_t)h * (60*60*1000) + m * (60*1000) + s*1000 + f*mspf;

        bool is_start = (strstr(line,"lost") == 0);

        if (cut_list_in_ms.empty() && is_start) {
            // first logo start, cut out beginning to logo
            cut_list_in_ms.push_back(0);
        }

		cut_list_in_ms.push_back( milli_secs );
	}

	marks->Close();

    // improve cut marks with a simple heuristic

    // a break somewhere in the first ten minutes -> also suppress beginning as
    // we assume start of film after the first break
    if (cut_list_in_ms.size() > 3) {
        if (cut_list_in_ms[3] < 10*60*1000) {
            cut_list_in_ms.erase(cut_list_in_ms.begin()+1, cut_list_in_ms.begin()+3);
        }
    }

    return true;
}
