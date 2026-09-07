#include "vectorforge/graph_index.hpp"

#include "vectorforge/distance.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace vectorforge {
namespace {

constexpr std::array<char, 8> kMagic{'K', 'V', 'F', 'G', 'R', 'A', 'P', 'H'};
constexpr std::uint32_t kFormatVersion = 1;
constexpr std::uint64_t kMaxPersistedNodes = 10'000'000ULL;
constexpr std::uint64_t kMaxPersistedDimension = 1'000'000ULL;

template <typename T>
void write_value(std::ostream& stream, const T& value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!stream) {
        throw std::runtime_error("failed to write VectorForge index");
    }
}

template <typename T>
T read_value(std::istream& stream) {
    T value{};
    stream.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!stream) {
        throw std::runtime_error("truncated VectorForge index");
    }
    return value;
}

std::size_t checked_size(std::uint64_t value, std::uint64_t maximum, const char* field) {
    if (value > maximum || value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error(std::string("invalid persisted ") + field);
    }
    return static_cast<std::size_t>(value);
}

} // namespace

GraphIndex::GraphIndex(GraphParameters parameters) : parameters_(parameters) {
    if (parameters_.dimension == 0) {
        throw std::invalid_argument("dimension must be greater than zero");
    }
    if (parameters_.max_degree == 0) {
        throw std::invalid_argument("max_degree must be greater than zero");
    }
    if (parameters_.ef_construction < parameters_.max_degree) {
        throw std::invalid_argument("ef_construction must be >= max_degree");
    }
}

void GraphIndex::validate(std::span<const float> values) const {
    if (values.size() != parameters_.dimension) {
        throw std::invalid_argument("vector dimension does not match index dimension");
    }
}

std::vector<GraphIndex::Candidate> GraphIndex::search_candidates_unlocked(
    std::span<const float> query,
    std::size_t ef) const {
    if (nodes_.empty()) {
        return {};
    }

    ef = std::max<std::size_t>(1, std::min(ef, nodes_.size()));

    const auto min_cmp = [](const Candidate& lhs, const Candidate& rhs) {
        return lhs.distance > rhs.distance;
    };
    const auto max_cmp = [](const Candidate& lhs, const Candidate& rhs) {
        return lhs.distance < rhs.distance;
    };

    std::priority_queue<Candidate, std::vector<Candidate>, decltype(min_cmp)> frontier(min_cmp);
    std::priority_queue<Candidate, std::vector<Candidate>, decltype(max_cmp)> best(max_cmp);
    std::vector<bool> visited(nodes_.size(), false);

    const Candidate entry{distance(query, nodes_.front().values, parameters_.metric), 0};
    frontier.push(entry);
    best.push(entry);
    visited[0] = true;

    while (!frontier.empty()) {
        const Candidate current = frontier.top();
        frontier.pop();

        if (best.size() >= ef && current.distance > best.top().distance) {
            break;
        }

        for (const auto neighbor : nodes_[current.index].neighbors) {
            if (neighbor >= nodes_.size()) {
                throw std::runtime_error("index graph contains invalid neighbor reference");
            }
            if (visited[neighbor]) {
                continue;
            }
            visited[neighbor] = true;

            const Candidate candidate{
                distance(query, nodes_[neighbor].values, parameters_.metric), neighbor};
            if (best.size() < ef || candidate.distance < best.top().distance) {
                frontier.push(candidate);
                best.push(candidate);
                if (best.size() > ef) {
                    best.pop();
                }
            }
        }
    }

    std::vector<Candidate> candidates;
    candidates.reserve(best.size());
    while (!best.empty()) {
        candidates.push_back(best.top());
        best.pop();
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.distance == rhs.distance) {
            return lhs.index < rhs.index;
        }
        return lhs.distance < rhs.distance;
    });
    return candidates;
}

void GraphIndex::prune_neighbors_unlocked(std::size_t node_index) {
    auto& node = nodes_.at(node_index);
    if (node.neighbors.size() <= parameters_.max_degree) {
        return;
    }

    std::sort(node.neighbors.begin(), node.neighbors.end(), [&](std::size_t lhs, std::size_t rhs) {
        const float lhs_distance = distance(node.values, nodes_[lhs].values, parameters_.metric);
        const float rhs_distance = distance(node.values, nodes_[rhs].values, parameters_.metric);
        if (lhs_distance == rhs_distance) {
            return lhs < rhs;
        }
        return lhs_distance < rhs_distance;
    });
    node.neighbors.resize(parameters_.max_degree);
}

void GraphIndex::add(std::uint64_t id, std::span<const float> values) {
    validate(values);
    std::unique_lock lock(mutex_);

    for (const auto& node : nodes_) {
        if (node.id == id) {
            throw std::invalid_argument("duplicate vector id");
        }
    }

    if (nodes_.empty()) {
        nodes_.push_back(Node{id, std::vector<float>(values.begin(), values.end()), {}});
        return;
    }

    auto candidates = search_candidates_unlocked(values, parameters_.ef_construction);
    if (candidates.empty()) {
        throw std::runtime_error("failed to find graph insertion candidates");
    }

    const std::size_t new_index = nodes_.size();
    Node new_node{id, std::vector<float>(values.begin(), values.end()), {}};
    const std::size_t degree = std::min(parameters_.max_degree, candidates.size());
    new_node.neighbors.reserve(degree);
    for (std::size_t i = 0; i < degree; ++i) {
        new_node.neighbors.push_back(candidates[i].index);
    }
    nodes_.push_back(std::move(new_node));

    for (const auto neighbor : nodes_[new_index].neighbors) {
        auto& adjacency = nodes_[neighbor].neighbors;
        if (std::find(adjacency.begin(), adjacency.end(), new_index) == adjacency.end()) {
            adjacency.push_back(new_index);
        }
        prune_neighbors_unlocked(neighbor);
    }
}

