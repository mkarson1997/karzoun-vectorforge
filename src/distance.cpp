#include "vectorforge/distance.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vectorforge {
namespace {

void validate_dimensions(std::span<const float> a, std::span<const float> b) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("vector dimensions do not match");
    }
    if (a.empty()) {
        throw std::invalid_argument("vectors must not be empty");
    }
}

float l2_distance(std::span<const float> a, std::span<const float> b) {
    float sum = 0.0F;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const float delta = a[i] - b[i];
        sum += delta * delta;
    }
    return sum;
}

float cosine_distance(std::span<const float> a, std::span<const float> b) {
    float dot = 0.0F;
    float norm_a = 0.0F;
    float norm_b = 0.0F;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    if (norm_a == 0.0F || norm_b == 0.0F) {
        return (norm_a == norm_b) ? 0.0F : 1.0F;
    }

    const float similarity = dot / std::sqrt(norm_a * norm_b);
    return 1.0F - std::clamp(similarity, -1.0F, 1.0F);
}

} // namespace

float distance(std::span<const float> a,
               std::span<const float> b,
               Metric metric) {
    validate_dimensions(a, b);
    switch (metric) {
    case Metric::L2:
        return l2_distance(a, b);
    case Metric::Cosine:
        return cosine_distance(a, b);
    }
    throw std::invalid_argument("unsupported distance metric");
}

} // namespace vectorforge
