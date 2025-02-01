#include <cmath>
#include <lightwave.hpp>

namespace lightwave {

class Sphere : public Shape {
    float radius;

    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;
        float phi     = atan2(-position[2], position[0]);
        float theta   = acos(position[1]);
        float u       = (phi + Pi) * Inv2Pi;
        float v       = theta / Pi;
        surf.uv       = Point2(u, v);

        surf.geometryNormal =
            Vector(position).normalized(); // vector from sphere
        surf.shadingNormal = surf.geometryNormal;
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
        // Ofsett ray origin to avoid numerical issues and self intersections
        Ray rayShifted = Ray(ray.origin + ray.direction.normalized() * Epsilon,
                             ray.direction.normalized());
        Vector oc      = Vector(rayShifted.origin);
        float a        = 1;
        float b        = 2.0f * oc.dot(rayShifted.direction);
        float c        = oc.dot(oc) - radius * radius;
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
                const Point position = rayShifted(t);
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

    AreaSample sampleArea(Sampler &rng) const override {
        float phi   = rng.next() * 2.0f * Pi; // Azimuthal angle [0, 2π]
        float theta = std::acos(2.0f * rng.next() - 1.0f); // Polar angle [0, π]

        float x = radius * std::sin(theta) * std::cos(phi);
        float y = radius * std::sin(theta) * std::sin(phi);
        float z = radius * std::cos(theta);

        Point position(x, y, z);

        AreaSample sample;
        sample.position = position;
        populate(sample, position);

        float area = 4.0f * Pi * radius * radius; // Surface area of the sphere
        float pdf = 1.0f / area; // Uniform distribution on the sphere's surface
        sample.pdf = pdf;

        return sample;
    }

    std::string toString() const override {
        return "Sphere[radius=" + std::to_string(radius) + "]";
    }
};

} // namespace lightwave

REGISTER_SHAPE(Sphere, "sphere");