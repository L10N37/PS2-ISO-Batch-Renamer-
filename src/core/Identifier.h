#pragma once

#include "core/ByteReader.h"
#include "core/Types.h"

namespace ps2br {

Identification identifyGame(ByteReader& reader);

} // namespace ps2br
