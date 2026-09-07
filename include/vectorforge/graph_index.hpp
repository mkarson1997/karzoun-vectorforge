#pragma once

#include "vectorforge/types.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <span>
#include <vector>

namespace vectorforge {

// VectorForge v0.1 implements a single-layer navigable proximity graph.
// It is an ANN foundation, not a full hierarchical HNSW implementation.
class GraphIndex {
public:
    explicit GraphIndex(GraphParameters parameters);

    GraphIndex(const GraphIndex&) = delete;
    GraphIndex& operator=(const GraphIndex&) = delete;
    GraphIndex(GraphIndex&&) = delete;
    GraphIndex& operator=(GraphIndex&&) = delete;

    void add(std::uint64_t id, std::span<const float> values);

    [[nodiscard]] std::vector<SearchResult> search(std::span<const float> query,
                                                   std::size_t k,
                                                   std::size_t ef_search = 64) const;

    void save(const std::filesystem::path& path) const;
    [[nodiscard]] static std::unique_ptr<GraphIndex> load(const std::filesystem::path& path);

    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] const GraphParameters& parameters() const noexcept { return parameters_; }

private:
    struct Node {
        std::uint64_t id{};
        std::vector<float> values;
        std::vector<std::size_t> neighbors;
    };

    struct Candidate {
        float distance{};
        std::size_t index{};
    };

    void validate(std::span<const float> values) const;
    [[nodiscard]] std::vector<Candidate> search_candidates_unlocked(
        std::span<const float> query,
        std::size_t ef) const;
    void prune_neighbors_unlocked(std::size_t node_index);

    GraphParameters parameters_;
    mutable std::shared_mutex mutex_;
    std::vector<Node> nodes_;
};

} // namespace vectorforge
