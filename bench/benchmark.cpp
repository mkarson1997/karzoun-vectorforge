#include "vectorforge/exact_index.hpp"
#include "vectorforge/graph_index.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

namespace {

std::vector<float> make_vector(std::mt19937& rng, std::size_t dimension) {
    std::normal_distribution<float> distribution(0.0F, 1.0F);
    std::vector<float> values(dimension);
    for (auto& value : values) {
        value = distribution(rng);
    }
    return values;
}

double recall(const std::vector<vectorforge::SearchResult>& exact,
              const std::vector<vectorforge::SearchResult>& approximate) {
    std::unordered_set<std::uint64_t> ids;
    for (const auto& item : exact) {
        ids.insert(item.id);
    }
    std::size_t hits = 0;
    for (const auto& item : approximate) {
        hits += ids.contains(item.id) ? 1U : 0U;
    }
    return static_cast<double>(hits) / static_cast<double>(exact.size());
}

} // namespace

int main() {
    constexpr std::size_t dimension = 64;
    constexpr std::size_t vector_count = 10'000;
    constexpr std::size_t query_count = 500;
    constexpr std::size_t k = 10;

    vectorforge::ExactIndex exact(dimension, vectorforge::Metric::L2);
    vectorforge::GraphIndex graph({dimension, 16, 96, vectorforge::Metric::L2});
    std::mt19937 rng(0xBADC0DEU);

    for (std::size_t i = 0; i < vector_count; ++i) {
        auto values = make_vector(rng, dimension);
        exact.add(i, values);
        graph.add(i, values);
    }

    std::chrono::nanoseconds exact_duration{};
    std::chrono::nanoseconds graph_duration{};
    double recall_sum = 0.0;

    for (std::size_t q = 0; q < query_count; ++q) {
        auto query = make_vector(rng, dimension);

        const auto exact_begin = std::chrono::steady_clock::now();
        const auto truth = exact.search(query, k);
        const auto exact_end = std::chrono::steady_clock::now();
        exact_duration += exact_end - exact_begin;

        const auto graph_begin = std::chrono::steady_clock::now();
        const auto approximate = graph.search(query, k, 96);
        const auto graph_end = std::chrono::steady_clock::now();
        graph_duration += graph_end - graph_begin;
        recall_sum += recall(truth, approximate);
    }

    const double exact_us = std::chrono::duration<double, std::micro>(exact_duration).count() / query_count;
    const double graph_us = std::chrono::duration<double, std::micro>(graph_duration).count() / query_count;

    std::cout << std::fixed << std::setprecision(3)
              << "dataset=" << vector_count << "x" << dimension << '\n'
              << "queries=" << query_count << '\n'
              << "recall@" << k << "=" << recall_sum / query_count << '\n'
              << "exact_mean_us=" << exact_us << '\n'
              << "graph_mean_us=" << graph_us << '\n';
}
