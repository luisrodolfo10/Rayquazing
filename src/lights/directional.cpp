#include <lightwave.hpp>

namespace lightwave {

class DirectionalLight final : public Light {
private:
    Vector direction;
    Color intensity;

public:
    DirectionalLight(const Properties &properties) : Light(properties) {
        Vector direction = properties.get<Vector>("direction");
        Color intensity  = properties.get<Color>("intensity");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        float distance = direction.length();
        Color weight   = intensity * Inv4Pi;

        return {
            .wi       = direction,
            .weight   = weight,
            .distance = Infinity,
        };
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
