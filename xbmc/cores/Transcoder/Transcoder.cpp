/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://xbmc.org
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

#include "Transcoder.h"

#include <stdio.h>

#include <utils/log.h>

CTranscoder::CTranscoder()
  : CThread(this, "Transcoder")
{
  CLog::Log(LOGDEBUG, "CTranscoder::CTranscoder() was called.\n");
  packet.data = NULL;
  packet.size = 0;
  frame = NULL;
  ifmt_ctx = NULL;
  ofmt_ctx = NULL;
  filter_ctx = NULL;
  sws_video_ctx = NULL;
  m_bFoundVideoStream = false;
  m_bFoundAudioStream = false;
  int m_iVideoWidth = 0;
  int m_iVideoHeight = 0;
  AVPixelFormat m_eVideoPixelFormat = AV_PIX_FMT_NONE;
}

CTranscoder::~CTranscoder()
{
  CLog::Log(LOGDEBUG, "CTranscoder::~CTranscoder() was called.\n");
  if (packet.data)
  {
    av_free_packet(&packet);
  }
  if (frame)
  {
    av_frame_free(&frame);
  }
  if (ifmt_ctx)
  {
    unsigned int i;
    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
      avcodec_close(ifmt_ctx->streams[i]->codec);
      if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && ofmt_ctx->streams[i]->codec)
      {
        avcodec_close(ofmt_ctx->streams[i]->codec);
      }
      if (filter_ctx && filter_ctx[i].filter_graph)
      {
        avfilter_graph_free(&filter_ctx[i].filter_graph);
      }
    }
    avformat_close_input(&ifmt_ctx);
  }
  if (ofmt_ctx)
  {
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
      avio_closep(&ofmt_ctx->pb);
    }
  avformat_free_context(ofmt_ctx);
  }
  if (filter_ctx)
  {
    av_free(filter_ctx);
  }
  if (sws_video_ctx)
  {
    sws_freeContext(sws_video_ctx);
  }
}


int CTranscoder::InitSwsContext()
{
  sws_video_ctx = sws_getContext(m_iVideoWidth,
    m_iVideoHeight,
    m_eVideoPixelFormat,
    m_TransOpts.GetWidth(),
    m_TransOpts.GetHeight(),
    m_TransOpts.GetPixelFormat(),
    m_TransOpts.GetSwsInterpolationMethod(),
    NULL, NULL, NULL);

  int ret = 0;
  if (sws_video_ctx == NULL)
  {
    ret = AVERROR(ENOMEM);
  }
  return ret;
}

int CTranscoder::SwsScaleVideo(const AVFrame *src_frame, AVFrame **scaled_frame)
{
  int ret;

  // Allocate a new frame
  *scaled_frame = av_frame_alloc();
  if (*scaled_frame == 0)
  {
    CLog::Log(LOGERROR, "CTranscoder::SwsScaleVideo(): Could not allocate frame.");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  // Allocate space for raw data
  uint8_t *raw_data = NULL;
  int numBytes = avpicture_get_size(m_TransOpts.GetPixelFormat(), m_TransOpts.GetWidth(), m_TransOpts.GetHeight());
  raw_data = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  if (raw_data == 0)
  {
    CLog::Log(LOGERROR, "CTranscoder::SwsScaleVideo(): Could not allocate raw data.");
    ret = AVERROR(ENOMEM);
    return ret;
  }
  if ((ret = avpicture_fill((AVPicture *)*scaled_frame, raw_data, m_TransOpts.GetPixelFormat(),
    m_TransOpts.GetWidth(), m_TransOpts.GetHeight())) < 0)
  {
    CLog::Log(LOGERROR, "CTranscoder::SwsScaleVideo(): Could not set up the picture fields.");
    return ret;
  }
  sws_scale(sws_video_ctx, (uint8_t const * const *)src_frame->data, src_frame->linesize,
    0, m_iVideoHeight, (*scaled_frame)->data, (*scaled_frame)->linesize);
  // TODO: Find out which of the following properties need to be set at all
  (*scaled_frame)->width = m_TransOpts.GetWidth();
  (*scaled_frame)->height = m_TransOpts.GetHeight();
  (*scaled_frame)->format = m_TransOpts.GetPixelFormat();
  (*scaled_frame)->sample_aspect_ratio = src_frame->sample_aspect_ratio;
  (*scaled_frame)->pts = src_frame->pts;
  (*scaled_frame)->pkt_pts = src_frame->pkt_pts;
  (*scaled_frame)->pkt_dts = src_frame->pkt_dts;
  (*scaled_frame)->nb_samples = src_frame->nb_samples;
  (*scaled_frame)->key_frame = src_frame->key_frame;
  (*scaled_frame)->pict_type = src_frame->pict_type;
  (*scaled_frame)->coded_picture_number = src_frame->coded_picture_number;

  return 0;
}

void CTranscoder::LogError(int errnum)
{
	char* buf = (char*) malloc(AV_ERROR_MAX_STRING_SIZE * sizeof(char));
	CLog::Log(LOGERROR, "Error occurred: %s\n", av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, errnum));
	free(buf);
}

