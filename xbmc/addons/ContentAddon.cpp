/*
 *      Copyright (C) 2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//TODO implement todo's

#include "ContentAddon.h"
#include "ContentAddons.h"
#include "music/Song.h"
#include "music/Artist.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "MediaSource.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "interfaces/AnnouncementManager.h"
#include "ApplicationMessenger.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"

using namespace ADDON;
using namespace ANNOUNCEMENT;
using namespace XFILE::MUSICDATABASEDIRECTORY;
using namespace std;

CContentAddon::CContentAddon(const AddonProps& props) :
    CAddonDll<DllContentAddon, ContentAddon, CONTENT_PROPERTIES>(props),
    m_apiVersion("0.0.0"),
    m_bReadyToUse(false),
    m_playState(CONTENT_ADDON_PLAYSTATE_STOP),
    m_bProvidesMusicCodec(false),
    m_bProvidesMusicFiles(false),
    m_bSupportsConcurrentStreams(false),
    m_bProvidesFiles(false)
{
  m_strUserPath          = CSpecialProtocol::TranslatePath(Profile()).c_str();
  m_strClientPath        = CSpecialProtocol::TranslatePath(Path()).c_str();
  m_pInfo                = new CONTENT_PROPERTIES;
  m_pInfo->strUserPath   = m_strUserPath.c_str();
  m_pInfo->strClientPath = m_strClientPath.c_str();
}

CContentAddon::CContentAddon(const cp_extension_t *ext) :
    CAddonDll<DllContentAddon, ContentAddon, CONTENT_PROPERTIES>(ext),
    m_apiVersion("0.0.0"),
    m_bReadyToUse(false),
    m_playState(CONTENT_ADDON_PLAYSTATE_STOP)
{
  CStdString strProvidesMusicCodec = CAddonMgr::Get().GetExtValue(ext->configuration, "@provides_music_codec");
  CStdString strProvidesMusicFiles = CAddonMgr::Get().GetExtValue(ext->configuration, "@provides_music_files");
  CStdString strProvidesFiles = CAddonMgr::Get().GetExtValue(ext->configuration, "@provides_files");
  CStdString strSupportsConcurrentStreams = CAddonMgr::Get().GetExtValue(ext->configuration, "@supports_concurrent_streams");

  m_fileTypes                  = StringUtils::Split(CAddonMgr::Get().GetExtValue(ext->configuration, "@filetypes"), "|");
  m_bProvidesMusicCodec        = strProvidesMusicCodec.ToLower().Equals("true") || strProvidesMusicCodec.Equals("1");
  m_bProvidesMusicFiles        = strProvidesMusicFiles.ToLower().Equals("true") || strProvidesMusicFiles.Equals("1");
  m_bProvidesFiles             = strProvidesFiles.ToLower().Equals("true") || strProvidesFiles.Equals("1");
  m_bSupportsConcurrentStreams = strSupportsConcurrentStreams.ToLower().Equals("true") || strSupportsConcurrentStreams.Equals("1");
  m_strUserPath                = CSpecialProtocol::TranslatePath(Profile()).c_str();
  m_strClientPath              = CSpecialProtocol::TranslatePath(Path()).c_str();
  m_pInfo                      = new CONTENT_PROPERTIES;
  m_pInfo->strUserPath         = m_strUserPath.c_str();
  m_pInfo->strClientPath       = m_strClientPath.c_str();

  //TODO hack
  if (!m_bProvidesMusicFiles)
    MarkAsDisabled();
}

CContentAddon::~CContentAddon(void)
{
  Destroy();
  SAFE_DELETE(m_pInfo);
}

void CContentAddon::ResetProperties(void)
{
  CSingleLock lock(m_critSection);
  m_apiVersion  = AddonVersion("0.0.0");
  m_bReadyToUse = false;
}

ADDON_STATUS CContentAddon::Create(void)
{
  ADDON_STATUS status(ADDON_STATUS_UNKNOWN);

  /* ensure that a previous instance is destroyed */
  Destroy();

  bool bReadyToUse(false);
  CLog::Log(LOGDEBUG, "%s - creating content add-on instance '%s'", __FUNCTION__, Name().c_str());
  try
  {
    status = CAddonDll<DllContentAddon, ContentAddon, CONTENT_PROPERTIES>::Create();
    bReadyToUse = (status == ADDON_STATUS_OK);
  }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  {
    CSingleLock lock(m_critSection);
    m_bReadyToUse = bReadyToUse;
  }

  if (bReadyToUse)
    CAnnouncementManager::AddAnnouncer(this);

  return status;
}

