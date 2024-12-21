
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
class Thinlens : public Camera {
public:
    Thinlens(const Properties &properties) : Camera(properties) {
        // Thin-lens parameters
        m_lens_radius = properties.get<float>(
            "lensRadius", 0.0f); // default to 0 (pinhole camera)
        m_focal_distance = properties.get<float>(
            "focalDistance", 1.0f); // default focus at 1 unit
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

        Vector direction(px, py, 1.0f);
        direction = direction.normalized();

        Point pFocus = direction * m_focal_distance;
        Point origin = { 0.0f, 0.0f, 0.0f };

        if (m_lens_radius > 0.0f) {
            Point2 pLens = SampleDisk(rng);
            pLens =
                Point2(pLens.x() * m_lens_radius, pLens.y() * m_lens_radius);
            origin = Point(pLens.x(), pLens.y(), 0.0f);
        }

        Ray ray;
        ray.origin    = origin;
        ray.direction = (pFocus - origin).normalized();
        ray           = m_transform->apply(ray);

        ray = ray.normalized();

        return CameraSample{ .ray = ray, .weight = Color(1.0f) };
        // code based on the PBR book  6.3
        // https://www.pbr-book.org/3ed-2018/Camera_Models/Projective_Camera_Models
    }

    std::string toString() const override {
        return tfm::format(
            "Perspective[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  lens radius = %d,\n"
            "  focal distance= %d,\n"
            "  transform = %s,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            m_lens_radius,
            m_focal_distance,
            indent(m_transform));
    }

    Point2 SampleDisk(Sampler &rng) const {
        float r = sqrt(rng.next());
        float theta = 2 * Pi * rng.next();
        return Point2(r * cos(theta), r * sin(theta));
    }

private:
    float mult_x;
    float mult_y;
    float m_lens_radius;
    float m_focal_distance;
};

} // namespace lightwave

REGISTER_CAMERA(Thinlens, "thinlens")
