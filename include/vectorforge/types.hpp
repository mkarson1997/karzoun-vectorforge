#pragma once

#include <cstddef>
#include <cstdint>

namespace vectorforge {

enum class Metric : std::uint8_t {
    L2 = 0,
    Cosine = 1,
};

struct SearchResult {
    std::uint64_t id{};
    float distance{};
};

struct GraphParameters {
    std::size_t dimension{};
    std::size_t max_degree{16};
    std::size_t ef_construction{64};
    Metric metric{Metric::L2};
};

} // namespace vectorforge
