#include "CNX_GstMoviePlayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOG_TAG "[CNX_GstMoviePlayer]"
#include <NX_Log.h>
#include <math.h>
#include <media/CNX_Base.h>

//------------------------------------------------------------------------------
CNX_GstMoviePlayer::CNX_GstMoviePlayer(QWidget *parent)
    : debug(false)
    , m_hPlayer(NULL)
	, m_pAudioDeviceName(NULL)
	, m_fSpeed(1.0)
	, m_pSubtitleParser(NULL)
	, m_iSubtitleSeekTime(0)
{
	pthread_mutex_init(&m_hLock, NULL);
	pthread_mutex_init(&m_SubtitleLock, NULL);

    memset(&m_MediaInfo, 0, sizeof(GST_MEDIA_INFO));

	// Subtitle
	m_pSubtitleParser = new CNX_SubtitleParser();
}

CNX_GstMoviePlayer::~CNX_GstMoviePlayer()
{
	pthread_mutex_destroy(&m_hLock);
	pthread_mutex_destroy(&m_SubtitleLock);
	if(m_pSubtitleParser) {
		delete m_pSubtitleParser;
		m_pSubtitleParser = NULL;
	}
}

//================================================================================================================
//public methods	commomn Initialize , close
int CNX_GstMoviePlayer::InitMediaPlayer(void (*pCbEventCallback)(void *privateDesc,
                                                                 unsigned int EventType,
                                                                 unsigned int EventData,
                                                                 void* param),
                                     void *pCbPrivate,
                                     const char *pUri,
									 DISPLAY_INFO dspInfo,
                                     char *pAudioDeviceName)
{
	CNX_AutoLock lock( &m_hLock );

	NXLOGI("%s", __FUNCTION__);

	m_pAudioDeviceName = pAudioDeviceName;

	if(0 > OpenHandle(pCbEventCallback, pCbPrivate))		return -1;
	if(0 > SetDisplayMode(dspInfo.dspMode))					return -1;
	if(0 > SetUri(pUri))									return -1;
	if(0 > GetMediaInfo())									return -1;
	if(0 > SetAspectRatio(dspInfo))							return -1;

	return 0;
}

int CNX_GstMoviePlayer::SetAspectRatio(DISPLAY_INFO dspInfo)
{
	DSP_RECT m_dstDspRect;
	DSP_RECT m_dstSubDspRect;

	NXLOGI("%s() dspInfo(%d, %d, %d, %d, %d)",
			__FUNCTION__, dspInfo.dspWidth, dspInfo.dspHeight,
			dspInfo.dspMode, dspInfo.subDspWidth, dspInfo.subDspHeight);

	int video_width = m_MediaInfo.StreamInfo[0].VideoInfo[0].width;
	int video_height = m_MediaInfo.StreamInfo[0].VideoInfo[0].height;

	// Set aspect ratio for the primary display
	memset(&m_dstDspRect, 0, sizeof(DSP_RECT));

	GetAspectRatio(video_width, video_height,
				   dspInfo.dspWidth, dspInfo.dspHeight,
				   &m_dstDspRect);
	if (0 > SetDisplayInfo(DISPLAY_TYPE_PRIMARY, dspInfo.dspWidth, dspInfo.dspHeight, m_dstDspRect)) {
		NXLOGE("%s() Failed to set aspect ratio rect for primary", __FUNCTION__);
		return -1;
	}
	NXLOGI("%s() m_dstDspRect(%d, %d, %d, %d)",
			__FUNCTION__, m_dstDspRect.left, m_dstDspRect.top,
			m_dstDspRect.right, m_dstDspRect.bottom);

	// Set aspect ratio for the secondary display
	{
		memset(&m_dstSubDspRect, 0, sizeof(DSP_RECT));
		GetAspectRatio(video_width, video_height,
						dspInfo.subDspWidth, dspInfo.subDspHeight,
						&m_dstSubDspRect);
		if(0 > SetDisplayInfo(DISPLAY_TYPE_SECONDARY, dspInfo.subDspWidth, dspInfo.subDspHeight, m_dstSubDspRect)) {
			NXLOGE("%s() Failed to set aspect ratio rect for secondary", __FUNCTION__);
			return -1;
		}
	}
	NXLOGI("%s() m_dstSubDspRect(%d, %d, %d, %d)",
			__FUNCTION__, m_dstSubDspRect.left, m_dstSubDspRect.top,
			m_dstSubDspRect.right, m_dstSubDspRect.bottom);
	return 0;
}

