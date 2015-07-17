#pragma once

#include <string>

class TranscodingOptions
{
public:

  TranscodingOptions();
  virtual ~TranscodingOptions();

  std::string GetFileExtension() const;

protected:

  std::string m_sContainerFormat;

};