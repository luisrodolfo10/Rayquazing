#include <lightwave.hpp>

namespace lightwave {

class NormalIntegrator : public SamplingIntegrator {
    bool m_remap;

public:
    NormalIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        m_remap = properties.get<bool>("remap", true);
    }
    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        if (its) {
            Vector normal = its.geometryNormal;
            if (m_remap) {
                normal = (normal + Vector(1)) / 2;
            }
            return Color(normal);
        } else {
            // If no intersection, return black
            return Color(0.0f);
        }
    }
    std::string toString() const override {
        return tfm::format(
            "NormalIntegrator[\n"
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

REGISTER_INTEGRATOR(NormalIntegrator, "normal")