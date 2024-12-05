#include <lightwave.hpp>

namespace lightwave {

class Lambertian : public Emission {
    ref<Texture> m_emission;

public:
    Lambertian(const Properties &properties) {
        m_emission = properties.get<Texture>("emission");
    }

    EmissionEval evaluate(const Point2 &uv, const Vector &wo) const override {
        if (wo.z() <= 0) {
            return EmissionEval{ Color(0) };
        }
        Color emission_color = Color(m_emission->evaluate(uv));
        return EmissionEval{ emission_color };
    }

    std::string toString() const override {
        return tfm::format(
            "Lambertian[\n"
            "  emission = %s\n"
            "]",
            indent(m_emission));
    }
};

} // namespace lightwave

REGISTER_EMISSION(Lambertian, "lambertian")
