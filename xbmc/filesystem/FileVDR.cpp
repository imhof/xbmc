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
	, m_pos(0)
	, m_virtualStart(0)
	, m_file(0)
{
}

CFileVDR::~CFileVDR()
{
	Close();
}

bool CFileVDR::Exists(const CURL& url)
{
	CURL proxy_url = switchURL(url);
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
		new_position = iFilePosition + m_virtualStart;
		break;
	case SEEK_CUR:
		new_position = m_pos + iFilePosition;
		break;
	case SEEK_END:
		new_position = (GetLength() + m_virtualStart) - iFilePosition;
		break;
	}

	// find file that contains the position
	for (unsigned int i = 0; i < m_tsFiles.size(); ++i) {
		if (m_tsFiles[i].pos <= new_position && (m_tsFiles[i].size + m_tsFiles[i].pos) > new_position) {
			// update position
			m_pos = new_position;
			m_file = i;
			m_tsFiles[i].file->Seek(new_position - m_tsFiles[i].pos, SEEK_SET);
			return virtualPos();
		}
	}

	return -1;
}

int CFileVDR::Stat(const CURL& url, struct __stat64* buffer)
{
	// is this ok? FIXME
	CURL proxy_url = switchURL(url);
	IFile* smbDummy = CFileFactory::CreateLoader(proxy_url);
	ASSERT(smbDummy);
	return smbDummy->Stat(proxy_url, buffer);
}

int64_t CFileVDR::GetPosition()
{
	return m_pos - m_virtualStart;
}

int64_t CFileVDR::GetLength()
{
	if (m_tsFiles.size() == 0) return 0;
	return  m_tsFiles.back().pos + m_tsFiles.back().size - m_virtualStart;
}

bool CFileVDR::Open(const CURL &url)
{
	CURL smb_url = switchURL(url);
	IDirectory* base_dir = CFactoryDirectory::Create( smb_url.Get() );
	if (!base_dir) return false;

	CFileItemList items;
	base_dir->GetDirectory( smb_url.Get(), items );

	int64_t pos_sum = 0;
	double first_mark = 0.0;
	double last_mark = 0.0;

	for (int i = 0; i < items.Size(); ++i) {
		CStdString path = items[i]->GetPath();
		if (path.Right(3).ToUpper() == ".TS") {
			SubFileInfo info;
			info.file.reset(CFileFactory::CreateLoader("smb://dummy"));
			if (info.file.get()) {
				info.file->Open(items[i]->GetAsUrl());
				info.pos = pos_sum;
				info.size = info.file->GetLength();
				pos_sum += info.size;
				m_tsFiles.push_back(info);
			} else {
				// FIXME: log something?!
			}
		} else if (path.Right(5).ToUpper() == "MARKS") {
			// read cut marks if available
			IFile* marks = CFileFactory::CreateLoader(path);
			marks->Open(path);			
			char line[256] = {0};
			if( marks->ReadString(line, 255) ) {
				int h,m,s,ms;
				sscanf(line,"%1d:%2d:%2d.%2d", &h, &m, &s, &ms);
				first_mark = h * (60*60*100) + m * (60 * 100) + s * 100 + ms;
			}

			while( marks->ReadString(line, 255) ) {
				int h,m,s,ms;
				sscanf(line,"%1d:%2d:%2d.%2d", &h, &m, &s, &ms);
				last_mark = h * (60*60*100) + m * (60 * 100) + s * 100 + ms;
			}

			delete marks;
		}
	}

	m_virtualStart = 0;

	// if available init position with first cut mark
	if (first_mark > 0.0) {
		double rel_pos = first_mark / last_mark;
		m_virtualStart = static_cast<int64_t>(pos_sum * rel_pos);
	}

	m_pos = m_virtualStart;
	m_file = 0;
	return true;
}

unsigned int CFileVDR::Read(void* lpBuf, int64_t uiBufSize)
{
	unsigned int bytes_read = 0;

	if (m_tsFiles.empty()) return -1;

	int64_t to_read_in_file =  m_tsFiles[m_file].size - (m_pos - m_tsFiles[m_file].pos);

	if (to_read_in_file > uiBufSize) {
		bytes_read = m_tsFiles[m_file].file->Read(lpBuf, uiBufSize);
		m_pos += bytes_read;
	} else {
		bytes_read = m_tsFiles[m_file].file->Read(lpBuf, to_read_in_file);
		m_pos += bytes_read;
		
		if (m_pos == m_tsFiles[m_file].pos +  m_tsFiles[m_file].size && m_file < m_tsFiles.size()-1) {
			++m_file;
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

CURL CFileVDR::switchURL(const CURL &original) const
{
	CURL proxy_url(original);
	proxy_url.SetProtocol("smb");
	return proxy_url;
}

int64_t CFileVDR::virtualPos() const
{
	return m_pos - m_virtualStart;
}
