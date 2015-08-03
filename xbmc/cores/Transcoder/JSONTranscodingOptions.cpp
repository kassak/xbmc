#include "JSONTranscodingOptions.h"

JSONTranscodingOptions::JSONTranscodingOptions(const CVariant &options)
  : TranscodingOptions()
{
  m_sContainerFormat = options["container"].asString();
  CVariant streamingMethod = options["streaming"];
  if (!streamingMethod.isNull())
    SetStreamingMethod(streamingMethod.asString());
  m_iWidth = options["width"].asInteger();
  m_iHeight = options["height"].asInteger();
  CVariant segmentDuration = options["segmentduration"];
  if (!segmentDuration.isNull())
    m_fSegmentDuration = segmentDuration.asInteger();
}

JSONTranscodingOptions::~JSONTranscodingOptions()
{

}
