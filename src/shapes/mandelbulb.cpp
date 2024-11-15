// #include "accel.hpp"
// #include <cmath>
// #include <lightwave.hpp>
// #include <memory>
// #include <vector>

// namespace lightwave {

// class SquaredPointCloud : public AccelerationStructure {
//     int n; // Power exponent for the SquaredPointCloud calculation
//     std::vector<Vector> pointCloud;

// public:
//     SquaredPointCloud(const Properties &properties) {
//         n           = properties.get<float>("n", 8);
//         int DIM     = 8;
//         float scale = 0.5f / (DIM - 1);
//         // initialise the pointcloud with the mandelbulb
//         for (int i = 0; i < DIM; i++) {
//             for (int j = 0; j < DIM; j++) {
//                 bool edge = false;
//                 for (int k = 0; k < DIM; k++) {
//                     // float x     = (i - DIM / 2) * scale;
//                     // float y     = (j - DIM / 2) * scale;
//                     // float z     = (k - DIM / 2) * scale;
//                     // float r     = sqrt(x * x + y * y + z * z);
//                     // float theta = atan2(sqrt(x * x + y * y), z);
//                     // float phi   = atan2(y, x);

//                     // Vector zeta = Vector(0, 0, 0);

//                     // int maxiterations = 10;
//                     // int n             = 8;
//                     // int iter          = 0;
//                     // Vector sphere     = Vector(0, 0, 0);

//                     // while (true) {
//                     //     r     = getRadius(sphere[0], sphere[1],
//                     sphere[2]);
//                     //     phi   = getPhi(sphere[0], sphere[1], sphere[2]);
//                     //     theta = getTheta(sphere[0], sphere[1], sphere[2]);

//                     //     float newx = pow(r, n) * sin(theta * n) * cos(phi
//                     *
//                     //     n); float newy = pow(r, n) * sin(theta * n) *
//                     sin(phi
//                     //     * n); float newz = pow(r, n) * cos(theta * n);

//                     //     sphere[0] = newx + x;
//                     //     sphere[1] = newy + y;
//                     //     sphere[2] = newz + z;

//                     //     if (sphere[0] > 5) {
//                     //         if (edge) {
//                     //             edge = false;
//                     //         }
//                     //         break;
//                     //     }

//                     //     iter++;
//                     //     if (iter > maxiterations) {
//                     //         if (not edge) {
//                     //             edge = true;
//                     pointCloud.push_back(Vector(i, j, k));
//                     //                         break;
//                     //                     }
//                     //                 }
//                     //             }
//                 }
//             }
//         }
//         buildAccelerationStructure();
//     }

//     float getRadius(float x, float y, float z) {
//         return sqrt(x * x + y * y + z * z);
//     }

//     float getTheta(float x, float y, float z) {
//         return atan2(sqrt(x * x + y * y), z);
//     }

//     float getPhi(float x, float y, float z) { return atan2(y, z); }

//     bool intersect(int primitiveIndex, const Ray &ray, Intersection &its,
//                    Sampler &rng) const override {
//         const Vector &point  = pointCloud[primitiveIndex];
//         const int MAX_STEPS  = 50;
//         const float MAX_DIST = 15.0f;
//         const float Epsilon  = 0.05f;

//         Point currentPos    = ray.origin;
//         float totalDistance = 0.1f;

//         for (int i = 0; i < MAX_STEPS && totalDistance < MAX_DIST; ++i) {
//             float dist = (currentPos - point).length;

//             if (dist < Epsilon) {
//                 its.position       = currentPos;
//                 its.t              = totalDistance;
//                 its.geometryNormal = Vector(currentPos);
//                 its.shadingNormal  = Vector(currentPos);
//                 return true;
//             }

//             currentPos += ray.direction * dist;
//             totalDistance += dist;
//         }
//         return false;
//     }

//     float getDistance(const Point &p) const {
//         float minDistance = std::numeric_limits<float>::max();

//         for (const Vector &point : pointCloud) {
//             float distance = Vector(p - point).length();
//             if (distance < minDistance) {
//                 minDistance = distance;
//             }
//         }
//         return minDistance;
//     }

//     Bounds getBoundingBox() const override {
//         return Bounds(Point(-1, -1, -1), Point(1, 1, 1));
//     }

//     Point getCentroid() const override { return Point(0, 0, 0); }

//     AreaSample sampleArea(Sampler &rng) const override{ NOT_IMPLEMENTED }

//     std::string toString() const override {
//         return "SquaredPointCloud";
//     }
// };

// } // namespace lightwave

// REGISTER_SHAPE(SquaredPointCloud, "SquaredPointCloud");