void CTranscoder::ResetAVDictionary(AVDictionary **dict)
{
  if (*dict)
  {
    av_free(*dict);
    *dict = NULL;
  }
}

int CTranscoder::OpenInputFile(const char *filename)
{
	int ret;
	ifmt_ctx = NULL;

	if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0)
  {
    CLog::Log(LOGERROR, "CTranscoder::OpenInputFile: Cannot open input file '%s'\n", filename);
		return ret;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
  {
    CLog::Log(LOGERROR, "CTranscoder::OpenInputFile: Cannot find stream information\n");
		return ret;
	}

	unsigned int i;
	for (i = 0; i < ifmt_ctx->nb_streams; i++)
  {
    AVStream *stream = ifmt_ctx->streams[i];
    AVCodecContext *codec_ctx = stream->codec;
    AVMediaType codec_type = codec_ctx->codec_type;
		/* Reencode video & audio and remux subtitles etc. */
		if (codec_type == AVMEDIA_TYPE_VIDEO	|| codec_type == AVMEDIA_TYPE_AUDIO)
    {
			/* Open decoder */
      AVCodecID codec_id = codec_ctx->codec_id;
      if ((ret = avcodec_open2(codec_ctx, avcodec_find_decoder(codec_id), NULL)) < 0)
      {
        CLog::Log(LOGERROR, "Failed to open decoder for stream #%u\n", i);
				return ret;
			}
      if (!m_bFoundVideoStream && codec_type == AVMEDIA_TYPE_VIDEO)
      {
        m_bFoundVideoStream = true;
        m_iVideoWidth = codec_ctx->width;
        m_iVideoHeight = codec_ctx->height;
        m_eVideoPixelFormat = codec_ctx->pix_fmt;
      }
      if (!m_bFoundAudioStream && codec_type == AVMEDIA_TYPE_AUDIO)
      {
        m_bFoundAudioStream = true;
      }
		}
	}

	av_dump_format(ifmt_ctx, 0, filename, 0);
	return 0;
}