int CNX_GstMoviePlayer::CloseHandle()
{
	CNX_AutoLock lock( &m_hLock );
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GSTMP_Close(m_hPlayer);

	m_hPlayer = NULL;

	return 0;
}

//================================================================================================================
//public methods	common Control
int CNX_GstMoviePlayer::SetVolume(int volume)
{
	CNX_AutoLock lock( &m_hLock );
	if(NULL == m_hPlayer)
	{
		NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_SetVolume(m_hPlayer, volume);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE( "%s(): Error! NX_MPSetVolume() Failed! (ret = %d)", __FUNCTION__, iResult);
		return -1;
	}
	return 0;
}

int CNX_GstMoviePlayer::Play()
{
	CNX_AutoLock lock( &m_hLock );
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_Play(m_hPlayer);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_MPPlay() Failed! (ret = %d)", __FUNCTION__, iResult);
		return -1;
	}

	return 0;
}

int CNX_GstMoviePlayer::Seek(qint64 position)
{
	CNX_AutoLock lock(&m_hLock);
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_Seek(m_hPlayer, (gint64)position);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE( "%s(): Error! NX_MPSeek() Failed! (ret = %d)", __FUNCTION__, iResult);
		return -1;
	}
	return 0;
}

int CNX_GstMoviePlayer::Pause()
{
	CNX_AutoLock lock(&m_hLock);
	if(NULL == m_hPlayer)
	{
		NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_Pause(m_hPlayer);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_MPPause() Failed! (ret = %d)", __FUNCTION__, iResult);
		return -1;
	}

	return 0;
}

int CNX_GstMoviePlayer::Stop()
{
	CNX_AutoLock lock(&m_hLock);
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_Stop(m_hPlayer);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE( "%s(): Error! NX_MPStop() Failed! (ret = %d)", __FUNCTION__, iResult);
		return -1;
	}

	return 0;
}

//================================================================================================================
//public methods	common information
int64_t CNX_GstMoviePlayer::GetMediaPosition()
{
	CNX_AutoLock lock(&m_hLock);
	if(NULL == m_hPlayer)
	{
		NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
		return -1;
	}

	int64_t position = NX_GSTMP_GetPosition(m_hPlayer);
	if(-1 == position)
	{
		NXLOGE( "%s(): Error! NX_MPGetPosition() Failed!", __FUNCTION__);
		return -1;
	}

	return position;
}

int64_t CNX_GstMoviePlayer::GetMediaDuration()
{
	CNX_AutoLock lock(&m_hLock);
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	int64_t duration = NX_GSTMP_GetDuration(m_hPlayer);
	if(-1 == duration)
	{
		NXLOGE( "%s(): Error! NX_MPGetDuration() Failed!", __FUNCTION__);
		return -1;
	}

	return duration;
}

NX_MEDIA_STATE CNX_GstMoviePlayer::GetState()
{
	CNX_AutoLock lock(&m_hLock);
	if(NULL == m_hPlayer)
	{
		NXLOGI("%s() handle is NULL, state is considered as STOPPED!", __FUNCTION__);
		return MP_STATE_STOPPED;
	}
	return (NX_MEDIA_STATE)NX_GSTMP_GetState(m_hPlayer);
}

//================================================================================================================
int CNX_GstMoviePlayer::OpenHandle(void (*pCbEventCallback)(void *privateDesc, unsigned int EventType,
															unsigned int EventData, void* param),
								 void *cbPrivate)
{
	NXLOGI("%s", __FUNCTION__);

	NX_GST_RET iResult = NX_GSTMP_Open(&m_hPlayer, pCbEventCallback, cbPrivate);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE( "%s: Error! Handle is not initialized!\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

int CNX_GstMoviePlayer::SetDisplayMode(DISPLAY_MODE mode)
{
	NXLOGI("%s mode(%d)", __FUNCTION__, mode);

	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_SetDisplayMode(m_hPlayer, mode);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_GSTMP_SetDisplayMode() Failed! (ret = %d, mode = %d)\n", __FUNCTION__, iResult, mode);
		return -1;
	}
	return 0;
}

int CNX_GstMoviePlayer::SetUri(const char *pUri)
{
	NXLOGI("%s", __FUNCTION__);

	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_SetUri(m_hPlayer, pUri);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_MPSetUri() Failed! (ret = %d, uri = %s)\n", __FUNCTION__, iResult, pUri);
		return -1;
	}
	return 0;
}

