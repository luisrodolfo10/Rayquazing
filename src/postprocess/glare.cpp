#include <cmath>
#include <lightwave.hpp>
#include <vector>

namespace lightwave {

class Glare : public Postprocess {
    float threshold;
    float intensity;
    int numStreaks;
    float streakLength;

public:
    Glare(const Properties &properties) : Postprocess(properties) {
        threshold    = properties.get<float>("threshold", 1.0f);
        intensity    = properties.get<float>("intensity", 0.0f);
        numStreaks   = properties.get<int>("num_streaks", 1);
        streakLength = properties.get<float>("streak_length", 0.1f);
    }

    void execute() override {
        Point2i resolution = m_input->resolution();
        int width          = resolution.x();
        int height         = resolution.y();

        m_output->initialize(resolution);

        // Image with bright areas
        Image glareAreas(resolution);

        // Extract bright areas where glare is applied
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Point2i pixel(x, y);
                Color color      = m_input->get(pixel);
                float brightness = (color.r() + color.g() + color.b()) / 3.0f;
                if (brightness > threshold) {
                    std::cout << "Brightness" << brightness << std::endl;
                    glareAreas.get(pixel) = color;
                } else {
                    glareAreas.get(pixel) = Color(0.0f);
                }
            }
        }

        Image glareEffect = applyGlareFilter(glareAreas, width, height);

        // Glare effect back into the image
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Point2i pixel(x, y);
                Color original   = m_input->get(pixel);
                Color glare      = glareEffect.get(pixel);
                Color finalColor = original + glare * intensity;

                m_output->get(pixel) = finalColor;
            }
        }

        m_output->save();
    }

    std::string toString() const override {
        return tfm::format(
            "Glare[\n"
            "  threshold = %f\n"
            "  intensity = %f\n"
            "  num_streaks = %d\n"
            "  streak_length = %f\n"
            "]",
            threshold,
            intensity,
            numStreaks,
            streakLength);
    }

private:
    // Image applyGlareFilter(const Image &input, int width, int height) {
    //     Image output(Point2i(width, height));

    //     // Define glare angles
    //     std::vector<float> angles;
    //     for (int i = 0; i < numStreaks; i++) {
    //         angles.push_back(i * (2 * Pi / numStreaks));
    //     }

    //     for (int y = 0; y < height; y++) {
    //         for (int x = 0; x < width; x++) {
    //             Color sum(0.0f);

    //             // Apply streaks in different directions
    //             for (float angle : angles) {
    //                 float cosA = cos(angle);
    //                 float sinA = sin(angle);

    //                 for (int s = 1; s < streakLength; s++) {
    //                     int dx = static_cast<int>(s * cosA);
    //                     int dy = static_cast<int>(s * sinA);

    //                     int nx = x + dx;
    //                     int ny = y + dy;

    //                     if (nx >= 0 && nx < width && ny >= 0 && ny < height)
    //                     {
    //                         float alongFactor =
    //                             1.0f -
    //                             (float(s) / streakLength); // Linear falloff
    //                                                        // along the
    //                                                        streak

    //                         // Perpendicular spread: Quadratic falloff
    //                         float maxWidth =
    //                             1.0f + (s * 0.2f); // Wider with distance
    //                         for (float spread = -maxWidth; spread <=
    //                         maxWidth;
    //                              spread++) {
    //                             int spreadX =
    //                                 static_cast<int>(nx + spread * sinA);
    //                             int spreadY =
    //                                 static_cast<int>(ny - spread * cosA);

    //                             if (spreadX >= 0 && spreadX < width &&
    //                                 spreadY >= 0 && spreadY < height) {
    //                                 float perpFactor = std::exp(
    //                                     -0.5f * (spread * spread) /
    //                                     (maxWidth * maxWidth)); // Gaussian
    //                                                             // falloff

    //                                 sum +=
    //                                     input.get(Point2i(spreadX, spreadY))
    //                                     * (alongFactor * perpFactor);
    //                             }
    //                         }
    //                     }
    //                 }
    //             }

    //             output.get(Point2i(x, y)) = sum;
    //         }
    //     }

    //     return output;
    // }
    Image applyGlareFilter(const Image &input, int width, int height) {
        Image output(Point2i(width, height));

        // Define glare angles
        std::vector<float> angles;
        for (int i = 0; i < numStreaks; i++) {
            angles.push_back(i * (2 * Pi / numStreaks));
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Color sum(0.0f);

                // Apply streaks in different directions
                for (float angle : angles) {
                    for (int s = 1; s < streakLength; s++) {
                        int dx = static_cast<int>(s * cos(angle));
                        int dy = static_cast<int>(s * sin(angle));

                        int nx = x + dx;
                        int ny = y + dy;

                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            sum += input.get(Point2i(nx, ny)) *
                                   (1.0f - (float(s) / streakLength));
                        }
                    }
                }

                output.get(Point2i(x, y)) = sum;
            }
        }

        return output;
    }
};

} // namespace lightwave

REGISTER_POSTPROCESS(Glare, "glare")
