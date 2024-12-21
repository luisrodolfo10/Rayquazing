#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        Color color = Color(m_albedo->evaluate(uv) / Pi); 
        color *= abs(wi.normalized().z());
        BsdfEval bsdf = BsdfEval();
        if (!Frame::sameHemisphere(wo, wi)) {
            return bsdf.invalid();
        }
        bsdf.value = color;
        return BsdfEval(bsdf);
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // Sample a ray direction
        Vector wi = squareToCosineHemisphere(rng.next2D());
        // Assign the correct weight to the sample: evaluation of the BSDF
        // itself (how much light is reflect) multiplied by the foreshortening
        // term cos 𝜔𝑖

        if (!Frame::sameHemisphere(wo, wi)) {
            wi = -wi; // Flip the direction
        }

        Color weight = m_albedo->evaluate(
            uv); // Would be cosTheta / pdf,  but with simplified equation we
                 // just have to evaluate the albedo.

        BsdfSample bsdfSample = BsdfSample();
        bsdfSample.weight     = weight;
        bsdfSample.wi         = wi.normalized();

        return bsdfSample;
    }

    std::string toString() const override {
        return tfm::format(
            "Diffuse[\n"
            "  albedo = %s\n"
            "]",
            indent(m_albedo));
    }
};

} // namespace lightwave

REGISTER_BSDF(Diffuse, "diffuse")
