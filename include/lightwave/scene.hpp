/**
 * @file sampler.hpp
 * @brief Contains the Scene interface and related structures.
 */

#pragma once

#include <lightwave/core.hpp>
#include <vector>

namespace lightwave {

/// @brief The result of asking the scene to pick a random light source using
/// @ref Scene::sampleLight .
struct LightSample {
    /// @brief The light source that has been picked.
    const Light *light;
    /// @brief The probability of this light source having been picked.
    float probability;

    /// @brief Indicates light sampling has failed (e.g., the scene has no
    /// explicit lights).
    static LightSample invalid() {
        return {
            .light       = nullptr,
            .probability = 0,
        };
    }

    bool isInvalid() const { return light == nullptr; }
    explicit operator bool() const { return !isInvalid(); }
};

/// @brief Scenes are the input to rendering algorithms: They contain all
/// geometry, materials, lights and the camera.
class Scene : public Object {
    /// @brief The camera from which the image is to be rendered.
    ref<Camera> m_camera;
    /// @brief The geometry of the scene that should be rendered (typically an
    /// acceleration structure with instances in it).
    ref<Shape> m_shape;
    /// @brief An optional background light, which provides color when rays exit
    /// the scene.
    ref<BackgroundLight> m_background;

    class LightSampling;
    /// @brief Contains all necessary data structures to randomly pick light
    /// sources.
    ref<LightSampling> m_lightSampling;

public:
    Scene(const Properties &properties);
    std::string toString() const override;

    /// @brief The camera from which the image is to be rendered.
    Camera *camera() const { return m_camera.get(); }

    /// @brief Finds the closest intersection of the scene for a given ray.
    Intersection intersect(const Ray &ray, Sampler &rng) const;
    /// @brief Reports whether any intersection up to a given maximal distance
    /// exists (used for testing visibility of light sources).
    bool intersect(const Ray &ray, float tMax, Sampler &rng) const;

    /// @brief Reports whether at least one light exists that could be sampled.
    bool hasLights() const;

    /// @brief Randomly picks a light from the list of sampleable light sources.
    LightSample sampleLight(Sampler &rng) const;
    /// @brief Returns the bounding box of the scene geometry.
    Bounds getBoundingBox() const;
};

} // namespace lightwave
