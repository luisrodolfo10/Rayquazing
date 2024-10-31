#include <lightwave.hpp>

namespace lightwave {

class AovGridIntegrator : public SamplingIntegrator {
    enum Variable {
        AovNormals,
        AovDistance,
        AovBvh,
        AovUv,
    } m_variable;
    float m_scale;
    /// @brief Whether to show the grid, or only output the ray's direction as
    /// color.
    bool m_showGrid;
    /// @brief The color of the grid, if the grid is shown.
    Color m_gridColor;
    /// @brief The frequency of the grid spacing, if the grid is shown.
    float m_gridFrequency;

public:
    AovGridIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        // clang-format off
        m_variable = properties.getEnum<Variable>("variable", {
            { "normals",  AovNormals  },
            { "distance", AovDistance },
            { "bvh",      AovBvh      },
            { "uv",       AovUv       },
        });
        m_scale = properties.get<float>("scale", 1.f);
        // clang-format on
        m_showGrid      = properties.get<bool>("grid", true);
        m_gridColor     = properties.get<Color>("gridColor", Color::black());
        m_gridFrequency = properties.get<float>("gridFrequency", 0.2);
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);

        Vector d  = ray.direction;
        float fcc = 0.25f; // floor/ceil/sky color

        switch (m_variable) {
        case AovNormals:
            if (its) {
                return (Color(its.shadingNormal) + Color(1)) / 2;
            } else {
                if (m_showGrid) {
                    // intersect the ray with a grid at z=+1
                    Point o = ray.origin;
                    // Only apply grid if the ray points downward (for a floor
                    // grid at z=0)
                    if (d.z() < 0) { // Ensures grid is only drawn below the
                                     // camera
                        // Removed -1 from camera.cpp, to make the plane at z=0
                        if (std::fmod(abs(d.x() * (-o.z()) / d.z() + o.x()) *
                                          m_gridFrequency,
                                      1.0f) < 0.1f)
                            return m_gridColor;
                        if (std::fmod(abs(d.y() * (-o.z()) / d.z() + o.y()) *
                                          m_gridFrequency,
                                      1.0f) < 0.1f)
                            return m_gridColor;
                        // if the grid is below the camera (floor), change the
                        // color
                        fcc = 0.5f;
                    }
                    d = (d + Vector(1)) / 2;
                }
                return Color(fcc);
            }
        case AovDistance:
            return its ? Color(its.t) : Color(Infinity);
        case AovBvh:
            return Color(its.stats.bvhCounter / m_scale,
                         its.stats.primCounter / m_scale,
                         0);
        case AovUv:
            return its ? Color(its.uv.x(), its.uv.y(), 0) : Color(0);
        default:
            return Color(0);
        }
    }

    std::string toString() const override {
        return tfm::format(
            "AovIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(AovGridIntegrator, "aovGrid")