bool CContentAddon::DllLoaded(void) const
{
  try { return CAddonDll<DllContentAddon, ContentAddon, CONTENT_PROPERTIES>::DllLoaded(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  return false;
}

void CContentAddon::Destroy(void)
{
  CAnnouncementManager::RemoveAnnouncer(this);

  MusicCloseFile();

  /* reset 'ready to use' to false */
  {
    CSingleLock lock(m_critSection);
    if (!m_bReadyToUse)
      return;
    m_bReadyToUse = false;
  }

  CLog::Log(LOGDEBUG, "%s - destroying add-on '%s'", __FUNCTION__, Name().c_str());

  /* destroy the add-on */
  try { CAddonDll<DllContentAddon, ContentAddon, CONTENT_PROPERTIES>::Destroy(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  /* reset all properties to defaults */
  ResetProperties();
}

void CContentAddon::ReCreate(void)
{
  Destroy();
  Create();
}

bool CContentAddon::ReadyToUse(void) const
{
  CSingleLock lock(m_critSection);
  return m_bReadyToUse;
}

const char* CContentAddon::GetServerName(void)
{
  try { return m_pStruct->GetServerName(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }
  return NULL;
}

void CContentAddon::FreeFileList(CONTENT_ADDON_FILELIST* items)
{
  try { return m_pStruct->FreeFileList(items); }
  catch (exception &e) { LogException(e, __FUNCTION__); }
}

map<string, CONTENT_ADDON_FILE_PROPERTY> FileListToMap(CONTENT_ADDON_FILE_PROPERTY* properties, int len)
{
  map<string, CONTENT_ADDON_FILE_PROPERTY> retval;
  for (int i = 0; i < len; i++)
    retval.insert(make_pair(properties[i].key, properties[i]));
  return retval;
}

CStdString CContentAddon::GetPropertyString(map<string, CONTENT_ADDON_FILE_PROPERTY>& m, const CStdString& strKey, const CStdString& strDefault /* = "" */)
{
  CStdString strReturn(strDefault);
  map<string, CONTENT_ADDON_FILE_PROPERTY>::iterator it = m.find(strKey);
  if (it != m.end() && it->second.type == CONTENT_ADDON_PROPERTY_TYPE_STRING)
  {
    strReturn = it->second.strValue;
    m.erase(it);
  }
  return strReturn;
}

int CContentAddon::GetPropertyInt(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY>& m, const CStdString& strKey, int iDefault /* = 0 */)
{
  int iReturn(iDefault);
  map<string, CONTENT_ADDON_FILE_PROPERTY>::iterator it = m.find(strKey);
  if (it != m.end() && it->second.type == CONTENT_ADDON_PROPERTY_TYPE_INT)
  {
    iReturn = it->second.iValue;
    m.erase(it);
  }
  return iReturn;
}

void CContentAddon::AddCommonProperties(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY>& item, CFileItemPtr& fileItem)
{
  CStdString strThumb = ContentBuildPath(GetPropertyString(item, "thumb"));
  if (!strThumb.empty())
    fileItem->SetArt("thumb", strThumb);
  CStdString strArt   = ContentBuildPath(GetPropertyString(item, "fanart_image"));
  if (!strArt.empty())
    fileItem->SetProperty("fanart_image", strArt);

  for (map<string, CONTENT_ADDON_FILE_PROPERTY>::iterator it = item.begin(); it != item.end(); it++)
  {
    if (it->second.type == CONTENT_ADDON_PROPERTY_TYPE_STRING)
      fileItem->SetProperty(it->first, CVariant(it->second.strValue));
    else if (it->second.type == CONTENT_ADDON_PROPERTY_TYPE_INT)
      fileItem->SetProperty(it->first, CVariant(it->second.iValue));
  }
}

void CContentAddon::ReadFilePlaylist(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY> item, CFileItemList& xbmcItems)
{
  CMediaSource playlist;
  playlist.strPath = MusicBuildPath(CONTENT_ADDON_TYPE_PLAYLIST, GetPropertyString(item, "path"));
  playlist.strName = GetPropertyString(item, "name");
  if (playlist.strPath.empty() || playlist.strName.empty()) return;

  CFileItemPtr pItem(new CFileItem(playlist));
  AddCommonProperties(item, pItem);

  {
    CSingleLock lock(m_critSection);
    m_playlistNames.insert(make_pair(playlist.strPath, playlist.strName));
  }
  xbmcItems.Add(pItem);
}

void CContentAddon::ReadFileSong(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY> item, CFileItemList& xbmcItems, const string& strArtist /* = "" */, const string& strAlbum /* = "" */)
{
  CSong song;
  song.strFileName = MusicBuildPath(CONTENT_ADDON_TYPE_SONG, GetPropertyString(item, "path"), strArtist, strAlbum);
  song.strTitle    = GetPropertyString(item, "name");
  if (song.strFileName.empty() || song.strTitle.empty()) return;

  song.iTrack      = GetPropertyInt(item, "track");
  song.iDuration   = GetPropertyInt(item, "duration");
  song.rating      = GetPropertyInt(item, "rating");
  song.artist      = StringUtils::Split(GetPropertyString(item, "artists"), g_advancedSettings.m_musicItemSeparator);
  song.iYear       = GetPropertyInt(item, "year");
  song.strAlbum    = GetPropertyString(item, "album");
  song.albumArtist = StringUtils::Split(GetPropertyString(item, "album_artists"), g_advancedSettings.m_musicItemSeparator);

  CFileItemPtr pItem(new CFileItem(song));
  AddCommonProperties(item, pItem);

  xbmcItems.AddAutoJoin(pItem);
}

void CContentAddon::ReadFileAlbum(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY> item, CFileItemList& xbmcItems, const std::string& strArtist /* = "" */)
{
// TODO do something useful with CAlbum
//      CAlbum album;
//      album.strAlbum = addonItems->items[iPtr].album.strAlbum;
//      if (album.strAlbum.IsEmpty())
//        album.strAlbum = g_localizeStrings.Get(1050);
//      album.artist = StringUtils::Split(addonItems->items[iPtr].album.strArtists, g_advancedSettings.m_musicItemSeparator);
//      album.genre = StringUtils::Split(addonItems->items[iPtr].album.strGenres, g_advancedSettings.m_musicItemSeparator);
//      album.iYear = addonItems->items[iPtr].album.iYear;
//      if (addonItems->items[iPtr].album.strThumb[0] != 0)
//        album.thumbURL.ParseString(addonItems->items[iPtr].album.strThumb);
//      album.iRating = addonItems->items[iPtr].album.iRating;
//      album.strReview = addonItems->items[iPtr].album.strReview;
//      album.styles = StringUtils::Split(addonItems->items[iPtr].album.strStyles, g_advancedSettings.m_musicItemSeparator);
//      album.moods = StringUtils::Split(addonItems->items[iPtr].album.strMoods, g_advancedSettings.m_musicItemSeparator);
//      album.themes = StringUtils::Split(addonItems->items[iPtr].album.strThemes, g_advancedSettings.m_musicItemSeparator);
//      album.strLabel = addonItems->items[iPtr].album.strLabel;
//      album.strType = addonItems->items[iPtr].album.strType;
//      album.bCompilation = addonItems->items[iPtr].album.bCompilation == 1;

  vector<string> artists = StringUtils::Split(GetPropertyString(item, "artists"), g_advancedSettings.m_musicItemSeparator);
  const CStdString strAlbumArtist(strArtist.empty() && !artists.empty() ? artists.at(0) : strArtist);
  CMediaSource albumSource;
  albumSource.strPath = MusicBuildPath(CONTENT_ADDON_TYPE_ALBUM, GetPropertyString(item, "path"), strAlbumArtist);
  albumSource.strName = GetPropertyString(item, "name");
  if (albumSource.strPath.empty() || albumSource.strName.empty()) return;

  CFileItemPtr pItem(new CFileItem(albumSource));
  AddCommonProperties(item, pItem);

  {
    CSingleLock lock(m_critSection);
    map<CStdString, map<CStdString, CStdString> >::iterator it = m_albumNames.find(strAlbumArtist);
    if (it != m_albumNames.end())
      it->second.insert(make_pair(albumSource.strPath, albumSource.strName));
    else
    {
      map<CStdString, CStdString> m;
      m.insert(make_pair(albumSource.strPath, albumSource.strName));
      m_albumNames.insert(make_pair(strAlbumArtist, m));
    }
  }
  xbmcItems.Add(pItem);
}

void CContentAddon::ReadFileArtist(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY> item, CFileItemList& xbmcItems)
{
  CArtist artist;
  CStdString strPath  = MusicBuildPath(CONTENT_ADDON_TYPE_ARTIST, GetPropertyString(item, "path"));
  artist.strArtist    = GetPropertyString(item, "name");
  if (strPath.empty() || artist.strArtist.empty()) return;

  artist.genre        = StringUtils::Split(GetPropertyString(item, "genres"), g_advancedSettings.m_musicItemSeparator);
  artist.strBiography = GetPropertyString(item, "biography");
  artist.styles       = StringUtils::Split(GetPropertyString(item, "styles"), g_advancedSettings.m_musicItemSeparator);
  artist.moods        = StringUtils::Split(GetPropertyString(item, "moods"), g_advancedSettings.m_musicItemSeparator);
  artist.strBorn      = GetPropertyString(item, "born");
  artist.strFormed    = GetPropertyString(item, "formed");
  artist.strDied      = GetPropertyString(item, "died");
  artist.strDisbanded = GetPropertyString(item, "disbanded");
  artist.yearsActive  = StringUtils::Split(GetPropertyString(item, "years_active"), g_advancedSettings.m_musicItemSeparator);
  artist.instruments  = StringUtils::Split(GetPropertyString(item, "instruments"), g_advancedSettings.m_musicItemSeparator);

  CFileItemPtr pItem(new CFileItem(artist));
  pItem->SetPath(strPath);
  pItem->SetIconImage("DefaultArtist.png");
  pItem->SetProperty("artist_description", artist.strBiography);
  AddCommonProperties(item, pItem);

  {
    CSingleLock lock(m_critSection);
    m_artistNames.insert(make_pair(strPath, artist.strArtist));
  }
  xbmcItems.AddAutoJoin(pItem);
}

void CContentAddon::ReadFileDirectory(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY> item, CFileItemList& xbmcItems)
{
  CMediaSource m;
  m.strPath = ContentBuildPath(GetPropertyString(item, "path"));
  m.strName = GetPropertyString(item, "name");

  if (m.strPath.empty() || m.strName.empty())
    return;

  CFileItemPtr pItem(new CFileItem(m));
  AddCommonProperties(item, pItem);

  xbmcItems.Add(pItem);
}

void CContentAddon::ReadFileFile(std::map<std::string, CONTENT_ADDON_FILE_PROPERTY> item, CFileItemList& xbmcItems)
{
  CMediaSource m;
  m.strPath = ContentBuildPath(GetPropertyString(item, "path"));
  m.strName = GetPropertyString(item, "name");

  if (m.strPath.empty() || m.strName.empty())
    return;

  CFileItemPtr pItem(new CFileItem(m));
  AddCommonProperties(item, pItem);

  xbmcItems.Add(pItem);
}

void CContentAddon::ReadFiles(CONTENT_ADDON_FILELIST* addonItems, CFileItemList& xbmcItems, const string& strArtist /* = "" */, const string& strAlbum /* = "" */)
{
  for (unsigned iPtr = 0; iPtr < addonItems->iSize; iPtr++)
  {
    map<string, CONTENT_ADDON_FILE_PROPERTY> m = FileListToMap(addonItems->items[iPtr].properties, addonItems->items[iPtr].iSize);
    switch (addonItems->items[iPtr].type)
    {
    case CONTENT_ADDON_TYPE_SONG:
      ReadFileSong(m, xbmcItems, strArtist, strAlbum);
      break;
    case CONTENT_ADDON_TYPE_ARTIST:
      ReadFileArtist(m, xbmcItems);
      break;
    case CONTENT_ADDON_TYPE_ALBUM:
      ReadFileAlbum(m, xbmcItems, strArtist);
      break;
    case CONTENT_ADDON_TYPE_PLAYLIST:
      ReadFilePlaylist(m, xbmcItems);
      break;
    case CONTENT_ADDON_TYPE_DIRECTORY:
      ReadFileDirectory(m, xbmcItems);
      break;
    case CONTENT_ADDON_TYPE_FILE:
      ReadFileFile(m, xbmcItems);
      break;
    default:
      CLog::Log(LOGWARNING, "invalid filetype: %d", addonItems->items[iPtr].type);
      break;
    }
  }
}

bool CContentAddon::MusicGetPlaylists(CFileItemList& items)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  CLog::Log(LOGDEBUG, "getting playlists from add-on '%s'", Name().c_str());
  try { err = m_pStruct->MusicGetPlaylists(&retVal); }
  catch (exception &e) { LogException(e, __FUNCTION__); return false; }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items);

    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetArtists(CFileItemList& items)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  CLog::Log(LOGDEBUG, "getting artists from add-on '%s'", Name().c_str());
  try { err = m_pStruct->MusicGetArtists(&retVal); }
  catch (exception &e) { LogException(e, __FUNCTION__); return false; }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items);
    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetPlaylistContent(CFileItemList& items, const CStdString& strName)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  CLog::Log(LOGDEBUG, "getting playlist '%s' from add-on '%s'", MusicGetPlaylistName(strName).c_str(), Name().c_str());
  try { err = m_pStruct->MusicGetPlaylist(&retVal, strName.c_str()); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items);
    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetAlbums(CFileItemList& items, const CStdString& strArtist)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  if (strArtist.empty())
    CLog::Log(LOGDEBUG, "getting all albums from add-on '%s'", Name().c_str());
  else
    CLog::Log(LOGDEBUG, "getting albums for artist '%s' from add-on '%s'", MusicGetArtistName(strArtist).c_str(), Name().c_str());
  try { err = m_pStruct->MusicGetAlbums(&retVal, strArtist.c_str()); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items, strArtist);
    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetSongs(CFileItemList& items, const CStdString& strArtist, const CStdString& strAlbum)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  if (strAlbum.empty())
    CLog::Log(LOGDEBUG, "getting songs from artist '%s' from add-on '%s'", MusicGetArtistName(strArtist).c_str(), Name().c_str());
  else
    CLog::Log(LOGDEBUG, "getting songs for album '%s' from artist '%s' from add-on '%s'", MusicGetAlbumName(strArtist, strAlbum).c_str(), MusicGetArtistName(strArtist).c_str(), Name().c_str());

  try { err = m_pStruct->MusicGetSongs(&retVal, strArtist.c_str(), strAlbum.c_str()); }
  catch (exception &e) { LogException(e, __FUNCTION__); return false; }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items, strArtist, strAlbum);

    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetCodecInfo(const string& strPath, CONTENT_ADDON_CODEC_INFO* info)
{
  if (!ReadyToUse() || !ProvidesMusicCodec())
    return false;

  CStdString strFilePath = GetFilename(strPath);

  try { return m_pStruct->MusicGetCodecInfo(strFilePath.c_str(), info) == CONTENT_ERROR_NO_ERROR; }
  catch (exception &e) { LogException(e, __FUNCTION__); }
  return false;
}

CStdString CContentAddon::GetFilename(const CStdString& strPath) const
{
  CStdString retval(strPath);

  // check whether the filename starts with content://id/
  CStdString strContentNode;
  strContentNode.Format("%s%s/", CONTENT_NODE, ID().c_str());
  if (strPath.Left(strContentNode.length()).Equals(strContentNode))
    return strPath.Right(strPath.length() - strContentNode.length());

  // check whether the file resolves to a directory node that we created
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (pNode.get())
  {
    CContentAddonDirectoryNode* pAddonNode = dynamic_cast<CContentAddonDirectoryNode*>(pNode.get());
    if (pAddonNode)
    {
      CONTENT_ADDON addon = pAddonNode->GetAddon();
      if (addon.get() && addon->ID().Equals(ID()))
        retval = pAddonNode->Filename();
    }
  }

  return retval;
}

bool CContentAddon::MusicOpenFile(const string& strPath)
{
  if (!CreateOnDemand() || !ProvidesMusicCodec() || !SupportsFile(strPath))
    return false;

  CStdString strFilePath = GetFilename(strPath);

  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);
  try { err = m_pStruct->MusicOpenFile(strFilePath.c_str()); }
  catch (exception &e) { LogException(e, __FUNCTION__); return false; }

  if (err != CONTENT_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "add-on '%s' returned an error from OpenMusicFile(%s): %d", Name().c_str(), strPath.c_str(), err);

  return err == CONTENT_ERROR_NO_ERROR;
}

bool CContentAddon::MusicPreloadFile(const string& strPath)
{
  if (!CreateOnDemand() || !ProvidesMusicCodec())
    return false;

  CStdString strFilePath = GetFilename(strPath);

  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);
  try { err = m_pStruct->MusicPreloadFile(strFilePath.c_str()); }
  catch (exception &e) { LogException(e, __FUNCTION__); return false; }

  if (err != CONTENT_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "add-on '%s' returned an error from MusicPreloadFile(%s): %d", Name().c_str(), strPath.c_str(), err);

  return err == CONTENT_ERROR_NO_ERROR;
}

