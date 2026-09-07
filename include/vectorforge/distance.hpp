#pragma once

#include "vectorforge/types.hpp"

#include <span>

namespace vectorforge {

[[nodiscard]] float distance(std::span<const float> a,
                             std::span<const float> b,
                             Metric metric);

} // namespace vectorforge