int CNX_GstMoviePlayer::GetMediaInfo()
{
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}
	NX_GST_RET iResult = NX_GSTMP_GetMediaInfo(m_hPlayer, &m_MediaInfo);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_MPGetMediaInfo() Failed! (ret = %d)\n", __FUNCTION__, iResult);
		return -1;
	}

	return 0;
}

int CNX_GstMoviePlayer::SetDisplayInfo(enum DISPLAY_TYPE type, int dspWidth, int dspHeight, DSP_RECT rect)
{
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}
	NX_GST_RET iResult = NX_GSTMP_SetDisplayInfo(m_hPlayer, type, dspWidth, dspHeight, rect);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_GSTMP_SetDisplayInfo() Failed! (ret = %d)\n", __FUNCTION__, iResult);
		return -1;
	}

	return 0;
}

int CNX_GstMoviePlayer::DrmVideoMute(int bOnOff)
{
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	m_bVideoMute = bOnOff;
	NX_GST_RET iResult = NX_GSTMP_VideoMute(m_hPlayer, m_bVideoMute);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_GSTMP_VideoMute() Failed! (ret = %d)\n", __FUNCTION__, iResult);
		return -1;
	}

	return 0;
}

//================================================================================================================
// subtitle routine
void CNX_GstMoviePlayer::CloseSubtitle()
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser)
	{
		if(m_pSubtitleParser->NX_SPIsParsed())
		{
			m_pSubtitleParser->NX_SPClose();
		}
	}
}

int CNX_GstMoviePlayer::OpenSubtitle(char * subtitlePath)
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser)
	{
		return m_pSubtitleParser->NX_SPOpen(subtitlePath);
	}
	else
	{
		NXLOGW("in OpenSubtitle no parser instance\n");
		return 0;
	}
}

int CNX_GstMoviePlayer::GetSubtitleStartTime()
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser->NX_SPIsParsed())
	{
		return m_pSubtitleParser->NX_SPGetStartTime();
	}
	else
	{
		return 0;
	}
}

void CNX_GstMoviePlayer::SetSubtitleIndex(int idx)
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser->NX_SPIsParsed())
	{
		m_pSubtitleParser->NX_SPSetIndex(idx);
	}
}

int CNX_GstMoviePlayer::GetSubtitleIndex()
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser->NX_SPIsParsed())
	{
		return m_pSubtitleParser->NX_SPGetIndex();
	}
	else
	{
		return 0;
	}
}

int CNX_GstMoviePlayer::GetSubtitleMaxIndex()
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser->NX_SPIsParsed())
	{
		return m_pSubtitleParser->NX_SPGetMaxIndex();
	}
	else
	{
		return 0;
	}
}

void CNX_GstMoviePlayer::IncreaseSubtitleIndex()
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser->NX_SPIsParsed())
	{
		m_pSubtitleParser->NX_SPIncreaseIndex();
	}
}

char* CNX_GstMoviePlayer::GetSubtitleText()
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser->NX_SPIsParsed())
	{
		return m_pSubtitleParser->NX_SPGetSubtitle();
	}
	else
	{
		return NULL;
	}
}

bool CNX_GstMoviePlayer::IsSubtitleAvailable()
{
	return m_pSubtitleParser->NX_SPIsParsed();
}

const char *CNX_GstMoviePlayer::GetBestSubtitleEncode()
{
	CNX_AutoLock lock( &m_SubtitleLock );
	if(m_pSubtitleParser->NX_SPIsParsed())
	{
		return m_pSubtitleParser->NX_SPGetBestTextEncode();
	}
	else
	{
		return NULL;
	}
}

const char *CNX_GstMoviePlayer::GetBestStringEncode(const char *str)
{
	if(!m_pSubtitleParser)
	{
		NXLOGW("GetBestStringEncode no parser instance\n");
		return "EUC-KR";
	}
	else
	{
		return m_pSubtitleParser->NX_SPFindStringEncode(str);
	}
}

