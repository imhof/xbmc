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
#include "FileFactory.h"

using namespace XFILE;
using namespace std;

CFileVDR::CFileVDR()
	: IFile()
	, m_proxy(CFileFactory::CreateLoader("smb://dummy"))
{
}

CFileVDR::~CFileVDR()
{
	Close();

	delete m_proxy;
}

bool CFileVDR::Exists(const CURL& url)
{
	ASSERT(m_proxy);
	return m_proxy->Exists(url);
}

int64_t CFileVDR::Seek(int64_t iFilePosition, int iWhence)
{
	ASSERT(m_proxy);
	return m_proxy->Seek(iFilePosition, iWhence);
}

int CFileVDR::Stat(const CURL& url, struct __stat64* buffer)
{
	ASSERT(m_proxy);
	return m_proxy->Stat(url, buffer);
}

int64_t CFileVDR::GetPosition()
{
	ASSERT(m_proxy);
	return m_proxy->GetPosition();
}

int64_t CFileVDR::GetLength()
{
	ASSERT(m_proxy);
	return m_proxy->GetLength();
}

bool CFileVDR::Open(const CURL &url)
{
	ASSERT(m_proxy);

	CURL proxy_url(url);
	proxy_url.SetProtocol("smb");

	return m_proxy->Open(proxy_url);
}

unsigned int CFileVDR::Read(void* lpBuf, int64_t uiBufSize)
{
	ASSERT(m_proxy);
	return m_proxy->Read(lpBuf, uiBufSize);
}

void CFileVDR::Close()
{
	ASSERT(m_proxy);
	m_proxy->Close();
}
