#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {
private:
    Point m_position;
    Color m_power;

public:
    PointLight(const Properties &properties) : Light(properties) {
        m_position = properties.get<Point>("position");
        m_power    = properties.get<Color>("power");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        Vector direction = Vector(m_position - origin);
        float distance   = direction.length();
        Color weight     = m_power * Inv4Pi / (distance * distance);

        return {
            .wi       = direction.normalized(),
            .weight   = weight,
            .distance = distance,
        };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "PointLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(PointLight, "point")