void CContentAddon::MusicCloseFile(void)
{
  if (!ReadyToUse() || !ProvidesMusicCodec())
    return;

  try { m_pStruct->MusicCloseFile(); }
  catch (exception &e) { LogException(e, __FUNCTION__); }
}

void CContentAddon::MusicPause(bool bPause)
{
  if (!ReadyToUse() || !ProvidesMusicCodec())
    return;

  {
    CSingleLock lock(m_critSection);
    if ((m_playState == CONTENT_ADDON_PLAYSTATE_PAUSE && bPause) ||
        (m_playState == CONTENT_ADDON_PLAYSTATE_PLAY && !bPause) ||
        m_playState == CONTENT_ADDON_PLAYSTATE_STOP)
      return;
    m_playState = bPause ? CONTENT_ADDON_PLAYSTATE_PAUSE : CONTENT_ADDON_PLAYSTATE_PLAY;
  }

  try { m_pStruct->MusicPause(bPause ? 1 : 0); }
  catch (exception &e) { LogException(e, __FUNCTION__); }
}

int64_t CContentAddon::MusicSeek(int64_t iSeekTime)
{
  if (!ReadyToUse() || !ProvidesMusicCodec())
    return -1;

  try { return m_pStruct->MusicSeek(iSeekTime); }
  catch (exception &e) { LogException(e, __FUNCTION__); }
  return 0;
}

