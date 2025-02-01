#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughConductor : public Bsdf {
    ref<Texture> m_reflectance;
    ref<Texture> m_roughness;

public:
    RoughConductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
        m_roughness   = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // Using the squared roughness parameter results in a more gradual
        // transition from specular to rough. For numerical stability, we avoid
        // extremely specular distributions (alpha values below 10^-3)
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        Vector wh = (wi + wo).normalized();
        Color R   = m_reflectance->evaluate(uv);

        if (std::isnan(wi.x()) || std::isnan(wi.y()) || std::isnan(wi.z())) {
            return BsdfEval().invalid();
        }

        float cosThetao = Frame::absCosTheta(wo);
        float cosThetai = Frame::absCosTheta(wi);

        if (cosThetao == 0 || cosThetai == 0) {
            return BsdfEval().invalid(); // Invalid
        }

        float D   = microfacet::evaluateGGX(alpha, wh);
        float Gwi = microfacet::smithG1(alpha, wh, wi);
        float Gwo = microfacet::smithG1(alpha, wh, wo);

        // cosththetai get cancelled due to multiplication
        Color Fr      = (R * D * Gwi * Gwo) / (4 * cosThetao);
        BsdfEval bsdf = BsdfEval();

        bsdf.value = Fr;
        return BsdfEval(bsdf);
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        // Normalized
        Vector wh =
            microfacet::sampleGGXVNDF(alpha, wo, rng.next2D()).normalized();
        Vector wi = reflect(wo, wh).normalized(); // wi has the jacobian term

        if (std::isnan(wi.x()) || std::isnan(wi.y()) || std::isnan(wi.z())) {
            return BsdfSample().invalid();
        }

        float cosThetai = Frame::cosTheta(wi);

        if (cosThetai == 0) {
            return BsdfSample().invalid(); // Invalid sample
        }

        Color R   = m_reflectance->evaluate(uv);
        float Gwi = microfacet::smithG1(alpha, wh, wi);

        Color weight = (R * Gwi); // Simplifying the equation has /costhetai

        BsdfSample bsdfSample = BsdfSample();
        bsdfSample.weight     = weight;
        bsdfSample.wi         = wi.normalized();
        return bsdfSample;
    }

    std::string toString() const override {
        return tfm::format(
            "RoughConductor[\n"
            "  reflectance = %s,\n"
            "  roughness = %s\n"
            "]",
            indent(m_reflectance),
            indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughConductor, "roughconductor")
