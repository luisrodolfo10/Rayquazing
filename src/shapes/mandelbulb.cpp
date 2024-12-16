#include <cmath>
#include <lightwave.hpp>
#include <memory>
#include <vector>

namespace lightwave {

class MandelBulb : public Shape {
    int n; // Power exponent for the MandelBulb calculation

public:
    MandelBulb(const Properties &properties) {
        n = properties.get<float>("n", 8);
    }

    float getRadius(float x, float y, float z) const {
        return sqrt(x * x + y * y + z * z);
    }

    float getTheta(float x, float y, float z) const {
        return atan2(sqrt(x * x + y * y), z);
    }

    float getPhi(float x, float y, float z) const { return atan2(y, z); }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        const int MAX_STEPS  = 300;
        const float MAX_DIST = 20.0f;
        const float Epsilon  = 0.0001f;

        Point currentPos    = ray.origin;
        float totalDistance = 0.1f;
        float dist          = std::numeric_limits<float>::max();
        for (int i = 0; i < MAX_STEPS && totalDistance < MAX_DIST; ++i) {
            dist = getMandelbulbDistance(currentPos);
            currentPos += ray.direction * dist;
            totalDistance += abs(dist);
            if (dist < Epsilon) {
                break;
            }
        }
        if (dist < Epsilon) {
            its.position       = currentPos;
            its.t              = totalDistance;
            its.geometryNormal = Vector(currentPos);
            its.shadingNormal  = Vector(currentPos);
            return true;
        }
        return false;
    }

    float getMandelbulbDistance(const Point &p) const {
        Vector z                = Vector(p);
        float dr                = 1.0;
        float r                 = 0.0;
        const float bailout     = 2.0f;
        const int maxIterations = 16;

        for (int i = 0; i < maxIterations; i++) {
            r = getRadius(z[0], z[1], z[2]);
            if (r > bailout)
                break;

            // Spherical coordinates
            float theta = getTheta(z[0], z[1], z[2]);
            float phi   = getPhi(z[0], z[1], z[2]);

            // Update z
            float zr = std::pow(r, n);
            z = Vector(zr * std::sin(theta * n) * std::cos(phi * n) + p[0],
                       zr * std::sin(theta * n) * std::sin(phi * n) + p[1],
                       zr * std::cos(theta * n) + p[2]);

            // Update the distance
            dr = std::pow(r, n - 1) * n * dr + 1.0;
        }
        return 0.5f * std::log(r) * r / dr;
    }

    Bounds getBoundingBox() const override {
        return Bounds(Point(-1, -1, -1), Point(1, 1, 1));
    }

    Point getCentroid() const override { return Point(0, 0, 0); }

    AreaSample sampleArea(Sampler &rng) const override{ NOT_IMPLEMENTED }

    std::string toString() const override {
        return "MandelBulb";
    }
};

} // namespace lightwave

REGISTER_SHAPE(MandelBulb, "MandelBulb");