int CContentAddon::MusicReadPCM(BYTE* pBuffer, int size, int* actualsize)
{
  if (!ReadyToUse() || !ProvidesMusicCodec())
    return -1;

  try { return m_pStruct->MusicReadPCM(pBuffer, size, actualsize); }
  catch (exception &e) { LogException(e, __FUNCTION__); }
  return -1;
}

void CContentAddon::LogException(const exception &e, const char *strFunctionName) const
{
  CLog::Log(LOGERROR, "exception '%s' caught while trying to call '%s' on add-on '%s'. Please contact the developer of this add-on: %s", e.what(), strFunctionName, Name().c_str(), Author().c_str());
}

CStdString CContentAddon::MusicGetPlaylistName(const CStdString& strPlaylist) const
{
  CStdString strReturn(strPlaylist);
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return strReturn;
  CSingleLock lock(m_critSection);
  map<CStdString, CStdString>::const_iterator it = m_playlistNames.find(strPlaylist);
  if (it != m_playlistNames.end())
    strReturn = it->second;
  return strReturn;
}

CStdString CContentAddon::MusicGetArtistName(const CStdString& strArtist) const
{
  CStdString strReturn(strArtist);
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return strReturn;
  CSingleLock lock(m_critSection);
  map<CStdString, CStdString>::const_iterator it = m_artistNames.find(strArtist);
  if (it != m_artistNames.end())
    strReturn = it->second;
  return strReturn;
}

