#include <lightwave.hpp>

namespace lightwave {

class PathTracerIntegrator : public SamplingIntegrator {
    int m_maxdepth;

public:
    PathTracerIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        m_maxdepth = properties.get<int>("depth", 2);
    }
    Color Li(const Ray &ray, Sampler &rng) override {
        // Determine if the ray intersects any surfaces in the scene.
        Color throughput(1.0f);
        Ray currentRay = ray;
        int depth      = 0;
        Color contribution(0.0f);
        while (depth < m_maxdepth) {
            Intersection its = m_scene->intersect(currentRay, rng);
            contribution += throughput * its.evaluateEmission().value;
            if (!its || depth >= m_maxdepth - 1) {
                break;
            }
            LightSample lightSample = m_scene->sampleLight(rng);
            if (lightSample.probability > 0) {
                DirectLightSample directSample =
                    lightSample.light->sampleDirect(its.position, rng);

                // Trace a secondary ray in the direction of the light
                Ray secondaryRay;
                secondaryRay.origin    = its.position;
                secondaryRay.direction = directSample.wi;
                Intersection secondaryIts =
                    m_scene->intersect(secondaryRay, rng);
                if (!secondaryIts || secondaryIts.t > directSample.distance) {
                    BsdfEval bsdf = its.evaluateBsdf(directSample.wi);
                    if (!bsdf.isInvalid()) {
                        contribution += throughput * directSample.weight *
                                        bsdf.value / lightSample.probability;
                    }
                }
            }

            BsdfSample bsdfSample = its.sampleBsdf(rng);
            if (bsdfSample.isInvalid()) {
                break;
            }
            currentRay = Ray(its.position, bsdfSample.wi.normalized());
            throughput *= bsdfSample.weight;
            depth++;
        }
        return contribution;
    }
    std::string toString() const override {
        return tfm::format(
            "DirectIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(PathTracerIntegrator, "pathtracer")