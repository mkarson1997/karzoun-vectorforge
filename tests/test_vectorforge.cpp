#include "vectorforge/distance.hpp"
#include "vectorforge/exact_index.hpp"
#include "vectorforge/graph_index.hpp"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function>
void require_throws(Function&& function, const std::string& message) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

std::vector<float> make_vector(std::mt19937& rng, std::size_t dimension) {
    std::normal_distribution<float> distribution(0.0F, 1.0F);
    std::vector<float> values(dimension);
    for (auto& value : values) {
        value = distribution(rng);
    }
    return values;
}

double recall_at_k(const std::vector<vectorforge::SearchResult>& exact,
                   const std::vector<vectorforge::SearchResult>& approximate) {
    std::unordered_set<std::uint64_t> expected;
    for (const auto& item : exact) {
        expected.insert(item.id);
    }
    std::size_t hits = 0;
    for (const auto& item : approximate) {
        hits += expected.contains(item.id) ? 1U : 0U;
    }
    return static_cast<double>(hits) / static_cast<double>(exact.size());
}

void test_distance_metrics() {
    const std::vector<float> a{1.0F, 0.0F};
    const std::vector<float> b{0.0F, 1.0F};
    require(std::abs(vectorforge::distance(a, b, vectorforge::Metric::L2) - 2.0F) < 1e-6F,
            "L2 distance mismatch");
    require(std::abs(vectorforge::distance(a, b, vectorforge::Metric::Cosine) - 1.0F) < 1e-6F,
            "cosine distance mismatch");
    require_throws([&] { (void)vectorforge::distance(a, std::vector<float>{1.0F}, vectorforge::Metric::L2); },
                   "dimension mismatch must throw");
}

void test_exact_index() {
    vectorforge::ExactIndex index(2, vectorforge::Metric::L2);
    index.add(10, std::vector<float>{0.0F, 0.0F});
    index.add(20, std::vector<float>{2.0F, 0.0F});
    index.add(30, std::vector<float>{5.0F, 0.0F});
    const auto results = index.search(std::vector<float>{1.8F, 0.0F}, 2);
    require(results.size() == 2, "exact search returned wrong result count");
    require(results[0].id == 20 && results[1].id == 10, "exact search ordering mismatch");
}

void test_graph_recall() {
    constexpr std::size_t dimension = 24;
    constexpr std::size_t count = 1200;
    constexpr std::size_t queries = 80;
    constexpr std::size_t k = 10;

    vectorforge::ExactIndex exact(dimension, vectorforge::Metric::L2);
    vectorforge::GraphIndex graph({dimension, 20, 120, vectorforge::Metric::L2});
    std::mt19937 rng(0xC0FFEEU);

    for (std::size_t i = 0; i < count; ++i) {
        auto values = make_vector(rng, dimension);
        exact.add(i, values);
        graph.add(i, values);
    }

    double recall_sum = 0.0;
    for (std::size_t i = 0; i < queries; ++i) {
        auto query = make_vector(rng, dimension);
        recall_sum += recall_at_k(exact.search(query, k), graph.search(query, k, 120));
    }

    const double mean_recall = recall_sum / static_cast<double>(queries);
    require(mean_recall >= 0.75, "graph ANN mean recall@10 fell below 0.75");
}

void test_persistence_roundtrip() {
    const auto path = std::filesystem::temp_directory_path() / "vectorforge-roundtrip.kvf";
    std::filesystem::remove(path);

    vectorforge::GraphIndex index({4, 4, 12, vectorforge::Metric::Cosine});
    index.add(1, std::vector<float>{1.0F, 0.0F, 0.0F, 0.0F});
    index.add(2, std::vector<float>{0.0F, 1.0F, 0.0F, 0.0F});
    index.add(3, std::vector<float>{0.9F, 0.1F, 0.0F, 0.0F});
    const std::vector<float> query{1.0F, 0.05F, 0.0F, 0.0F};
    const auto before = index.search(query, 3, 12);

    index.save(path);
    auto loaded = vectorforge::GraphIndex::load(path);
    const auto after = loaded->search(query, 3, 12);
    require(before.size() == after.size(), "persistence changed result count");
    for (std::size_t i = 0; i < before.size(); ++i) {
        require(before[i].id == after[i].id, "persistence changed search ordering");
    }
    std::filesystem::remove(path);
}

void test_corruption_rejected() {
    const auto path = std::filesystem::temp_directory_path() / "vectorforge-corrupt.kvf";
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream << "not-an-index";
    }
    require_throws([&] { (void)vectorforge::GraphIndex::load(path); },
                   "corrupt persistence file must be rejected");
    std::filesystem::remove(path);
}

void test_concurrent_readers() {
    vectorforge::GraphIndex graph({16, 12, 64, vectorforge::Metric::L2});
    std::mt19937 rng(12345U);
    for (std::size_t i = 0; i < 500; ++i) {
        auto values = make_vector(rng, 16);
        graph.add(i, values);
    }

    const auto query = make_vector(rng, 16);
    std::atomic<std::size_t> successful{0};
    std::vector<std::future<void>> workers;
    for (int worker = 0; worker < 8; ++worker) {
        workers.emplace_back(std::async(std::launch::async, [&] {
            for (int iteration = 0; iteration < 100; ++iteration) {
                const auto results = graph.search(query, 10, 64);
                if (results.size() == 10) {
                    successful.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }));
    }
    for (auto& worker : workers) {
        worker.get();
    }
    require(successful.load(std::memory_order_relaxed) == 800,
            "concurrent readers returned incomplete results");
}

} // namespace

int main() {
    try {
        test_distance_metrics();
        test_exact_index();
        test_graph_recall();
        test_persistence_roundtrip();
        test_corruption_rejected();
        test_concurrent_readers();
        std::cout << "VectorForge tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "VectorForge test failure: " << error.what() << '\n';
        return 1;
    }
}
