
#include <lightwave.hpp>
namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 *
 * In local coordinates (before applying m_transform), the camera looks in
 * positive z direction [0,0,1]. Pixels on the left side of the image ( @code
 * normalized.x < 0 @endcode ) are directed in negative x direction ( @code
 * ray.direction.x < 0 ), and pixels at the bottom of the image ( @code
 * normalized.y < 0 @endcode ) are directed in negative y direction ( @code
 * ray.direction.y < 0 ).
 */
class Perspective : public Camera {
public:
    Perspective(const Properties &properties) : Camera(properties) {
        float fov           = properties.get<float>("fov");
        std::string fovAxis = properties.get<std::string>("fovAxis");

        float m_aspect_ratio;
        float m_tan_half_fov = tan(fov * 0.5f * Pi / 180.0f);

        if (fovAxis == "x") {
            m_aspect_ratio = m_resolution.y() / (float) m_resolution.x();
            mult_x         = m_tan_half_fov;
            mult_y         = m_aspect_ratio * m_tan_half_fov;
        }

        if (fovAxis == "y") {
            m_aspect_ratio = m_resolution.x() / (float) m_resolution.y();
            mult_x         = m_aspect_ratio * m_tan_half_fov;
            mult_y         = m_tan_half_fov;
        }
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {

        float px = normalized.x() * mult_x;
        float py = normalized.y() * mult_y;

        Ray ray;
        ray.origin    = Point(0.0f, 0.0f, 0.0f);
        ray.direction = Vector(px, py, 1.0f);

        // Transform the ray to world space
        ray = m_transform->apply(ray).normalized();

        // Return the sample
        return CameraSample{ .ray = ray, .weight = Color(1.0f) };
    }

    std::string toString() const override {
        return tfm::format(
            "Perspective[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  transform = %s,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            indent(m_transform));
    }

private:
    float mult_x;
    float mult_y;
};

} // namespace lightwave

REGISTER_CAMERA(Perspective, "perspective")
