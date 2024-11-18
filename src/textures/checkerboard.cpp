#include <lightwave.hpp>

namespace lightwave {

class CheckerboardTexture : public Texture {
    Color color0;
    Color color1;
    Point2 scale;

public:
    CheckerboardTexture(const Properties &properties) {
        // define the two colors of the checkerboard pattern, and a
        // two-component scale vector used to rescale the UV coordinates and
        // achieve various sized patterns
        color0 = properties.get<Color>("color0", Color(0));
        color1 = properties.get<Color>("color1", Color(1));
        scale  = properties.get<Point2>("scale", Point2(1, 1));
    }

    Color evaluate(const Point2 &uv) const override {
        Point2 scaledUV = Point2(uv.x() * scale.x(), uv.y() * scale.y());

        int uCheck = int(std::floor(scaledUV.x())) % 2;
        int vCheck = int(std::floor(scaledUV.y())) % 2;

        bool isCheckerboard = ((uCheck + vCheck) % 2 == 0);

        if (isCheckerboard) {
            return color0;
        } else {
            return color1;
        }
    }

    std::string toString() const override {
        return tfm::format(
            "CheckerboardTexture[\n"
            "  color0 = %s\n",
            "  color1 = %s\n",
            "  scale = %s\n",
            "]",
            indent(color0),
            indent(color1),
            indent(scale));
    }
};

} // namespace lightwave

REGISTER_TEXTURE(CheckerboardTexture, "checkerboard")
