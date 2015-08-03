#pragma once

#include <string>

#include "TranscodingOptions.h"
#include <threads/Thread.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

class CTranscoder : CThread, public IRunnable
{

public:

  CTranscoder();
  virtual ~CTranscoder();

  int Transcode(std::string path);
  std::string TranscodePath(const std::string &path) const;
  std::string TranscodePlaylistPath(const std::string &path) const;
  std::string TranscodeSegmentPath(const std::string &path, int segment = 0) const;

  void SetTranscodingOptions(TranscodingOptions transOpts);

protected:

  virtual void OnStartup();
  virtual void OnExit();

private:

  TranscodingOptions m_TransOpts;

  typedef struct FilteringContext {
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
  } FilteringContext;

  int OpenInputFile(const char *filename);
  int CloseInputFile();
  int OpenOutputFile(const char *filename);
  int CloseOutputFile();
  int InitFilter(FilteringContext* fctx, AVCodecContext *dec_ctx,
    AVCodecContext *enc_ctx, const char *filter_spec);
  int InitFilters(void);
  int EncodeWriteFrame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame);
  int FilterEncodeWriteFrame(AVFrame *frame, unsigned int stream_index);
  int FlushEncoder(unsigned int stream_index);
  int FlushFiltersAndEncoders();
  int InitSwsContext();
  int CloseSwsContext();
  int SwsScaleVideo(const AVFrame *src_frame, AVFrame **scaled_frame);

  int HLS_CreatePlaylist(const char* filename);
  int m_iHLS_Segment;
  int m_iDuration;
  int ShouldStartNewSegment(int64_t time_stamp, const AVRational& time_base);

  static void LogError(int errnum);
  static void ResetAVDictionary(AVDictionary **dict);

  virtual void Run();

  std::string path;

  AVPacket packet;

  AVFrame *frame;

  enum AVMediaType type;

  AVFormatContext *ifmt_ctx;
  AVFormatContext *ofmt_ctx;

  FilteringContext *filter_ctx;

  bool m_bFoundVideoStream;
  bool m_bFoundAudioStream;
  int m_iVideoWidth;
  int m_iVideoHeight;
  AVPixelFormat m_eVideoPixelFormat;
  SwsContext *sws_video_ctx;

};