CStdString CContentAddon::MusicGetAlbumName(const CStdString& strArtist, const CStdString& strAlbum) const
{
  CStdString strReturn(strAlbum);
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return strReturn;
  CSingleLock lock(m_critSection);

  if (!strArtist.empty())
  {
    map<CStdString, map<CStdString, CStdString> >::const_iterator it = m_albumNames.find(strArtist.c_str());
    if (it != m_albumNames.end())
    {
      map<CStdString, CStdString>::const_iterator it2 = it->second.find(strAlbum.c_str());
      if (it2 != it->second.end())
        return it2->second;
    }
  }

  // return the first match
  for (map<CStdString, map<CStdString, CStdString> >::const_iterator it = m_albumNames.begin(); it != m_albumNames.end(); it++)
  {
    map<CStdString, CStdString>::const_iterator it2 = it->second.find(strAlbum.c_str());
    if (it2 != it->second.end())
    {
      strReturn = it2->second;
      break;
    }
  }
  return strReturn;
}

void CContentAddon::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (!strcmp(sender, "xbmc") && !strcmp(message, "OnStop"))
    MusicCloseFile();
  else if (!strcmp(sender, "xbmc") && !strcmp(message, "OnPause"))
    MusicPause(true);
  else if (!strcmp(sender, "xbmc") && !strcmp(message, "OnPlay"))
    MusicPause(false);
}

