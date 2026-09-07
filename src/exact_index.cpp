#include "vectorforge/exact_index.hpp"

#include "vectorforge/distance.hpp"

#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace vectorforge {

ExactIndex::ExactIndex(std::size_t dimension, Metric metric)
    : dimension_(dimension), metric_(metric) {
    if (dimension_ == 0) {
        throw std::invalid_argument("dimension must be greater than zero");
    }
}

void ExactIndex::validate(std::span<const float> values) const {
    if (values.size() != dimension_) {
        throw std::invalid_argument("vector dimension does not match index dimension");
    }
}

void ExactIndex::add(std::uint64_t id, std::span<const float> values) {
    validate(values);
    std::unique_lock lock(mutex_);
    entries_.push_back(Entry{id, std::vector<float>(values.begin(), values.end())});
}

std::vector<SearchResult> ExactIndex::search(std::span<const float> query,
                                             std::size_t k) const {
    validate(query);
    if (k == 0) {
        return {};
    }

    std::shared_lock lock(mutex_);
    std::vector<SearchResult> results;
    results.reserve(entries_.size());
    for (const auto& entry : entries_) {
        results.push_back(SearchResult{entry.id, distance(query, entry.values, metric_)});
    }

    const auto count = std::min(k, results.size());
    std::partial_sort(results.begin(), results.begin() + static_cast<std::ptrdiff_t>(count),
                      results.end(), [](const auto& lhs, const auto& rhs) {
                          if (lhs.distance == rhs.distance) {
                              return lhs.id < rhs.id;
                          }
                          return lhs.distance < rhs.distance;
                      });
    results.resize(count);
    return results;
}

std::size_t ExactIndex::size() const {
    std::shared_lock lock(mutex_);
    return entries_.size();
}

} // namespace vectorforge
