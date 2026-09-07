#pragma once

#include "vectorforge/types.hpp"

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <span>
#include <vector>

namespace vectorforge {

class ExactIndex {
public:
    ExactIndex(std::size_t dimension, Metric metric);

    void add(std::uint64_t id, std::span<const float> values);

    [[nodiscard]] std::vector<SearchResult> search(std::span<const float> query,
                                                   std::size_t k) const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t dimension() const noexcept { return dimension_; }

private:
    struct Entry {
        std::uint64_t id{};
        std::vector<float> values;
    };

    void validate(std::span<const float> values) const;

    std::size_t dimension_{};
    Metric metric_{};
    mutable std::shared_mutex mutex_;
    std::vector<Entry> entries_;
};

} // namespace vectorforge
