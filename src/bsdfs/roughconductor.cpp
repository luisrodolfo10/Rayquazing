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

        // hints:
        // * the microfacet normal can be computed from `wi' and `wo'
        // Vector sum = wi + wo;
        Vector wh = (wi + wo).normalized();
        Color R   = m_reflectance->evaluate(uv);
        float D   = microfacet::evaluateGGX(alpha, wh);
        float Gwi = microfacet::smithG1(alpha, wh, wi);
        float Gwo = microfacet::smithG1(alpha, wh, wo);

        // float cosThetai = abs(wi.z());
        // float cosThetao = abs(wo.z());
        float cosThetai = Frame::absCosTheta(wi);
        float cosThetao = Frame::absCosTheta(wo);
        // float cosThetai = std::max(0.0f, Frame::cosTheta(wi));
        // float cosThetao = std::max(0.0f, Frame::cosTheta(wo));

        Color Fr      = (R * D * Gwi * Gwo) / (4 * cosThetai * cosThetao);
        BsdfEval bsdf = BsdfEval();
        bsdf.value    = Fr;
        return BsdfEval(bsdf);
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        // Normalized
        Vector wh = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        Vector wi = reflect(wo, wh).normalized(); // wi has the jacobian term

        float jacobian = microfacet::detReflection(wh, wo);
        float pdfWi    = microfacet::pdfGGXVNDF(alpha, wh, wo);

        Color R = m_reflectance->evaluate(uv);
        float D = microfacet::evaluateGGX(alpha, wh);
        // float G   = microfacet::smithG1(alpha, wh, wo);
        float Gwi = microfacet::smithG1(alpha, wh, wi);
        float Gwo = microfacet::smithG1(alpha, wh, wo);

        float cosThetai = Frame::absCosTheta(wi);
        float cosThetao = Frame::absCosTheta(wo);

        // Color weight = (R * D * Gwo * abs(wh.dot(wo))) / cosThetao;
        // Color weight = (R * D * Gwi * Gwo) / (4 * cosThetai * cosThetao);
        // weight /= pdfWi / jacobian;

        Color weight = (Gwi * m_reflectance->evaluate(uv));

        //  float fresnel = schlickWeight(Frame::absCosTheta(wh));
        //  Color weight = (R * D * Gwo) * abs(wh.dot(wo));

        BsdfSample bsdfSample = BsdfSample();
        bsdfSample.weight     = weight;
        bsdfSample.wi         = wi.normalized();
        return bsdfSample;
        // hints:
        // * do not forget to cancel out as many terms from your equations as
        // possible!
        //   (the resulting sample weight is only a product of two factors)
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
