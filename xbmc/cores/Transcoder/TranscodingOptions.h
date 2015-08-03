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
  std::string GetStreamingMethod() const;
  int GetWidth() const;
  int GetHeight() const;
  float GetSegmentDuration() const;
  AVPixelFormat GetPixelFormat() const;
  int GetSwsInterpolationMethod() const;

protected:

  std::string m_sContainerFormat;
  std::string m_sStreamingMethod;
  void SetStreamingMethod(std::string streamingMethod);
  int m_iWidth;
  int m_iHeight;
  int m_fSegmentDuration;
  AVPixelFormat m_ePixelFormat;
  int m_iSwsInterpolationMethod;

};