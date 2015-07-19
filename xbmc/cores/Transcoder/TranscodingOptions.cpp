#include "TranscodingOptions.h"

extern "C" {
#include <libswscale/swscale.h>
}

TranscodingOptions::TranscodingOptions()
{
  m_sContainerFormat = "flv";
  m_ePixelFormat = AV_PIX_FMT_YUV420P;
  m_iWidth = 0;
  m_iHeight = 0;
  m_iSwsInterpolationMethod = SWS_BILINEAR;
}

TranscodingOptions::~TranscodingOptions()
{

}

std::string TranscodingOptions::GetFileExtension() const
{
  return m_sContainerFormat;
}

AVPixelFormat TranscodingOptions::GetPixelFormat() const
{
  return m_ePixelFormat;
}

int TranscodingOptions::GetWidth() const
{
  return m_iWidth;
}

int TranscodingOptions::GetHeight() const
{
  return m_iHeight;
}

int TranscodingOptions::GetSwsInterpolationMethod() const
{
  return m_iSwsInterpolationMethod;
}