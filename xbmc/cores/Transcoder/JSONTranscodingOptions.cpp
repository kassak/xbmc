#include "JSONTranscodingOptions.h"

JSONTranscodingOptions::JSONTranscodingOptions(const CVariant &options)
  : TranscodingOptions()
{
  m_sContainerFormat = options["container"].asString();
  m_iWidth = options["width"].asInteger();
  m_iHeight = options["height"].asInteger();
}

JSONTranscodingOptions::~JSONTranscodingOptions()
{

}
