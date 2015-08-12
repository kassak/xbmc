#include "JSONTranscodingOptions.h"

JSONTranscodingOptions::JSONTranscodingOptions(const CVariant &options)
  : TranscodingOptions()
{
  CVariant containerFormat = options["container"];
  if (!containerFormat.isNull() && containerFormat.asString().compare("") != 0)
    m_sContainerFormat = containerFormat.asString();
  CVariant streamingMethod = options["streaming"];
  if (!streamingMethod.isNull() && streamingMethod.asString().compare("") != 0)
  {
    // Check if a valid streaming method was provided
    if (streamingMethod.asString().compare("http") == 0
      || streamingMethod.asString().compare("hls") == 0)
    {
      SetStreamingMethod(streamingMethod.asString());
    }
  }
  CVariant width = options["width"];
  if (!width.isNull() && width.asInteger() != 0)
    m_iWidth = width.asInteger();
  CVariant height = options["height"];
  if (!height.isNull() && height.asInteger() != 0)
  m_iHeight = height.asInteger();
  CVariant videoBitrate = options["videobitrate"];
  if (!videoBitrate.isNull() && videoBitrate.asInteger() != 0)
    m_iVideoBitrate = videoBitrate.asInteger();
  CVariant segmentDuration = options["segmentduration"];
  if (!segmentDuration.isNull() && segmentDuration.asInteger() != 0)
    m_fSegmentDuration = segmentDuration.asInteger();
}

JSONTranscodingOptions::~JSONTranscodingOptions()
{

}
