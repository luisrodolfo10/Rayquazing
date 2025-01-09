// #include <lightwave.hpp>

// #include "pcg32.h"
// #include <cmath>
// #include <functional>
// #include <random>

// // Reference:
// //
// https://www.pbr-book.org/4ed/Sampling_and_Reconstruction/Halton_Sampler#fragment-HaltonSamplerPublicMethods-0
// namespace lightwave {

// /**
//  * @brief Generates numbers in [0,1) following the Halton sampling
//  distribution.
//  * @see Internally, this sampler uses the PCG32 library to generate random
//  * numbers.
//  */
// class Halton : public Sampler {
//     uint64_t m_seed;
//     pcg32 m_pcg;
//     static constexpr int PrimeTableSize = 64;

// private:
//     float OwenScrambledRadicalInverse(int baseIndex, uint64_t a,
//                                       uint32_t hash) {
//         int base      = Primes[baseIndex]; // TODO.
//         float invBase = (float) 1 / (float) base, invBaseM = 1;
//         uint64_t reversedDigits = 0;
//         int digitIndex          = 0;
//         while (1 - invBaseM < 1) {
//             uint64_t next  = a / base;
//             int digitValue = a - next * base;
//             uint32_t digitHash =
//                 MixBits(hash ^ reversedDigits); // TODO. IS IT IN CMATH?
//             digitValue = PermutationElement(
//                 digitValue, base, digitHash); // TODO. IS IT IN CMATH?
//             reversedDigits = reversedDigits * base + digitValue;
//             invBaseM *= invBase;
//             ++digitIndex;
//             a = next;
//         }
//         return min(invBaseM * reversedDigits, 1 - Epsilon);
//     }

//     static uint64_t multiplicativeInverse(int64_t a, int64_t n) {
//         int64_t x, y;
//         extendedGCD(a, n, &x, &y);
//         return x % n;
//     }

//     static void extendedGCD(uint64_t a, uint64_t b, int64_t *x, int64_t *y) {
//         if (b == 0) {
//             *x = 1;
//             *y = 0;
//             return;
//         }
//         int64_t d = a / b, xp, yp;
//         extendedGCD(b, a % b, &xp, &yp);
//         *x = yp;
//         *y = xp - (d * yp);
//     }

//     float SampleDimension(int dim) const {
//         if (dim >= PrimeTableSize)
//             dim = 0;
//         return OwenScrambledRadicalInverse(primes[dim], m_haltonIndex);
//     }

// public:
//     Halton(const Properties &properties) : Sampler(properties) {
//         m_seed =
//             properties.get<int>("seed", std::getenv("reference") ? 1337 :
//             420);
//     }

//     void seed(int sampleIndex) override {
//         m_pcg.seed(m_seed, sampleIndex);
//         m_haltonIndex = 0;
//     }

//     void seed(const Point2i &pixel, int sampleIndex) override {
//         const uint64_t a =
//             hash::fnv1a(pixel.x(), pixel.y(), sampleIndex, m_seed);
//         m_pcg.seed(a);
//     }

//     float next() override {
//         float value = SampleDimension(m_dimension);
//         m_dimension++;
//         return value;
//     }

//     ref<Sampler> clone() const override {
//         return std::make_shared<Halton>(*this);
//     }

//     std::string toString() const override {
//         return tfm::format(
//             "Halton[\n"
//             "  count = %d\n"
//             "]",
//             m_samplesPerPixel);
//     }
// };

// } // namespace lightwave

// REGISTER_SAMPLER(Halton, "halton")