int CTranscoder::OpenOutputFile(const char *filename)
{
	ofmt_ctx = NULL;
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
	if (!ofmt_ctx)
  {
    CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile: Could not create output context\n");
		return AVERROR_UNKNOWN;
	}

	AVStream *in_stream;
	AVStream *out_stream;
  AVCodecContext *dec_ctx;
  AVCodecContext *enc_ctx;
	AVCodec *encoder;

	AVDictionary *dict = NULL;
	int ret;
	unsigned int i;
  for (i = 0; i < ifmt_ctx->nb_streams; i++)
  {
    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream) {
      CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile: Failed allocating output stream\n");
      return AVERROR_UNKNOWN;
    }

    in_stream = ifmt_ctx->streams[i];
    dec_ctx = in_stream->codec;
    enc_ctx = out_stream->codec;

    AVMediaType codec_type = dec_ctx->codec_type;
    if (codec_type == AVMEDIA_TYPE_VIDEO)
    {
      // Use H.264 for video encoding
      encoder = avcodec_find_encoder(AV_CODEC_ID_H264);

      if (!encoder)
      {
        CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile: Neccessary video encoder not found\n");
        return AVERROR_INVALIDDATA;
      }

      ResetAVDictionary(&dict);
      av_dict_set(&dict, "preset", "slow", 0);
      av_dict_set(&dict, "vprofile", "high", 0);

      av_opt_set(enc_ctx->priv_data, "profile", "high", AV_OPT_SEARCH_CHILDREN);

      enc_ctx->profile = FF_PROFILE_H264_HIGH;
      enc_ctx->height = m_TransOpts.GetHeight();
      enc_ctx->width = m_TransOpts.GetWidth();
      enc_ctx->pix_fmt = m_TransOpts.GetPixelFormat();
      AVRational sar; sar.num = 1; sar.den = 1;
      enc_ctx->sample_aspect_ratio = sar;
      CLog::Log(LOGDEBUG, "CTranscoder::OpenOutputFile: Video framerate: %u/%u", dec_ctx->framerate.num, dec_ctx->framerate.den);
      enc_ctx->time_base = dec_ctx->time_base;
      enc_ctx->max_b_frames = 0;
      enc_ctx->bit_rate = 500 * 1000;
      enc_ctx->bit_rate_tolerance = 4 * 1000 * 1000;
      enc_ctx->rc_max_rate = 500 * 1000;
      enc_ctx->rc_min_rate = 0;
      enc_ctx->rc_buffer_size = 1 * 1000 * 1000;
      // TODO: Some of the following settings are needed for a correctly working encoder.
      // Find out which one or which combination of them!
      enc_ctx->flags = 2143289344;
      enc_ctx->gop_size = -1;
      enc_ctx->b_frame_strategy = -1;
      enc_ctx->coder_type = -1;
      enc_ctx->me_method = -1;
      enc_ctx->me_subpel_quality = -1;
      enc_ctx->me_cmp = -1;
      enc_ctx->me_range = -1;
      enc_ctx->scenechange_threshold = -1;
      enc_ctx->i_quant_factor = -1;
      enc_ctx->qcompress = -1;
      enc_ctx->qmin = -1;
      enc_ctx->qmax = -1;
      enc_ctx->max_qdiff = -1;

      if ((ret = avcodec_open2(enc_ctx, encoder, &dict)) < 0)
      {
        CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile(): Cannot open video encoder for stream #%u\n", i);
        return ret;
      }
    }
    else if (codec_type == AVMEDIA_TYPE_AUDIO)
    {
      // Use AAC for audio encoding
      encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);

      if (!encoder)
      {
        CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile(): Neccessary audio encoder not found\n");
        return AVERROR_INVALIDDATA;
      }

      enc_ctx->sample_rate = dec_ctx->sample_rate;
      enc_ctx->channel_layout = dec_ctx->channel_layout;
      enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
      enc_ctx->sample_fmt = encoder->sample_fmts[0];
      enc_ctx->time_base = dec_ctx->time_base;
      enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

      if ((ret = avcodec_open2(enc_ctx, encoder, NULL)) < 0)
      {
        CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile(): Cannot open audio encoder for stream #%u\n", i);
        return ret;
      }
    }
		else if (codec_type == AVMEDIA_TYPE_UNKNOWN)
    {
      CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile(): Elementary stream #%d is of unknown type, cannot proceed\n", i);
			return AVERROR_INVALIDDATA;
		}
		else
    {
			/* Simply remux other streams */
			ret = avcodec_copy_context(ofmt_ctx->streams[i]->codec,
				ifmt_ctx->streams[i]->codec);
			if (ret < 0) {
        CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile(): Copying stream context failed\n");
				return ret;
			}
		}

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
      enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
	}
	av_dump_format(ofmt_ctx, 0, filename, 1);

	if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
  {
    if ((ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE)) < 0)
    {
      CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile(): Could not open output file '%s'", filename);
			return ret;
		}
	}

	/* init muxer, write output file header */
  ResetAVDictionary(&dict);
	av_dict_set(&dict, "movflags", "faststart", 0);
  if ((ret = avformat_write_header(ofmt_ctx, &dict)) < 0)
  {
    CLog::Log(LOGERROR, "CTranscoder::OpenOutputFile(): Error occurred when opening output file\n");
		return ret;
	}

	return 0;
}

