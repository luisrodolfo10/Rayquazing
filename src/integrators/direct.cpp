#include <lightwave.hpp>

namespace lightwave {

class DirectIntegrator : public SamplingIntegrator {
    bool m_remap;

public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        m_remap = properties.get<bool>("remap", true);
    }
    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        if (its) {
            // you will need to get a light sample using the provided
            // sampleLight function.
            //  Then, get its direct contribution information (emission weight,
            //  direction 𝜔 and distance) using the 𝑖 sampleDirect function of
            //  the light associated with the light sample.
            LightSample lightSample = m_scene->sampleLight(rng);
            DirectLightSample directSample =
                lightSample.light->sampleDirect(its.position, rng);
            Ray SecondaryRay           = Ray(its.position, directSample.wi);
            Intersection secondary_its = m_scene->intersect(SecondaryRay, rng);

            if (!secondary_its) {
                float cosTheta =
                    std::max(0.f, its.shadingNormal.dot(directSample.wi));
                Color contribution = directSample.weight * cosTheta;
                return contribution;
            }
            return Color(0.f);
        } else {
            // If no intersection, return black
            return its.evaluateEmission().value;
        }
    }
    std::string toString() const override {
        return tfm::format(
            "DirectIntegrator[\n"
            "  remap = %s,\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            m_remap,
            indent(m_sampler),
            indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(DirectIntegrator, "direct")