void CNX_GstMoviePlayer::SeekSubtitle(int milliseconds)
{
	if (0 > pthread_create(&m_subtitleThread, NULL, ThreadWrapForSubtitleSeek, this) )
	{
		NXLOGE("SeekSubtitle creating Thread err\n");
		m_pSubtitleParser->NX_SPSetIndex(0);
		return;
	}

	m_iSubtitleSeekTime = milliseconds;
	NXLOGD("seek input  : %d\n",milliseconds);

	pthread_join(m_subtitleThread, NULL);
}

void* CNX_GstMoviePlayer::ThreadWrapForSubtitleSeek(void *Obj)
{
	if( NULL != Obj )
	{
		NXLOGD("ThreadWrapForSubtitleSeek ok");
		( (CNX_GstMoviePlayer*)Obj )->SeekSubtitleThread();
	}
	else
	{
		NXLOGE("ThreadWrapForSubtitleSeek err");
		return (void*)0xDEADDEAD;
	}
	return (void*)1;
}

void CNX_GstMoviePlayer::SeekSubtitleThread(void)
{
	CNX_AutoLock lock( &m_SubtitleLock );
	m_pSubtitleParser->NX_SPSetIndex(m_pSubtitleParser->NX_SPSeekSubtitleIndex(m_iSubtitleSeekTime));
}

void CNX_GstMoviePlayer::GetAspectRatio(int srcWidth, int srcHeight,
									 int dspWidth, int dspHeight,
									 DSP_RECT *pDspDstRect)
{
	// Calculate Video Aspect Ratio
	double xRatio = (double)dspWidth / (double)srcWidth;
	double yRatio = (double)dspHeight / (double)srcHeight;

	int width, height, x = 0, y = 0;

	if( xRatio > yRatio )
	{
		width    = (int)((double)srcWidth * yRatio);
		height   = dspHeight;
	}
	else
	{
		width    = dspWidth;
		height   = (int)((double)srcHeight * xRatio);
	}

	if(dspWidth != width)
	{
		if(dspWidth > width)
		{
			x = (dspWidth - width)/2;
		}
	}

	if(dspHeight != height)
	{
		if(dspHeight > height)
		{
			y = (dspHeight - height)/2;
		}
	}

	pDspDstRect->left = x;
	pDspDstRect->right = x + width;
	pDspDstRect->top = y;
	pDspDstRect->bottom = y + height;
}

double CNX_GstMoviePlayer::GetVideoSpeed()
{
	double speed = 1.0;

	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return speed;
	}

	speed = NX_GSTMP_GetVideoSpeed(m_hPlayer);
	NXLOGI("%s() GetVideoSpeed(%f)", __FUNCTION__, speed);
	return speed;
}

int CNX_GstMoviePlayer::SetVideoSpeed(double rate)
{
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NXLOGI("%s() SetVideoSpeed(%f)", __FUNCTION__, rate);
	NX_GST_RET iResult = NX_GSTMP_SetVideoSpeed(m_hPlayer, rate);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE("%s(): Error! NX_GSTMP_SetVideoSpeed() Failed! (ret = %d)\n", __FUNCTION__, iResult);
		return -1;
	}

	return 0;
}

int	CNX_GstMoviePlayer::GetVideoSpeedSupport()
{
	int ret = 0;

	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return -1;
	}

	NX_GST_RET iResult = NX_GSTMP_GetVideoSpeedSupport(m_hPlayer);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGE( "%s: Error! This file doesn't support changing playback speed!", __FUNCTION__);
		return -1;
	}

	return ret;
}

bool CNX_GstMoviePlayer::HasSubTitleStream()
{
	if(NULL == m_hPlayer)
	{
		NXLOGE("%s: Error! Handle is not initialized!", __FUNCTION__);
		return false;
	}
	NXLOGI("%s() %s", __FUNCTION__, ((m_MediaInfo.n_subtitle > 0)?"true":"false"));
	return (m_MediaInfo.n_subtitle > 0) ? true:false;
}

int CNX_GstMoviePlayer::MakeThumbnail(const char *pUri, int64_t pos_msec, int32_t width, const char *outPath)
{
	NXLOGI("%s", __FUNCTION__);

	NX_GST_RET iResult = NX_GSTMP_MakeThumbnail(pUri, pos_msec, width, outPath);
	if(NX_GST_RET_OK != iResult)
	{
		NXLOGI("%s Failed to make thumbnail", __FUNCTION__);
		return -1;
	}

	return 0;
}