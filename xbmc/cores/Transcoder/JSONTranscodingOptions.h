#pragma once

#include "TranscodingOptions.h"

#include "utils/Variant.h"

class JSONTranscodingOptions : public TranscodingOptions
{
public:

  JSONTranscodingOptions(const CVariant &options);
  virtual ~JSONTranscodingOptions();

};
