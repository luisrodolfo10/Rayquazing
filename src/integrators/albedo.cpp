#include <lightwave.hpp>

namespace lightwave {

class AlbedoIntegrator : public SamplingIntegrator {
public:
    AlbedoIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        if (its) {
            // Sample the BSDF at the intersection.
            BsdfSample sample = its.sampleBsdf(rng);
            Color albedo      = sample.weight;
            return albedo;
        } else {
            // If no intersection, return black.
            return Color(0.0f);
        }
    }

    std::string toString() const override {
        return tfm::format(
            "AlbedoIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(AlbedoIntegrator, "albedo")