bool CContentAddon::MusicGetTop100(CFileItemList& items, CONTENT_TOP100_TYPE type)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  CLog::Log(LOGDEBUG, "getting top100 from add-on '%s'", Name().c_str());
  try { err = m_pStruct->MusicGetTop100(&retVal, type); }
  catch (exception &e) { LogException(e, __FUNCTION__); return false; }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items);
    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetOverviewItems(CFileItemList& items)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  CLog::Log(LOGDEBUG, "getting overview items from add-on '%s'", Name().c_str());
  try { err = m_pStruct->MusicGetOverviewItems(&retVal); }
  catch (exception &e) { LogException(e, __FUNCTION__); }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items);
    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetOverview(CFileItemList& items)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CStdString strPrepend;
  strPrepend.Format("%s%s/", MUSIC_VIRTUAL_NODE, ID().c_str());

  // TODO only add items that this add-on supports

  CFileItemPtr pArtistItem(new CFileItem(g_localizeStrings.Get(133)));
  CStdString strDir;
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_ARTIST);
  pArtistItem->SetPath(strDir);
  pArtistItem->m_bIsFolder = true;
  pArtistItem->SetCanQueue(false);
  items.Add(pArtistItem);

  CFileItemPtr pAlbumItem(new CFileItem(g_localizeStrings.Get(132)));
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_ALBUM);
  pAlbumItem->SetPath(strDir);
  pAlbumItem->m_bIsFolder = true;
  pAlbumItem->SetCanQueue(false);
  items.Add(pAlbumItem);

  CFileItemPtr pSongItem(new CFileItem(g_localizeStrings.Get(134)));
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_SONG);
  pSongItem->SetPath(strDir);
  pSongItem->m_bIsFolder = true;
  pSongItem->SetCanQueue(false);
  items.Add(pSongItem);

  CFileItemPtr pTop100Item(new CFileItem(g_localizeStrings.Get(271)));
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_TOP100);
  pTop100Item->SetPath(strDir);
  pTop100Item->m_bIsFolder = true;
  pTop100Item->SetCanQueue(false);
  items.Add(pTop100Item);

  CFileItemPtr pPlaylistItem(new CFileItem(g_localizeStrings.Get(136)));
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_PLAYLIST);
  pPlaylistItem->SetPath(strDir);
  pPlaylistItem->m_bIsFolder = true;
  pPlaylistItem->SetCanQueue(false);
  items.Add(pPlaylistItem);

  return true;
}

bool CContentAddon::MusicGetTop100Overview(CFileItemList& items)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CStdString strPrepend;
  strPrepend.Format("%s%s/%s/", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_TOP100);

  // TODO only add items that this add-on supports

  CFileItemPtr pArtistItem(new CFileItem(g_localizeStrings.Get(133)));
  CStdString strDir;
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_ARTIST);
  pArtistItem->SetPath(strDir);
  pArtistItem->m_bIsFolder = true;
  pArtistItem->SetCanQueue(false);
  items.Add(pArtistItem);

  CFileItemPtr pAlbumItem(new CFileItem(g_localizeStrings.Get(132)));
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_ALBUM);
  pAlbumItem->SetPath(strDir);
  pAlbumItem->m_bIsFolder = true;
  pAlbumItem->SetCanQueue(false);
  items.Add(pAlbumItem);

  CFileItemPtr pSongItem(new CFileItem(g_localizeStrings.Get(134)));
  strDir.Format("%s%s/", strPrepend.c_str(), MUSIC_SONG);
  pSongItem->SetPath(strDir);
  pSongItem->m_bIsFolder = true;
  pSongItem->SetCanQueue(false);
  items.Add(pSongItem);

  return true;
}

