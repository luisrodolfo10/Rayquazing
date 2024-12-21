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
class Perlin : public Sampler {
    uint64_t m_seed;
    pcg32 m_pcg;

public:
    Perlin(const Properties &properties) : Sampler(properties) {
        m_seed =
            properties.get<int>("seed", std::getenv("reference") ? 1337 : 420);
    }

    void seed(int sampleIndex) override { m_pcg.seed(m_seed, sampleIndex); }

    void seed(const Point2i &pixel, int sampleIndex) override {
        const uint64_t a =
            hash::fnv1a(pixel.x(), pixel.y(), sampleIndex, m_seed);
        m_pcg.seed(a);
    }

    // Perlin noise obtained from https://www.youtube.com/watch?v=kCIaHqb60Cw
    Vector2 randomGradient(int ix, int iy) {
        // No precomputed gradients mean this works for any number of grid
        // coordinates
        const unsigned w = 8 * sizeof(unsigned);
        const unsigned s = w / 2;
        unsigned a = ix, b = iy;
        a *= 3284157443;

        b ^= a << s | a >> w - s;
        b *= 1911520717;

        a ^= b << s | b >> w - s;
        a *= 2048419325;
        float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]

        // Create the vector from the angle
        Vector2 v;
        v.x() = sin(random);
        v.y() = cos(random);

        return v;
    }

    float dotGridGradient(int ix, int iy, float x, float y) {
        Vector2 gradient = randomGradient(ix, iy);

        float dx = x - float(ix);
        float dy = y - float(iy);
        return (dx * gradient.x() + dy * gradient.y());
    }

    float interpolate(float a0, float a1, float w) {
        return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
    }

    float perlin(float x, float y) {
        // Grid cell coordinates
        int x0 = int(x);
        int y0 = int(y);
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        // Interpolating weights
        float sx = x - float(x0);
        float sy = y - float(y0);

        float n0  = dotGridGradient(x0, y0, x, y);
        float n1  = dotGridGradient(x1, y0, x, y);
        float ix0 = interpolate(n0, n1, sx);

        n0        = dotGridGradient(x0, y1, x, y);
        n1        = dotGridGradient(x1, y1, x, y);
        float ix1 = interpolate(n0, n1, sx);

        float val = interpolate(ix0, ix1, sy);
        return val;
    }

    float next() override {
        static int sampleIndex = 0;
        float x =
            sampleIndex * 0.01f; // Scale to control the frequency of the noise
        float y = m_seed * 0.001f; // Seed-dependent y-coordinate
        sampleIndex++;
        return perlin(x, y);
    }

    ref<Sampler> clone() const override {
        return std::make_shared<Perlin>(*this);
    }

    std::string toString() const override {
        return tfm::format(
            "Perlin[\n"
            "  count = %d\n"
            "]",
            m_samplesPerPixel);
    }
};

} // namespace lightwave

REGISTER_SAMPLER(Perlin, "perlin")