int CTranscoder::InitFilter(FilteringContext* fctx, AVCodecContext *dec_ctx,
	AVCodecContext *enc_ctx, const char *filter_spec)
{
	char args[512];
	int ret = 0;
	AVFilter *buffersrc = NULL;
	AVFilter *buffersink = NULL;
	AVFilterContext *buffersrc_ctx = NULL;
	AVFilterContext *buffersink_ctx = NULL;
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs = avfilter_inout_alloc();
	AVFilterGraph *filter_graph = avfilter_graph_alloc();

	if (!outputs || !inputs || !filter_graph)
  {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
  {
		buffersrc = avfilter_get_by_name("buffer");
		buffersink = avfilter_get_by_name("buffersink");
		if (!buffersrc || !buffersink) 
    {
      CLog::Log(LOGERROR, "filtering source or sink element not found\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		_snprintf(args, sizeof(args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      m_TransOpts.GetWidth(), m_TransOpts.GetHeight(), m_TransOpts.GetPixelFormat(),
			dec_ctx->time_base.num, dec_ctx->time_base.den,
			dec_ctx->sample_aspect_ratio.num,
			dec_ctx->sample_aspect_ratio.den);

		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
			args, NULL, filter_graph);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot create buffer source\n");
			goto end;
		}

		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot create buffer sink\n");
			goto end;
		}

		ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
			(uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot set output pixel format\n");
			goto end;
		}
	}
	else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		buffersrc = avfilter_get_by_name("abuffer");
		buffersink = avfilter_get_by_name("abuffersink");
		if (!buffersrc || !buffersink) {
      CLog::Log(LOGERROR, "filtering source or sink element not found\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		if (!dec_ctx->channel_layout)
			dec_ctx->channel_layout =
			av_get_default_channel_layout(dec_ctx->channels);
		_snprintf(args, sizeof(args),
			"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
			dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
			av_get_sample_fmt_name(dec_ctx->sample_fmt),
			dec_ctx->channel_layout);
		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
			args, NULL, filter_graph);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot create audio buffer source\n");
			goto end;
		}

		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot create audio buffer sink\n");
			goto end;
		}

		ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
			(uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot set output sample format\n");
			goto end;
		}

		ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
			(uint8_t*)&enc_ctx->channel_layout,
			sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot set output channel layout\n");
			goto end;
		}

		ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
			(uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
      CLog::Log(LOGERROR, "Cannot set output sample rate\n");
			goto end;
		}
	}
	else {
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	/* Endpoints for the filter graph. */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	if (!outputs->name || !inputs->name) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
		&inputs, &outputs, NULL)) < 0)
		goto end;

	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;

	/* Fill FilteringContext */
	fctx->buffersrc_ctx = buffersrc_ctx;
	fctx->buffersink_ctx = buffersink_ctx;
	fctx->filter_graph = filter_graph;

end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);

	return ret;
}

int CTranscoder::InitFilters(void)
{
	const char *filter_spec;
	unsigned int i;
	int ret;
	filter_ctx = (FilteringContext*) av_malloc_array(ifmt_ctx->nb_streams, sizeof(*filter_ctx));
	if (!filter_ctx)
		return AVERROR(ENOMEM);

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		filter_ctx[i].buffersrc_ctx = NULL;
		filter_ctx[i].buffersink_ctx = NULL;
		filter_ctx[i].filter_graph = NULL;
		if (!(ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
			|| ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO))
			continue;


		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			filter_spec = "null"; /* passthrough (dummy) filter for video */
		else
			filter_spec = "anull"; /* passthrough (dummy) filter for audio */
		ret = InitFilter(&filter_ctx[i], ifmt_ctx->streams[i]->codec,
			ofmt_ctx->streams[i]->codec, filter_spec);
		if (ret)
			return ret;
	}
	return 0;
}

