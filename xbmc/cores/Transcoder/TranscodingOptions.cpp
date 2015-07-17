#include "TranscodingOptions.h"

TranscodingOptions::TranscodingOptions()
{
  m_sContainerFormat = "flv";
}

TranscodingOptions::~TranscodingOptions()
{

}

std::string TranscodingOptions::GetFileExtension() const
{
  return m_sContainerFormat;
}