//------------------------------------------------------------------------------
//
//	Copyright (C) 2015 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		: libnxgstvplayer.so
//	File		:
//	Description	:
//	Author		:
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __NX_GSTTYPES_H
#define __NX_GSTTYPES_H

#include <glib.h>

#define MAX_TRACK_NUM		10

typedef struct MOVIE_TYPE	*MP_HANDLE;

/*! \enum NX_GST_EVENT
 * \brief Describes the event types */
enum NX_GST_EVENT {
    /*! \brief EOS event */
    MP_EVENT_EOS                = 0,
    /*! \brief Demux error */
    MP_EVENT_DEMUX_LINK_FAILED  = 1,
    /*! \brief Not supported contents */
    MP_EVENT_NOT_SUPPORTED      = 2,
    /*! \brief General error from GStreamer */
    MP_EVENT_GST_ERROR          = 3,
    /*! \brief State is changed */
    MP_EVENT_STATE_CHANGED      = 4,
    /*! \brief Subtitle is updated */
    MP_EVENT_SUBTITLE_UPDATED   = 5,
    /*! \brief Unknown error   */
    MP_EVENT_UNKNOWN            = 6
};

/*! \enum NX_GST_RET
 * \brief Describes the return result */
typedef enum {
    /*! \brief On failure */
    NX_GST_RET_ERROR,
    /*! \brief On Success */
    NX_GST_RET_OK
} NX_GST_RET;

/*! \enum NX_URI_TYPE
 * \brief Describes the URI types */
typedef enum
{
    /*! \brief File type */
    URI_TYPE_FILE,
    /*! \brief URL type */
    URI_TYPE_URL
} NX_URI_TYPE;

/*! \enum NX_MEDIA_STATE
 * \brief Describes the media states */
enum NX_MEDIA_STATE
{
    /*! \brief No pending state */
    MP_STATE_VOID_PENDING	= 0,
    /*! \brief Stopped state or initial state */
    MP_STATE_STOPPED		= 1,
    /*! \brief Ready to go to PAUSED state */
    MP_STATE_READY			= 2,
    /*! \brief Paused state */
    MP_STATE_PAUSED 		= 3,
    /*! \brief Playing state */
    MP_STATE_PLAYING		= 4,
};

/*! \struct DSP_RECT
 * \brief Describes the information of display rectangle */
struct DSP_RECT {
    /*! \brief The X-coordinate of the left side of the rectangle */
    int32_t     left;
    /*! \brief The Y-coordinate of the top of the rectangle */
    int32_t     top;
    /*! \brief The X-coordinate of the right side of the rectangle */
    int32_t     right;
    /*! \brief The Y-coordinate of the bottom of the rectangle */
    int32_t     bottom;
};

/*! \def MAX_STREAM_INFO
 * \brief Maximum number of stream information */
#define	MAX_STREAM_INFO		20

/*! \struct _GST_STREAM_INFO
 * \brief Describes the stream information */
struct _GST_STREAM_INFO {
    /*! \brief Total number of audio */
    gint32			iAudioNum;
    /*! \brief Total number of videos */
    gint32			iVideoNum;
    /*! \brief Total number of subtitles */
    gint32			iSubTitleNum;
    /*! \brief Total stream duration  */
    gint64			iDuration;
};
typedef struct _GST_STREAM_INFO	GST_STREAM_INFO;/*! \typedef GST_STREAM_INFO */

/*! \struct GST_MEDIA_INFO
 * \brief Describes the media information */
struct GST_MEDIA_INFO {
    /*! \brief Container type */
    gchar*          container_format;
    /*! \brief Total number of container */
    gint32			n_container;
    /*! \brief Total number of videos */
    gint32			n_video;
    /*! \brief Total number of audio */
    gint32			n_audio;
    /*! \brief Total number of subtitles */
    gint32			n_subtitle;
    /*! \brief If the content is seekable */
    gboolean        isSeekable;
    /*! \brief Total stream duration */
    gint64          iDuration;
    /*! \brief The information for each stream */
    GST_STREAM_INFO	StreamInfo[MAX_STREAM_INFO];

    /*! \brief Video codec type */
    gchar*          video_mime_type;
    /*! \brief Video mpeg version */
    gint32			video_mpegversion;

    struct DSP_RECT        dsp_rect;
    
    /*! \brief Video Width */
    gint32          video_width;
    /*! \brief Video Height */
    gint32          video_height;

    /*! \brief Audio codec type */
    gchar*          audio_mime_type;
    /*! \brief Audio mpeg version */
    gint32			audio_mpegversion;

    /*! \brief Subtitle codec type */
    gchar*			subtitle_codec;

    /*! \brief URI type */
    NX_URI_TYPE		uriType;
};

/*! \enum SUBTITLE_INFO
 * \brief Describes the subtitle information */
struct SUBTITLE_INFO {
    /*! \brief PTS time */
    gint64 startTime;
    /*! \brief startTime + duration */
    gint64 endTime;
    /*! \brief duration time */
    gint64 duration;
    /*! \brief Subtitle texts */
    gchar*	subtitleText;
};

/*! \enum DISPLAY_MODE
 * \brief Describes the display mode */
enum DISPLAY_MODE {
    /*! \brief If only primary LCD is supported */
    DISPLAY_MODE_LCD_ONLY   = 0,
    /*! \brief If only secondary HDMI display is supported */
    DISPLAY_MODE_HDMI_ONLY  = 1,
    /*! \brief If both the primary LCD and the secondary HDMI display are supported */
    DISPLAY_MODE_LCD_HDMI   = 2,
    /*! \brief Unknown */
    DISPLAY_MODE_UNKNOWN    = 3
};

/*! \enum DISPLAY_TYPE
 * \brief Describes the display type */
enum DISPLAY_TYPE {
    /*! \brief If the display type is primary */
    DISPLAY_TYPE_PRIMARY,
    /*! \brief If the display type is secondary */
    DISPLAY_TYPE_SECONDARY
};

#ifdef __cplusplus
extern "C" {
#endif	//	__cplusplus

#ifdef __cplusplus
}
#endif

#endif // __NX_GSTTYPES_H