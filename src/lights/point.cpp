#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {
private:
    Point position;
    Color power;

public:
    PointLight(const Properties &properties) : Light(properties) {
        Point position = properties.get<Point>("position");
        Color power    = properties.get<Color>("power");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        Vector direction = Vector(position - origin);
        float distance   = direction.length();
        Color weight     = power * Inv4Pi / (distance * distance);

        return {
            .wi       = direction,
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
