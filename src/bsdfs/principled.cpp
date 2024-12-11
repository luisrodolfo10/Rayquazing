#include <lightwave.hpp>

#include "fresnel.hpp"
#include "microfacet.hpp"

namespace lightwave {

struct DiffuseLobe {
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by color
        BsdfEval bsdf = BsdfEval();
        // if (!Frame::sameHemisphere(wo, wi)) {
        //     return bsdf.invalid();
        // }
        bsdf.value = color / Pi;
        bsdf.value *= abs(wi.z());
        return BsdfEval(bsdf);
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        Vector wi = squareToCosineHemisphere(rng.next2D());
        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by color

        BsdfSample bsdfSample = BsdfSample();
        bsdfSample.weight     = color;
        bsdfSample.wi         = wi.normalized();

        return bsdfSample;
    }
};

struct MetallicLobe {
    float alpha;
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        // hints:
        // * copy your roughconductor bsdf evaluate here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
        Vector wh = (wi + wo).normalized();
        Color R   = color;
        float D   = microfacet::evaluateGGX(alpha, wh);
        float Gwi = microfacet::smithG1(alpha, wh, wi);
        float Gwo = microfacet::smithG1(alpha, wh, wo);

        // float cosThetai = Frame::absCosTheta(wi);
        float cosThetao = Frame::absCosTheta(wo);
        if (cosThetao == 0) {
            return BsdfEval(); // Invalid
        }

        Color Fr      = (R * D * Gwi * Gwo) / (4 * cosThetao);
        BsdfEval bsdf = BsdfEval();
        bsdf.value    = Fr;
        return BsdfEval(bsdf);
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        // hints:
        // * copy your roughconductor bsdf sample here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
        Vector wh =
            microfacet::sampleGGXVNDF(alpha, wo, rng.next2D()).normalized();
        Vector wi = reflect(wo, wh).normalized(); // wi has the jacobian term

        Color R   = color;
        float Gwi = microfacet::smithG1(alpha, wh, wi);

        Color weight = (R * Gwi); // Simplifying the equation has /costhetai

        BsdfSample bsdfSample = BsdfSample();
        bsdfSample.weight     = weight;
        bsdfSample.wi         = wi.normalized();
        return bsdfSample;
    }
};

class Principled : public Bsdf {
    ref<Texture> m_baseColor;
    ref<Texture> m_roughness;
    ref<Texture> m_metallic;
    ref<Texture> m_specular;

    struct Combination {
        float diffuseSelectionProb;
        DiffuseLobe diffuse;
        MetallicLobe metallic;
    };

    Combination combine(const Point2 &uv, const Vector &wo) const {
        const auto baseColor = m_baseColor->evaluate(uv);
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const auto specular = m_specular->scalar(uv);
        const auto metallic = m_metallic->scalar(uv);
        const auto F =
            specular * schlick((1 - metallic) * 0.08f, Frame::cosTheta(wo));

        const DiffuseLobe diffuseLobe = {
            .color = (1 - F) * (1 - metallic) * baseColor,
        };
        const MetallicLobe metallicLobe = {
            .alpha = alpha,
            .color = F * Color(1) + (1 - F) * metallic * baseColor,
        };

        const auto diffuseAlbedo = diffuseLobe.color.mean();
        const auto totalAlbedo =
            diffuseLobe.color.mean() + metallicLobe.color.mean();
        return {
            .diffuseSelectionProb =
                totalAlbedo > 0 ? diffuseAlbedo / totalAlbedo : 1.0f,
            .diffuse  = diffuseLobe,
            .metallic = metallicLobe,
        };
    }

public:
    Principled(const Properties &properties) {
        m_baseColor = properties.get<Texture>("baseColor");
        m_roughness = properties.get<Texture>("roughness");
        m_metallic  = properties.get<Texture>("metallic");
        m_specular  = properties.get<Texture>("specular");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);

        // hint: evaluate ⁠ combination.diffuse ⁠ and
        // ⁠ combination.metallic ⁠ and combine their results
        const BsdfEval diffuseEval  = combination.diffuse.evaluate(wo, wi);
        const BsdfEval metallicEval = combination.metallic.evaluate(wo, wi);

        // float diffuseWeight  = combination.diffuseSelectionProb;
        // float metallicWeight = 1.0f - diffuseWeight;
        BsdfEval bsdfEval = BsdfEval();
        // bsdfEval.value       = diffuseEval.value * diffuseWeight +
        //                  metallicEval.value * metallicWeight;
        bsdfEval.value = diffuseEval.value + metallicEval.value;
        return bsdfEval;
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);

        // hint: sample either ⁠ combination.diffuse ⁠ (probability
        // ⁠ combination.diffuseSelectionProb) or
        // ⁠ combination.metallic ⁠
        if (rng.next() < combination.diffuseSelectionProb) {
            BsdfSample diffuseSample = combination.diffuse.sample(wo, rng);
            diffuseSample.weight /= combination.diffuseSelectionProb;
            return diffuseSample;
        } else {
            BsdfSample metallicSample = combination.metallic.sample(wo, rng);
            metallicSample.weight /= (1.0f - combination.diffuseSelectionProb);
            return metallicSample;
        }
    }

    std::string toString() const override {
        return tfm::format(
            "Principled[\n"
            "  baseColor = %s,\n"
            "  roughness = %s,\n"
            "  metallic  = %s,\n"
            "  specular  = %s,\n"
            "]",
            indent(m_baseColor),
            indent(m_roughness),
            indent(m_metallic),
            indent(m_specular));
    }
};

} // namespace lightwave

REGISTER_BSDF(Principled, "principled")