int CTranscoder::EncodeWriteFrame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame) {
	int ret;
	int got_frame_local;
	AVPacket enc_pkt;
	int(*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
		(ifmt_ctx->streams[stream_index]->codec->codec_type ==
		AVMEDIA_TYPE_VIDEO) ? avcodec_encode_video2 : avcodec_encode_audio2;

	if (!got_frame)
		got_frame = &got_frame_local;

  //CLog::Log(LOGINFO, "Encoding frame\n");
	/* encode filtered frame */
	enc_pkt.data = NULL;
	enc_pkt.size = 0;
	av_init_packet(&enc_pkt);
	ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt,
		filt_frame, got_frame);
	av_frame_free(&filt_frame);
	if (ret < 0)
		return ret;
	if (!(*got_frame))
		return 0;

	/* prepare packet for muxing */
	enc_pkt.stream_index = stream_index;
	av_packet_rescale_ts(&enc_pkt,
		ofmt_ctx->streams[stream_index]->codec->time_base,
		ofmt_ctx->streams[stream_index]->time_base);

  //CLog::Log(LOGDEBUG, "Muxing frame\n");
	/* mux encoded frame */
	ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
	return ret;
}

int CTranscoder::FilterEncodeWriteFrame(AVFrame *frame, unsigned int stream_index)
{
	int ret;
	AVFrame *filt_frame;

  //CLog::Log(LOGINFO, "Pushing decoded frame to filters\n");
	/* push the decoded frame into the filtergraph */
	ret = av_buffersrc_add_frame_flags(filter_ctx[stream_index].buffersrc_ctx,
		frame, 0);
	if (ret < 0) {
    CLog::Log(LOGERROR, "Error while feeding the filtergraph\n");
		return ret;
	}

	/* pull filtered frames from the filtergraph */
	while (1) {
		filt_frame = av_frame_alloc();
		if (!filt_frame) {
			ret = AVERROR(ENOMEM);
			break;
		}
    //CLog::Log(LOGINFO, "Pulling filtered frame from filters\n");
		ret = av_buffersink_get_frame(filter_ctx[stream_index].buffersink_ctx,
			filt_frame);
		if (ret < 0) {
			/* if no more frames for output - returns AVERROR(EAGAIN)
			* if flushed and no more frames for output - returns AVERROR_EOF
			* rewrite retcode to 0 to show it as normal procedure completion
			*/
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				ret = 0;
			av_frame_free(&filt_frame);
			break;
		}

		filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
		ret = EncodeWriteFrame(filt_frame, stream_index, NULL);
		if (ret < 0)
			break;
	}

	return ret;
}

int CTranscoder::FlushEncoder(unsigned int stream_index)
{
	int ret;
	int got_frame;

	if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;

	while (1) {
    //CLog::Log(LOGINFO, "Flushing stream #%u encoder\n", stream_index);
    ret = EncodeWriteFrame(NULL, stream_index, &got_frame);
		if (ret < 0)
			break;
		if (!got_frame)
			return 0;
	}
	return ret;
}

int CTranscoder::Transcode(std::string path)
{
  this->path = path;
  bool autoDelete = true;
  this->Create(autoDelete);
  return 1;
}

std::string CTranscoder::TranscodePath(const std::string &path) const
{
  return path.substr(0, path.find_last_of('.'))
    + std::string("-transcoded.") + m_TransOpts.GetFileExtension();
}

