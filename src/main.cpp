#include "vectorforge/exact_index.hpp"
#include "vectorforge/graph_index.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct BenchmarkOptions {
    std::size_t vectors{5000};
    std::size_t dimensions{64};
    std::size_t queries{200};
    std::size_t k{10};
    std::size_t ef_search{64};
};

std::size_t parse_size(const char* value, const char* name) {
    try {
        const std::string input(value);
        std::size_t consumed = 0;
        const auto parsed = std::stoull(input, &consumed);
        if (consumed != input.size() || parsed == 0) {
            throw std::invalid_argument("invalid");
        }
        return static_cast<std::size_t>(parsed);
    } catch (...) {
        throw std::invalid_argument(std::string("invalid value for ") + name);
    }
}

BenchmarkOptions parse_benchmark(int argc, char** argv) {
    BenchmarkOptions options;
    for (int i = 2; i < argc; ++i) {
        const std::string flag = argv[i];
        if (i + 1 >= argc) {
            throw std::invalid_argument("missing value after " + flag);
        }
        if (flag == "--vectors") {
            options.vectors = parse_size(argv[++i], "--vectors");
        } else if (flag == "--dimensions") {
            options.dimensions = parse_size(argv[++i], "--dimensions");
        } else if (flag == "--queries") {
            options.queries = parse_size(argv[++i], "--queries");
        } else if (flag == "--k") {
            options.k = parse_size(argv[++i], "--k");
        } else if (flag == "--ef-search") {
            options.ef_search = parse_size(argv[++i], "--ef-search");
        } else {
            throw std::invalid_argument("unknown option: " + flag);
        }
    }
    if (options.k > options.vectors) {
        throw std::invalid_argument("--k must not exceed --vectors");
    }
    return options;
}

std::vector<float> random_vector(std::mt19937& rng, std::size_t dimension) {
    std::normal_distribution<float> distribution(0.0F, 1.0F);
    std::vector<float> values(dimension);
    for (auto& value : values) {
        value = distribution(rng);
    }
    return values;
}

double recall_at_k(std::span<const vectorforge::SearchResult> exact,
                   std::span<const vectorforge::SearchResult> approximate) {
    std::unordered_set<std::uint64_t> expected;
    expected.reserve(exact.size());
    for (const auto& result : exact) {
        expected.insert(result.id);
    }
    std::size_t hits = 0;
    for (const auto& result : approximate) {
        hits += expected.contains(result.id) ? 1U : 0U;
    }
    return exact.empty() ? 1.0 : static_cast<double>(hits) / static_cast<double>(exact.size());
}

void run_benchmark(const BenchmarkOptions& options) {
    vectorforge::ExactIndex exact(options.dimensions, vectorforge::Metric::L2);
    vectorforge::GraphIndex graph(vectorforge::GraphParameters{
        .dimension = options.dimensions,
        .max_degree = 16,
        .ef_construction = 96,
        .metric = vectorforge::Metric::L2,
    });

    std::mt19937 rng(0x56464350U);
    std::vector<std::vector<float>> dataset;
    dataset.reserve(options.vectors);

    const auto build_start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < options.vectors; ++i) {
        dataset.push_back(random_vector(rng, options.dimensions));
        exact.add(i, dataset.back());
        graph.add(i, dataset.back());
    }
    const auto build_end = std::chrono::steady_clock::now();

    double recall_sum = 0.0;
    std::chrono::nanoseconds exact_time{0};
    std::chrono::nanoseconds graph_time{0};

    for (std::size_t i = 0; i < options.queries; ++i) {
        auto query = random_vector(rng, options.dimensions);

        const auto exact_start = std::chrono::steady_clock::now();
        const auto exact_results = exact.search(query, options.k);
        const auto exact_end = std::chrono::steady_clock::now();
        exact_time += std::chrono::duration_cast<std::chrono::nanoseconds>(exact_end - exact_start);

        const auto graph_start = std::chrono::steady_clock::now();
        const auto graph_results = graph.search(query, options.k, options.ef_search);
        const auto graph_end = std::chrono::steady_clock::now();
        graph_time += std::chrono::duration_cast<std::chrono::nanoseconds>(graph_end - graph_start);

        recall_sum += recall_at_k(exact_results, graph_results);
    }

    const auto build_ms = std::chrono::duration<double, std::milli>(build_end - build_start).count();
    const auto exact_us = std::chrono::duration<double, std::micro>(exact_time).count() /
                          static_cast<double>(options.queries);
    const auto graph_us = std::chrono::duration<double, std::micro>(graph_time).count() /
                          static_cast<double>(options.queries);

    std::cout << std::fixed << std::setprecision(3)
              << "vectors=" << options.vectors << '\n'
              << "dimensions=" << options.dimensions << '\n'
              << "queries=" << options.queries << '\n'
              << "k=" << options.k << '\n'
              << "ef_search=" << options.ef_search << '\n'
              << "build_ms=" << build_ms << '\n'
              << "recall_at_k=" << (recall_sum / static_cast<double>(options.queries)) << '\n'
              << "exact_mean_us=" << exact_us << '\n'
              << "graph_mean_us=" << graph_us << '\n';
}

void print_usage() {
    std::cout << "VectorForge v0.1\n\n"
              << "Usage:\n"
              << "  vectorforge benchmark [--vectors N] [--dimensions N] [--queries N] [--k N] [--ef-search N]\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            print_usage();
            return EXIT_SUCCESS;
        }
        if (std::string(argv[1]) == "benchmark") {
            run_benchmark(parse_benchmark(argc, argv));
            return EXIT_SUCCESS;
        }
        throw std::invalid_argument("unknown command");
    } catch (const std::exception& error) {
        std::cerr << "vectorforge: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
