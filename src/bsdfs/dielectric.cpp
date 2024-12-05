#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting or refracting `wo' is zero, hence we can
        // just ignore that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        Vector n = Vector(0, 0, 1);
        Color weight;
        Vector wi;
        float eta      = m_ior->scalar(uv);
        float cosTheta = Frame::cosTheta(wo);
        if (cosTheta < 0) {
            eta = 1 / eta;
        }
        float fresnelReflectance = fresnelDielectric(cosTheta, eta);

        if (rng.next() < fresnelReflectance) {
            weight = m_reflectance->evaluate(uv);
            wi     = reflect(wo, n);
        } else {
            weight = m_transmittance->evaluate(uv) / (pow(eta, 2));
            wi     = refract(wo, n, eta);
        }

        BsdfSample bsdfSample = BsdfSample();
        bsdfSample.weight     = weight;
        bsdfSample.wi         = wi;

        return bsdfSample;
    }

    std::string toString() const override {
        return tfm::format(
            "Dielectric[\n"
            "  ior           = %s,\n"
            "  reflectance   = %s,\n"
            "  transmittance = %s\n"
            "]",
            indent(m_ior),
            indent(m_reflectance),
            indent(m_transmittance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Dielectric, "dielectric")
