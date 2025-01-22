#include <OpenImageDenoise/oidn.hpp>
#include <lightwave.hpp>

namespace lightwave {
// Reference: https://www.openimagedenoise.org/documentation.html
class Denoising : public Postprocess {
    // normals and albedo are images in exporter.py
    ref<Image> m_normals;
    ref<Image> m_albedo;

public:
    Denoising(const Properties &properties) : Postprocess(properties) {
        m_normals = properties.get<Image>("normals");
        m_albedo  = properties.get<Image>("albedo");
        m_input   = properties.get<Image>("noisy");
    }

    void execute() override {
        Point2i m_resolution = m_input->resolution();
        int width            = m_resolution.x();
        int height           = m_resolution.y();
        // Create an Open Image Denoise device
        oidn::DeviceRef device = oidn::newDevice(); // CPU or GPU if available
        // oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
        device.commit();

        // Create buffers for input/output images accessible by both host (CPU)
        // and device (CPU/GPU)
        oidn::BufferRef colorBuf =
            device.newBuffer(width * height * 3 * sizeof(float));
        oidn::BufferRef albedoBuf =
            device.newBuffer(width * height * 3 * sizeof(float));
        oidn::BufferRef normalBuf =
            device.newBuffer(width * height * 3 * sizeof(float));
        oidn::BufferRef outputBuf =
            device.newBuffer(width * height * 3 * sizeof(float));

        // Create a filter for denoising a beauty (color) image using optional
        // auxiliary images too This can be an expensive operation, so try no to
        // create a new filter for every image!
        oidn::FilterRef filter =
            device.newFilter("RT"); // generic ray tracing filter
        filter.setImage("color", colorBuf, oidn::Format::Float3, width, height);
        filter.setImage(
            "albedo", albedoBuf, oidn::Format::Float3, width, height);
        filter.setImage(
            "normals", normalBuf, oidn::Format::Float3, width, height);
        filter.setImage("output",
                        outputBuf,
                        oidn::Format::Float3,
                        width,
                        height); // denoised

        filter.set("hdr", true);
        filter.commit();

        // Filter the beauty image
        filter.execute();

        // Check for errors
        const char *errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cout << "Error: " << errorMessage << std::endl;
        }

        float *outputPtr  = (float *) outputBuf.getData();
        Color *outputData = m_output->data();

        for (int i = 0; i < width * height; ++i) {
            outputData[i] = Color(
                outputPtr[i * 3], outputPtr[i * 3 + 1], outputPtr[i * 3 + 2]);
        }
    };

    std::string toString() const override {
        return tfm::format(
            "Denoising[\n"
            "  m_normals = %s\n"
            "  m_albedo = %s\n"
            "]",
            m_normals,
            m_albedo);
    }
};

} // namespace lightwave

REGISTER_POSTPROCESS(Denoising, "denoising")
