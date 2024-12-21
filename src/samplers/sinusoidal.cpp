#include <lightwave.hpp>

#include "pcg32.h"
#include <functional>

namespace lightwave {

/**
 * @brief A sinusoidal-based number sampler.
 * Generates random samples using a sinusoidal function. Frequency and amplitud
 * are configurable.
 */
class Sinusoidal : public Sampler {
    uint64_t m_seed;
    pcg32 m_pcg;
    float frequency;
    float amplitude;

public:
    Sinusoidal(const Properties &properties) : Sampler(properties) {
        m_seed =
            properties.get<int>("seed", std::getenv("reference") ? 1337 : 420);
        frequency = properties.get<float>("frequency", 0.00003f);
        amplitude = properties.get<float>("amplitude", 0.5f);
    }

    void seed(int sampleIndex) override { m_pcg.seed(m_seed, sampleIndex); }

    void seed(const Point2i &pixel, int sampleIndex) override {
        const uint64_t a =
            hash::fnv1a(pixel.x(), pixel.y(), sampleIndex, m_seed);
        m_pcg.seed(a);
    }

    float sinusoidal(int sampleIndex) {
        return 0.5 + amplitude * std::sin(sampleIndex * frequency);
    }

    float next() override {
        static int sampleIndex = 0;
        return sinusoidal(sampleIndex++);
    }

    ref<Sampler> clone() const override {
        return std::make_shared<Sinusoidal>(*this);
    }

    std::string toString() const override {
        return tfm::format(
            "Sinusoidal[\n"
            "  count = %d\n"
            "]",
            m_samplesPerPixel);
    }
};

} // namespace lightwave

REGISTER_SAMPLER(Sinusoidal, "sinusoidal")