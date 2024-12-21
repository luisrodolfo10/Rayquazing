#include <lightwave.hpp>

#include "pcg32.h"
#include <functional>

namespace lightwave {

/**
 * @brief Generates random numbers uniformely distributed in [0,1), which are
 * all stochastically independent from another. This is the simplest form of
 * random number generation, and will be sufficient for our introduction to
 * Computer Graphics. If you want to reduce the noise in your renders, a simple
 * way is to implement more sophisticated random numbers (e.g., jittered
 * sampling or blue noise sampling).
 * @see Internally, this sampler uses the PCG32 library to generate random
 * numbers.
 */
class Sinusoidal : public Sampler {
    uint64_t m_seed;
    pcg32 m_pcg;

public:
    Sinusoidal(const Properties &properties) : Sampler(properties) {
        m_seed =
            properties.get<int>("seed", std::getenv("reference") ? 1337 : 420);
    }

    void seed(int sampleIndex) override { m_pcg.seed(m_seed, sampleIndex); }

    void seed(const Point2i &pixel, int sampleIndex) override {
        const uint64_t a =
            hash::fnv1a(pixel.x(), pixel.y(), sampleIndex, m_seed);
        m_pcg.seed(a);
    }

    float sinusoidal(int sampleIndex) {
        return 0.5 + 0.5 * std::sin(sampleIndex * 0.000003f);
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