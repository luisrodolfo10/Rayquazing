#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {
private:
    ref<Instance> m_instance;

public:
    AreaLight(const Properties &properties)
        : Light(properties), m_instance(properties.getChild<Instance>()) {}

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        // sample a position on the area light source
        AreaSample area_sample = m_instance->sampleArea(rng);

        // calculate direction and distance to the sampled point

        Vector direction  = (area_sample.position - origin).normalized();
        float distance_sq = (area_sample.position - origin).lengthSquared();

        // Vector direction  = (origin - area_sample.position).normalized();
        // float distance_sq = (origin - area_sample.position).lengthSquared();

        DirectLightSample direct_light;
        direct_light.wi       = direction;
        direct_light.distance = std::sqrt(distance_sq);

        // Evaluate intensity of the light at the sampled point

        Color intensity =
            m_instance->emission()
                ->evaluate(area_sample.uv,
                           area_sample.shadingFrame().toLocal(direction))
                .value;

        // Color intensity =
        //     m_instance->emission()->evaluate(area_sample.uv,
        //     -direction).value;

        // adjust intensity by the cosine of the angle and the distance
        // squared

        // intensity = intensity * Frame::absCosTheta(direction);

        intensity =
            intensity *
            Frame::absCosTheta(area_sample.shadingFrame().toLocal(direction));

        // compute weight
        direct_light.weight = intensity / distance_sq;
        // direct_light.weight *= area_sample.pdf;
        direct_light.weight /= area_sample.pdf;

        return direct_light;
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format("AreaLight[\n]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "area")