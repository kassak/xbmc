#include "JSONTranscodingOptions.h"

JSONTranscodingOptions::JSONTranscodingOptions(const CVariant &options)
  : TranscodingOptions()
{
  m_sContainerFormat = options["container"].asString();
}

JSONTranscodingOptions::~JSONTranscodingOptions()
{

}
