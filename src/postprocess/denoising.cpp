#ifdef LW_WITH_OIDN
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
        // m_input   = properties.get<Image>("input");
        m_normals = properties.getOptional<Image>("normal");
        m_albedo  = properties.getOptional<Image>("albedo");
    }

    void execute() override {
        Point2i m_resolution = m_input->resolution();
        int width            = m_resolution.x();
        int height           = m_resolution.y();
        // Create an Open Image Denoise device
        oidn::DeviceRef device = oidn::newDevice(); // CPU or GPU if available
        // oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
        device.commit();

        // Allocate buffers using OIDN
        oidn::BufferRef colorBuf = device.newBuffer(
            width * height * 3 * sizeof(float), oidn::Storage::Host);
        oidn::BufferRef outputBuf = device.newBuffer(
            width * height * 3 * sizeof(float), oidn::Storage::Host);

        // Copy input data to the OIDN buffer
        std::memcpy(colorBuf.getData(),
                    m_input->data(),
                    width * height * 3 * sizeof(float));

        // Debugging: Check if input data is valid
        std::cout << "[debug] First pixel (input): "
                  << static_cast<float *>(colorBuf.getData())[0] << ", "
                  << static_cast<float *>(colorBuf.getData())[1] << ", "
                  << static_cast<float *>(colorBuf.getData())[2] << std::endl;

        // Optional buffers for auxiliary features
        oidn::BufferRef albedoBuf, normalBuf;
        if (m_albedo) {
            albedoBuf = device.newBuffer(width * height * 3 * sizeof(float),
                                         oidn::Storage::Host);
            std::memcpy(albedoBuf.getData(),
                        m_albedo->data(),
                        width * height * 3 * sizeof(float));
        }
        if (m_normals) {
            normalBuf = device.newBuffer(width * height * 3 * sizeof(float),
                                         oidn::Storage::Host);
            std::memcpy(normalBuf.getData(),
                        m_normals->data(),
                        width * height * 3 * sizeof(float));
        }

        // Create a denoising filter
        oidn::FilterRef filter =
            device.newFilter("RT"); // "RT" = ray tracing filter
        filter.setImage("color", colorBuf, oidn::Format::Float3, width, height);

        if (m_albedo) {
            filter.setImage(
                "albedo", albedoBuf, oidn::Format::Float3, width, height);
        }
        if (m_normals) {
            filter.setImage(
                "normal", normalBuf, oidn::Format::Float3, width, height);
        }

        filter.setImage(
            "output", outputBuf, oidn::Format::Float3, width, height);
        filter.set("hdr", true);
        filter.commit();

        // Execute denoising
        filter.execute();

        // Check for errors
        const char *errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[OIDN ERROR] " << errorMessage << std::endl;
        }

        // Debugging: Check if output data is valid
        std::cout << "[debug] First pixel (output before copy): "
                  << static_cast<float *>(outputBuf.getData())[0] << ", "
                  << static_cast<float *>(outputBuf.getData())[1] << ", "
                  << static_cast<float *>(outputBuf.getData())[2] << std::endl;

        // Copy denoised data back to output
        // Segmentation fault
        m_output->initialize(m_resolution);
        std::memcpy(m_output->data(),
                    outputBuf.getData(),
                    width * height * 3 * sizeof(float));

        // Debugging: Check if final output data is valid
        std::cout << "[debug] First pixel (final output): "
                  << m_output->data()[0] << ", " << m_output->data()[1] << ", "
                  << m_output->data()[2] << std::endl;

        m_output->save();
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
#endif
