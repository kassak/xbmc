#include "JSONTranscodingOptions.h"

JSONTranscodingOptions::JSONTranscodingOptions(const CVariant &options)
  : TranscodingOptions()
{
  m_sContainerFormat = options["container"].asString();
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
  m_iWidth = options["width"].asInteger();
  m_iHeight = options["height"].asInteger();
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
