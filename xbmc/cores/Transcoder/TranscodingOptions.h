#pragma once

#include <string>


extern "C" {
#include <libavutil/pixdesc.h>
}

class TranscodingOptions
{
public:

  TranscodingOptions();
  virtual ~TranscodingOptions();

  std::string GetFileExtension() const;
  AVPixelFormat GetPixelFormat() const;
  int GetWidth() const;
  int GetHeight() const;
  int GetSwsInterpolationMethod() const;

protected:

  std::string m_sContainerFormat;
  AVPixelFormat m_ePixelFormat;
  int m_iWidth;
  int m_iHeight;
  int m_iSwsInterpolationMethod;

};