std::vector<SearchResult> GraphIndex::search(std::span<const float> query,
                                             std::size_t k,
                                             std::size_t ef_search) const {
    validate(query);
    if (k == 0) {
        return {};
    }

    std::shared_lock lock(mutex_);
    const auto candidates = search_candidates_unlocked(query, std::max(k, ef_search));
    const auto count = std::min(k, candidates.size());

    std::vector<SearchResult> results;
    results.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        results.push_back(SearchResult{nodes_[candidates[i].index].id, candidates[i].distance});
    }
    return results;
}

std::size_t GraphIndex::size() const {
    std::shared_lock lock(mutex_);
    return nodes_.size();
}

void GraphIndex::save(const std::filesystem::path& path) const {
    std::shared_lock lock(mutex_);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("failed to open index file for writing");
    }

    stream.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
    write_value(stream, kFormatVersion);
    write_value(stream, static_cast<std::uint64_t>(parameters_.dimension));
    write_value(stream, static_cast<std::uint64_t>(parameters_.max_degree));
    write_value(stream, static_cast<std::uint64_t>(parameters_.ef_construction));
    write_value(stream, static_cast<std::uint8_t>(parameters_.metric));
    write_value(stream, static_cast<std::uint64_t>(nodes_.size()));

    for (const auto& node : nodes_) {
        write_value(stream, node.id);
        stream.write(reinterpret_cast<const char*>(node.values.data()),
                     static_cast<std::streamsize>(node.values.size() * sizeof(float)));
        if (!stream) {
            throw std::runtime_error("failed to write vector payload");
        }
        write_value(stream, static_cast<std::uint64_t>(node.neighbors.size()));
        for (const auto neighbor : node.neighbors) {
            write_value(stream, static_cast<std::uint64_t>(neighbor));
        }
    }
}

std::unique_ptr<GraphIndex> GraphIndex::load(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("failed to open index file for reading");
    }

    std::array<char, kMagic.size()> magic{};
    stream.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!stream || magic != kMagic) {
        throw std::runtime_error("invalid VectorForge index magic");
    }

    const auto version = read_value<std::uint32_t>(stream);
    if (version != kFormatVersion) {
        throw std::runtime_error("unsupported VectorForge index version");
    }

    GraphParameters parameters;
    parameters.dimension = checked_size(read_value<std::uint64_t>(stream), kMaxPersistedDimension, "dimension");
    parameters.max_degree = checked_size(read_value<std::uint64_t>(stream), kMaxPersistedNodes, "max_degree");
    parameters.ef_construction = checked_size(read_value<std::uint64_t>(stream), kMaxPersistedNodes, "ef_construction");
    const auto raw_metric = read_value<std::uint8_t>(stream);
    if (raw_metric > static_cast<std::uint8_t>(Metric::Cosine)) {
        throw std::runtime_error("invalid persisted distance metric");
    }
    parameters.metric = static_cast<Metric>(raw_metric);

    const std::size_t node_count = checked_size(read_value<std::uint64_t>(stream), kMaxPersistedNodes, "node count");
    auto index = std::make_unique<GraphIndex>(parameters);
    index->nodes_.reserve(node_count);

    std::unordered_set<std::uint64_t> ids;
    ids.reserve(node_count);
    for (std::size_t i = 0; i < node_count; ++i) {
        Node node;
        node.id = read_value<std::uint64_t>(stream);
        if (!ids.insert(node.id).second) {
            throw std::runtime_error("persisted index contains duplicate ids");
        }

        node.values.resize(parameters.dimension);
        stream.read(reinterpret_cast<char*>(node.values.data()),
                    static_cast<std::streamsize>(node.values.size() * sizeof(float)));
        if (!stream) {
            throw std::runtime_error("truncated persisted vector payload");
        }

        const std::size_t neighbor_count = checked_size(
            read_value<std::uint64_t>(stream), parameters.max_degree, "neighbor count");
        node.neighbors.reserve(neighbor_count);
        for (std::size_t n = 0; n < neighbor_count; ++n) {
            const std::size_t neighbor = checked_size(
                read_value<std::uint64_t>(stream), kMaxPersistedNodes, "neighbor index");
            node.neighbors.push_back(neighbor);
        }
        index->nodes_.push_back(std::move(node));
    }

    for (const auto& node : index->nodes_) {
        for (const auto neighbor : node.neighbors) {
            if (neighbor >= index->nodes_.size()) {
                throw std::runtime_error("persisted index contains out-of-range neighbor");
            }
        }
    }

    char trailing{};
    if (stream.read(&trailing, 1)) {
        throw std::runtime_error("persisted index contains unexpected trailing bytes");
    }
    if (!stream.eof()) {
        throw std::runtime_error("failed while validating persisted index boundary");
    }

    return index;
}

} // namespace vectorforge