bool CContentAddon::MusicSearch(CFileItemList& items, const CStdString& strQuery)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  CONTENT_ADDON_FILELIST* retVal = NULL;
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  CLog::Log(LOGDEBUG, "getting searching for '%s' in add-on '%s'", strQuery.c_str(), Name().c_str());
  try { err = m_pStruct->MusicSearch(&retVal, strQuery.c_str()); }
  catch (exception &e) { LogException(e, __FUNCTION__); return false; }

  if (err == CONTENT_ERROR_NO_ERROR && retVal)
  {
    ReadFiles(retVal, items);

    FreeFileList(retVal);
    return true;
  }

  return false;
}

bool CContentAddon::MusicGetContextButtons(const CFileItemPtr& item, CContextButtons &buttons)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  // TODO
  return false;
}

bool CContentAddon::MusicClickContextButton(const CFileItemPtr& item, CONTEXT_BUTTON button)
{
  if (!ReadyToUse() || !ProvidesMusicFiles())
    return false;

  // TODO
  return false;
}

void CContentAddon::SetPlaystate(CONTENT_ADDON_PLAYSTATE newState)
{
  CLog::Log(LOGDEBUG, "%s(%d)", __FUNCTION__, (int)newState);
  CSingleLock lock(m_critSection);
  switch (newState)
  {
  case CONTENT_ADDON_PLAYSTATE_PAUSE:
    if (m_playState == CONTENT_ADDON_PLAYSTATE_PLAY)
    {
      m_playState = CONTENT_ADDON_PLAYSTATE_PAUSE;
      CApplicationMessenger::Get().MediaPause();
    }
    break;
  case CONTENT_ADDON_PLAYSTATE_PLAY:
    if (m_playState == CONTENT_ADDON_PLAYSTATE_PAUSE)
    {
      m_playState = CONTENT_ADDON_PLAYSTATE_PLAY;
      CApplicationMessenger::Get().MediaPause();
    }
    break;
  case CONTENT_ADDON_PLAYSTATE_STOP:
    if (m_playState != CONTENT_ADDON_PLAYSTATE_STOP)
    {
      m_playState = CONTENT_ADDON_PLAYSTATE_STOP;
      CApplicationMessenger::Get().MediaStop();
    }
    break;
  default:
    CLog::Log(LOGWARNING, "add-on '%s' tried to set an invalid playstate (%d)", Name().c_str(), (int)newState);
    break;
  }
}

