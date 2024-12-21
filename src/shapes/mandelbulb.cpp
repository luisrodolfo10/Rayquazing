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

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        const int MAX_STEPS = 300000; // Number of steps for the SDF
        const float MAX_DIST =
            20.0f; // Maximum distance to consider an intersection
        const float Epsilon = 0.000001f;

        Point currentPos    = ray.origin;
        float totalDistance = 0;
        float dist          = std::numeric_limits<float>::max();
        for (int i = 0; i < MAX_STEPS && totalDistance < MAX_DIST; ++i) {
            dist = getMandelbulbDistance(currentPos);
            currentPos += ray.direction * dist;
            totalDistance += abs(dist);
            if (dist < Epsilon) {
                break;
            }
        }
        if (dist < Epsilon && totalDistance > Epsilon) {
            its.position       = currentPos;
            its.t              = totalDistance;
            its.geometryNormal = Vector(currentPos).normalized();
            its.shadingNormal  = Vector(currentPos).normalized();

            Vector up = { 0, 1, 0 };
            if (abs(its.geometryNormal[1]) > 0.99f) {
                up = { 1, 0, 0 }; // Use a horizontal vector if the normal is
                                  // close
            }
            its.tangent = its.geometryNormal.cross(up).normalized();
            return true;
        }
        return false;
    }

    // Code modified from:
    // http://blog.hvidtfeldts.net/index.php/2011/09/distance-estimated-3d-fractals-v-the-mandelbulb-different-de-approximations/
    float getMandelbulbDistance(const Point &p) const {
        Vector z = Vector(p);
        float dr = 1.0;
        float r  = 0.0;
        const float bailout =
            2.0f; // Threshold after which the point goes to infinity
        const int maxIterations = 20; // Times the fractal pattern will repeat

        for (int i = 0; i < maxIterations; i++) {
            r = z.length();
            if (r > bailout)
                break;

            // Spherical coordinates
            float theta = acos(z[0] / r);
            float phi   = atan2(z[1], z[0]);
            // Update the distance
            dr = pow(r, n - 1) * n * dr + 1.0;

            // Scale and rotate the point
            float zr = pow(r, n);
            theta    = theta * n;
            phi      = phi * n;

            // Put in cartesian coordinates
            z = zr * Vector(sin(theta) * cos(phi),
                            sin(phi) * sin(theta),
                            cos(theta));
            z += Vector(p);
        }
        return 0.5f * log(r) * r / dr;
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