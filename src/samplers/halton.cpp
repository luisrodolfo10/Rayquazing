#include <lightwave.hpp>

#include "pcg32.h"
#include <cmath>
#include <random>

// Reference:
// https:
//
// www.pbr-book.org/4ed/Sampling_and_Reconstruction/Halton_Sampler#fragment-HaltonSamplerPublicMethods-0
namespace lightwave {

/**
 * @brief Generates numbers following the Halton sampling distribution.
 */
class Halton : public Sampler {
    uint64_t m_seed;
    pcg32 m_pcg;
    static constexpr int PrimeTableSize = 10000;
    std::vector<int> Primes             = GeneratePrimes(PrimeTableSize);
    // Instead of mixBits in the reference code
    std::vector<int> Permutations = GeneratePermutations(Primes);
    int64_t haltonIndex           = 0;
    int dimension                 = 0;
    int primeSum = std::accumulate(Primes.begin(), Primes.end(), 0);
    std::vector<int> primeSums;

private:
    std::vector<int> GeneratePermutations(const std::vector<int> &Primes) {
        std::vector<int> Permutations(Primes.size());
        Permutations.resize(primeSum);
        int *perm = &Permutations[0];
        for (size_t i = 0; i < Primes.size(); ++i) {
            for (int j = 0; j < Primes[i]; ++j)
                perm[j] = j;
            perm += Primes[i];
        }
        return Permutations;
    }

    std::vector<int> GeneratePrimes(int PrimeTableSize) {
        // a table of bools will mark whether the number is prime or not
        std::vector<bool> isPrime(PrimeTableSize + 1, true);
        isPrime[0] = isPrime[1] = false; // 0 and 1 are not prime numbers.
        for (int i = 2; i <= std::sqrt(PrimeTableSize); ++i) {
            if (isPrime[i]) {
                for (int j = i * i; j <= PrimeTableSize; j += i) {
                    isPrime[j] = false;
                }
            }
        }
        // we extract those that have not been marked as "not prime"
        std::vector<int> Primes;
        for (int i = 2; i <= PrimeTableSize; ++i) {
            if (isPrime[i]) {
                Primes.push_back(i);
            }
        }

        return Primes;
    }

    float OwenScrambledRadicalInverse(int baseIndex, int64_t a, int *perm) {
        int base      = Primes[baseIndex];
        float invBase = (float) 1 / (float) base, invBaseM = 1;
        uint64_t reversedDigits = 0;
        int digitIndex          = 0;
        while (1 - invBaseM < 1) {
            uint64_t next  = a / base;
            int digitValue = a - next * base;
            digitValue     = perm[digitValue];
            reversedDigits = reversedDigits * base + digitValue;
            invBaseM *= invBase;
            ++digitIndex;
            a = next;
        }
        return min(invBaseM * reversedDigits, 1 - Epsilon);
    }

    float SampleDimension(int dimension) {
        int *perm = &Permutations[primeSums[dimension]];
        return OwenScrambledRadicalInverse(dimension, haltonIndex, perm);
    }

public:
    Halton(const Properties &properties) : Sampler(properties) {
        m_seed =
            properties.get<int>("seed", std::getenv("reference") ? 1337 : 420);
        std::partial_sum(
            Primes.begin(), Primes.end(), std::back_inserter(primeSums));
    }

    void seed(int sampleIndex) override {
        m_pcg.seed(m_seed, sampleIndex);
        // m_haltonIndex = 0;
    }

    void seed(const Point2i &pixel, int sampleIndex) override {
        const uint64_t a =
            hash::fnv1a(pixel.x(), pixel.y(), sampleIndex, m_seed);
        m_pcg.seed(a);
    }

    float next() override {
        if (dimension >= PrimeTableSize) {
            dimension = dimension % PrimeTableSize;
        }
        return SampleDimension(dimension++);
    }

    Point2 next2D() override {
        if (dimension + 1 >= PrimeTableSize) {
            dimension = dimension % PrimeTableSize;
        }
        return { SampleDimension(dimension), SampleDimension(dimension + 1) };
    }

    ref<Sampler> clone() const override {
        return std::make_shared<Halton>(*this);
    }

    std::string toString() const override {
        return tfm::format(
            "Halton[\n"
            "  count = %d\n"
            "]",
            m_samplesPerPixel);
    }
};

} // namespace lightwave

REGISTER_SAMPLER(Halton, "halton")