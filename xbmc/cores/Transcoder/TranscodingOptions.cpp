#include "TranscodingOptions.h"

#include "utils/log.h"

extern "C" {
#include <libswscale/swscale.h>
}

TranscodingOptions::TranscodingOptions()
{
  m_sContainerFormat = "mp4";
  SetStreamingMethod("http");
  m_iWidth = 0;
  m_iHeight = 0;
  m_iVideoBitrate = 500 * 1000;
  m_fSegmentDuration = 10;
  m_ePixelFormat = AV_PIX_FMT_YUV420P;
  m_iSwsInterpolationMethod = SWS_BILINEAR;
}

TranscodingOptions::~TranscodingOptions()
{

}

std::string TranscodingOptions::GetFileExtension() const
{
  return m_sContainerFormat;
}

std::string TranscodingOptions::GetStreamingMethod() const
{
  return m_sStreamingMethod;
}

void TranscodingOptions::SetStreamingMethod(std::string streamingMethod)
{
  m_sStreamingMethod = streamingMethod;
  // In case of HLS as streaming method an MPEG transport stream is required
  if (m_sStreamingMethod.compare("hls") == 0)
  {
    if (m_sContainerFormat.compare("ts") != 0)
    {
      CLog::Log(LOGWARNING, "TranscodingOptions::SetStreamingMethod(): HTTP Live Streaming doesn't support the chosen container format. Using .ts instead");
      m_sContainerFormat = "ts";
    }
  }
}

int TranscodingOptions::GetWidth() const
{
  return m_iWidth;
}

int TranscodingOptions::GetHeight() const
{
  return m_iHeight;
}

int TranscodingOptions::GetVideoBitrate() const
{
  return m_iVideoBitrate;
}

int TranscodingOptions::GetSegmentDuration() const
{
  return m_fSegmentDuration;
}

AVPixelFormat TranscodingOptions::GetPixelFormat() const
{
  return m_ePixelFormat;
}

int TranscodingOptions::GetSwsInterpolationMethod() const
{
  return m_iSwsInterpolationMethod;
}
