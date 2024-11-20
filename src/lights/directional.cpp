#include <lightwave.hpp>

namespace lightwave {

class DirectionalLight final : public Light {
private:
    Vector m_direction;
    Color m_intensity;

public:
    DirectionalLight(const Properties &properties) : Light(properties) {
        m_direction = properties.get<Vector>("direction");
        m_intensity = properties.get<Color>("intensity");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        DirectLightSample directLight;
        directLight.wi       = m_direction.normalized();
        directLight.weight   = m_intensity;
        directLight.distance = Infinity;
        return directLight;
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "DirectionalLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(DirectionalLight, "directional")
