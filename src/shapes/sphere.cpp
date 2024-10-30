#include <cmath>
#include <lightwave.hpp>

namespace lightwave {

class Sphere : public Shape {
    float radius;

    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;

        surf.geometryNormal = Vector(position); // vector from sphere
        surf.shadingNormal  = surf.geometryNormal;
        // Any vector to calculate the tangent
        Vector up = { 0, 1, 0 };
        if (abs(surf.geometryNormal[1]) > 0.99f) {
            up = { 1, 0, 0 }; // Use a horizontal vector if the normal is close
        }
        surf.tangent = surf.geometryNormal.cross(up).normalized();
    }

public:
    Sphere(const Properties &properties) {
        radius = properties.get<float>("radius", 1.0f);
    }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        Vector oc          = Vector(ray.origin);
        float a            = ray.direction.dot(ray.direction);
        float b            = 2.0f * oc.dot(ray.direction);
        float c            = oc.dot(oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant < 0) {
            return false; // No intersection
        } else {
            float sqrtDiscriminant = std::sqrt(discriminant);
            float t1               = (-b - sqrtDiscriminant) / (2.0f * a);
            float t2               = (-b + sqrtDiscriminant) / (2.0f * a);
            float t                = (t1 < t2 && t1 > Epsilon) ? t1 : t2;
            // Using smaller t

            // Checking previous t -> using the closest
            if (t > Epsilon && t < its.t) {
                its.t                = t;
                const Point position = ray(t);
                populate(its, position); // Fill in the intersection details
                return true;
            }
        }
        return false;
    }

    Bounds getBoundingBox() const override {
        return Bounds(Point(-radius, -radius, -radius),
                      Point(radius, radius, radius));
    }

    Point getCentroid() const override {
        return Point(0, 0, 0); // Sphere is centered at the origin
    }

    AreaSample sampleArea(Sampler &rng) const override{ NOT_IMPLEMENTED }

    std::string toString() const override {
        return "Sphere[radius=" + std::to_string(radius) + "]";
    }
};

} // namespace lightwave

REGISTER_SHAPE(Sphere, "sphere");