void CTranscoder::Run()
{
  CLog::Log(LOGDEBUG, "CTranscoder::Run() was called.");
  
  if (path.empty()) {
    CLog::Log(LOGERROR, "CTranscoder::Run(): Path to input file must not be empty.\n");
    return;
  }

  std::string pathOut = TranscodePath(path);
  CLog::Log(LOGDEBUG, "CTranscoder::Run(): Using input file %s and output file %s", path.c_str(), pathOut.c_str());

  avfilter_register_all();
  avcodec_register_all();
  av_register_all();

  int ret;
  if ((ret = OpenInputFile(path.c_str())) < 0)
  {
    goto end;
  }
  if ((ret = OpenOutputFile(pathOut.c_str())) < 0)
  {
    goto end;
  }
  if ((ret = InitFilters()) < 0)
  {
    goto end;
  }
  if ((ret = InitSwsContext()) < 0)
  {
    goto end;
  }

  unsigned int stream_index;
  unsigned int i;
  int got_frame;
  int(*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);

  while (1)
  {
    if (m_bStop)
    {
      CLog::Log(LOGDEBUG, "CTranscoder::Run(): Transcoder asked to stop!\n");
    }
    if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
      break;
    stream_index = packet.stream_index;
    AVStream *stream = ifmt_ctx->streams[stream_index];
    AVCodecContext *codec_ctx = stream->codec;
    type = codec_ctx->codec_type;
    //CLog::Log(LOGDEBUG, "CTranscoder::Run(): Demuxer gave frame of stream_index %u\n", stream_index);

    if (filter_ctx[stream_index].filter_graph)
    {
      //CLog::Log(LOGDEBUG, "CTranscoder::Run(): Going to reencode and filter the frame\n"); 
      if ((frame = av_frame_alloc()) == NULL)
      {
        ret = AVERROR(ENOMEM);
        break;
      }
      av_packet_rescale_ts(&packet, stream->time_base, codec_ctx->time_base);
      if (type == AVMEDIA_TYPE_VIDEO)
      {
        ret = avcodec_decode_video2(codec_ctx, frame, &got_frame, &packet);
      }
      else if (type == AVMEDIA_TYPE_AUDIO)
      {
        ret = avcodec_decode_audio4(codec_ctx, frame, &got_frame, &packet);
      }
      else
      {
        CLog::Log(LOGERROR, "CTranscoder::Run(): Got packet of unexpected media type.");
        ret = -1;
      }
      if (ret < 0) {
        av_frame_free(&frame);
        CLog::Log(LOGERROR, "CTranscoder::Run(): Decoding failed\n");
        break;
      }

      if (got_frame)
      {
        frame->pts = av_frame_get_best_effort_timestamp(frame);
        if (type == AVMEDIA_TYPE_VIDEO)
        {
          // Rescale the video frame
          AVFrame *scaledFrame;
          if ((ret = SwsScaleVideo(frame, &scaledFrame)) == 0)
          {
            ret = FilterEncodeWriteFrame(scaledFrame, stream_index);
            av_frame_free(&scaledFrame);
            av_frame_free(&frame);
          }
          else
          {
            CLog::Log(LOGERROR, "CTranscoder::Run(): Scaling of video frame failed.");
            if (scaledFrame)
            {
              av_frame_free(&scaledFrame);
            }
            av_frame_free(&frame);
          }
        }
        else
        {
          ret = FilterEncodeWriteFrame(frame, stream_index);
          av_frame_free(&frame);
        }
        if (ret < 0)
          goto end;
      }
      else
      {
        av_frame_free(&frame);
      }
    }
    else
    {
      /* remux this frame without reencoding */
      av_packet_rescale_ts(&packet,
        ifmt_ctx->streams[stream_index]->time_base,
        ofmt_ctx->streams[stream_index]->time_base);

      ret = av_interleaved_write_frame(ofmt_ctx, &packet);
      if (ret < 0)
        goto end;
    }
    av_free_packet(&packet);
  }

  /* flush filters and encoders */
  for (i = 0; i < ifmt_ctx->nb_streams; i++)
  {
    /* flush filter */
    if (!filter_ctx[i].filter_graph)
    {
      continue;
    }
    if ((ret = FilterEncodeWriteFrame(NULL, i)) < 0)
    {
      CLog::Log(LOGERROR, "CTranscoder::Run(): Flushing filter failed\n");
      goto end;
    }

    /* flush encoder */
    if ((ret = FlushEncoder(i)) < 0)
    {
      CLog::Log(LOGERROR, "CTranscoder::Run(): Flushing encoder failed\n");
      goto end;
    }
  }

  av_write_trailer(ofmt_ctx);

end:
  if (ret < 0)
  {
    LogError(ret);
  }
}

void CTranscoder::OnStartup()
{
  CLog::Log(LOGDEBUG, "CTranscoder::onStartup() was called.\n");
}

void CTranscoder::OnExit()
{
  CLog::Log(LOGDEBUG, "CTranscoder::onExit() was called.\n");
}

void CTranscoder::SetTranscodingOptions(TranscodingOptions transOpts)
{
  m_TransOpts = transOpts;
}