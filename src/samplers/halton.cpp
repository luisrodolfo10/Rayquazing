#include <lightwave.hpp>

#include "pcg32.h"
#include <cmath>
#include <random>

// Reference:
// https://www.pbr-book.org/4ed/Sampling_and_Reconstruction/Halton_Sampler#fragment-HaltonSamplerPublicMethods-0
namespace lightwave {

/**
 * @brief Generates numbers following the Halton sampling distribution.
 */
class Halton : public Sampler {
    uint64_t m_seed;
    pcg32 m_pcg;
    float m_random;
    static constexpr int PrimeTableSize = 500;
    std::vector<int> Primes;
    std::vector<int> Permutations;
    int64_t haltonIndex = 0;
    int dimension       = 0;
    int primeSum;
    std::vector<int> primeSums;

private:
    std::vector<int> GeneratePermutations(const std::vector<int> &Primes,
                                          const std::vector<int> &primeSums) {
        std::mt19937 rng(m_seed);
        std::vector<int> Permutations;
        Permutations.resize(primeSums.back());

        for (int i = 0; i < Primes.size(); ++i) {
            int p      = Primes[i];
            int offset = primeSums[i];
            for (int v = 0; v < p; ++v) {
                Permutations[offset + v] = v;
            }

            std::shuffle(Permutations.begin() + offset,
                         Permutations.begin() + offset + p,
                         rng);
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

    float SampleDimension(int dimension, int index) {
        int *perm = &Permutations[primeSums[dimension]];
        return OwenScrambledRadicalInverse(dimension, index, perm);
    }

public:
    Halton(const Properties &properties) : Sampler(properties) {
        m_seed =
            properties.get<int>("seed", std::getenv("reference") ? 1337 : 420);
        Primes = GeneratePrimes(PrimeTableSize);
        primeSums.resize(Primes.size() + 1);
        for (size_t i = 0; i < Primes.size(); ++i)
            primeSums[i + 1] = primeSums[i] + Primes[i];
        primeSum     = std::accumulate(Primes.begin(), Primes.end(), 0);
        Permutations = GeneratePermutations(Primes, primeSums);
    }

    void seed(int sampleIndex) override {
        m_pcg.seed(m_seed, sampleIndex);
        m_random    = 0;
        haltonIndex = sampleIndex;
        dimension   = 0;
    }

    void seed(const Point2i &pixel, int sampleIndex) override {
        const uint64_t hashVal =
            hash::fnv1a(pixel.x(), pixel.y(), sampleIndex, m_seed);
        const uint64_t a = (uint64_t(pixel.x()) << 32) ^ pixel.y();
        m_pcg.seed(1337, a);
        m_random = m_pcg.nextFloat();

        haltonIndex = sampleIndex;
        dimension   = 0;
    }

    float next() override {
        if (dimension >= (int) Primes.size()) {
            dimension = dimension % (int) Primes.size();
        }
        float sample = SampleDimension(dimension, haltonIndex);
        dimension++;
        if (sample + m_random > 1.f)
            sample -= 1.f;
        return sample + m_random;
    }

    Point2 next2D() override {
        float x = next();
        float y = next();
        haltonIndex++;
        return Point2(x, y);
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