bool CContentAddon::SupportsFile(const CStdString& strPath) const
{
  // check whether the file resolves to a directory node that we created
  CStdString strContentNode;
  strContentNode.Format("%s%s/", CONTENT_NODE, ID().c_str());
  if (strPath.Left(strContentNode.length()).Equals(strContentNode))
    return true;

  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (pNode.get())
  {
    CContentAddonDirectoryNode* pAddonNode = dynamic_cast<CContentAddonDirectoryNode*>(pNode.get());
    if (pAddonNode)
    {
      CONTENT_ADDON addon = pAddonNode->GetAddon();
      return (addon.get() && addon->ID().Equals(ID()));
    }
  }

  // check whether the filetype matches any of the filetypes in addon.xml
  CURL url(strPath);
  if (SupportsFileType(url.GetFileType()))
    return true;

  // if this add-on provides a music codec, ask the add-on whether it supports this filename
  if (ProvidesMusicCodec())
  {
    try { return m_pStruct->SupportsFile(strPath.c_str()); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  // no match, the add-on doesn't support this file
  return false;
}

bool CContentAddon::SupportsFileType(const string& strType) const
{
  return find(m_fileTypes.begin(), m_fileTypes.end(), strType) != m_fileTypes.end();
}

bool CContentAddon::CreateOnDemand(void)
{
  if (ReadyToUse())
    return true;
  if (OnDemand())
  {
    CLog::Log(LOGDEBUG, "Creating on-demand add-on '%s'", Name().c_str());
    return Create() == ADDON_STATUS_OK;
  }
  return false;
}

CStdString CContentAddon::MusicBuildPath(CONTENT_ADDON_TYPE type, const CStdString& strFilename, const CStdString& strArtist /* = "" */, const CStdString& strAlbum /* = "" */) const
{
  if (strFilename.empty())
    return strFilename;

  CURL fileName(strFilename);
  if (!URIUtils::IsInternetStream(fileName))
  {
    CStdString strReturn;
    switch (type)
    {
    case CONTENT_ADDON_TYPE_SONG:
      {
        if (!strArtist.empty())
        {
          if (!strAlbum.empty())
            strReturn.Format("%s%s/%s/%s/%s/%s", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_ARTIST, strArtist.c_str(), strAlbum.c_str(), strFilename);
          else
            strReturn.Format("%s%s/%s/%s/%s", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_ARTIST, strArtist.c_str(), strFilename);
        }
        else
          strReturn.Format("%s%s/%s/%s", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_SONG, strFilename);
      }
      break;
    case CONTENT_ADDON_TYPE_ARTIST:
      strReturn.Format("%s%s/%s/%s/", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_ARTIST, strFilename.c_str());
      break;
    case CONTENT_ADDON_TYPE_PLAYLIST:
      strReturn.Format("%s%s/%s/%s", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_PLAYLIST, strFilename.c_str());
      break;
    case CONTENT_ADDON_TYPE_DIRECTORY:
      strReturn.Format("%s%s/%s", MUSIC_VIRTUAL_NODE, ID().c_str(), strFilename.c_str());
      break;
    case CONTENT_ADDON_TYPE_ALBUM:
      {
        if (strArtist.empty())
          strReturn.Format("%s%s/%s/%s/", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_ALBUM, strFilename.c_str());
        else
          strReturn.Format("%s%s/%s/%s/%s", MUSIC_VIRTUAL_NODE, ID().c_str(), MUSIC_ARTIST, strArtist.c_str(), strFilename.c_str());
      }
      break;
    default:
      break;
    }

    return strReturn;
  }

  return strFilename;
}

CStdString CContentAddon::ContentBuildPath(const CStdString& strPath)
{
  if (strPath.empty() || URIUtils::IsInternetStream(CURL(strPath)))
    return strPath;

  CStdString retVal;
  retVal.Format("%s%s/%s", CONTENT_NODE, ID().c_str(), strPath.c_str());
  return retVal;
}

bool CContentAddon::FileOpen(const CStdString& strFileName, CONTENT_HANDLE* handle)
{
  CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);

  if (ProvidesFiles())
  {
    CStdString strFilePath = GetFilename(strFileName);
    try { err = m_pStruct->FileOpen(strFilePath.c_str(), handle); }
    catch (exception &e) { LogException(e, __FUNCTION__); return false; }

    if (err != CONTENT_ERROR_NO_ERROR)
      CLog::Log(LOGERROR, "add-on '%s' returned an error from FileOpen(%s): %d", Name().c_str(), strFileName.c_str(), err);
  }

  return err == CONTENT_ERROR_NO_ERROR;
}

void CContentAddon::FileClose(CONTENT_HANDLE handle)
{
  if (ProvidesFiles())
  {
    try { m_pStruct->FileClose(handle); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }
}

unsigned int CContentAddon::FileRead(CONTENT_HANDLE handle, void* pBuffer, int64_t iBufLen)
{
  if (ProvidesFiles())
  {
    try { return m_pStruct->FileRead(handle, pBuffer, iBufLen); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  return 0;
}

bool CContentAddon::FileExists(const CStdString& strFileName)
{
  if (ProvidesFiles())
  {
    CStdString strFilePath = GetFilename(strFileName);
    try { return m_pStruct->FileExists(strFilePath.c_str()) == 0; }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  return false;
}

int64_t CContentAddon::FileSeek(CONTENT_HANDLE handle, int64_t iFilePosition, int iWhence)
{
  if (ProvidesFiles())
  {
    try { return m_pStruct->FileSeek(handle, iFilePosition, iWhence); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  return 0;
}

int64_t CContentAddon::FileGetPosition(CONTENT_HANDLE handle)
{
  if (ProvidesFiles())
  {
    try { return m_pStruct->FileGetPosition(handle); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  return 0;
}

int64_t CContentAddon::FileGetLength(CONTENT_HANDLE handle)
{
  if (ProvidesFiles())
  {
    try { return m_pStruct->FileGetLength(handle); }
    catch (exception &e) { LogException(e, __FUNCTION__); }
  }

  return 0;
}

bool CContentAddon::FileGetDirectory(CFileItemList& items, const CStdString& strPath)
{
  if (ProvidesFiles())
  {
    CONTENT_ADDON_FILELIST* retVal = NULL;
    CONTENT_ERROR err(CONTENT_ERROR_UNKNOWN);
    CLog::Log(LOGDEBUG, "getting directory '%s' from add-on '%s'", strPath.c_str(), Name().c_str());

    try { err = m_pStruct->FileGetDirectory(&retVal, strPath.c_str()); }
    catch (exception &e) { LogException(e, __FUNCTION__); }

    if (err == CONTENT_ERROR_NO_ERROR && retVal)
    {
      ReadFiles(retVal, items);
      FreeFileList(retVal);
      return true;
    }
  }

  return false;
}
