#include "vipsreader.h"
#include <QtGlobal>

#include <vips/vips8>
#include <glib.h>

#include <cstddef>
#include <string>
#include <utility>

void VipsReader::init()
{
    VIPS_INIT(nullptr);
}

void VipsReader::shutdown()
{
    vips_shutdown();
}

VipsReader::ReadResult VipsReader::read(const QString &fileName)
{
        try
    {
        vips::VImage in = vips::VImage::new_from_file(fileName.toUtf8().constData(),
                                                  vips::VImage::option()->set("access", VIPS_ACCESS_SEQUENTIAL));

        if (in.interpretation() == VIPS_INTERPRETATION_ERROR)
        {
            throw vips::VError("Vips Error: Cannot interpret image");
        }

        if (in.bands() > 4 && in.has_alpha())
        {
            in = in.extract_band(0, vips::VImage::option()->set("n", 4));
        } else if (in.bands() > 3 && !in.has_alpha())
        {
            in = in.extract_band(0, vips::VImage::option()->set("n", 3));
        }

        if (in.interpretation() != VIPS_INTERPRETATION_sRGB)
        {
            try
            {
                in = in.colourspace(VIPS_INTERPRETATION_sRGB);
            } catch (const vips::VError&)
            {
                // ignore errors
            }
        }

        if (!in.has_alpha())
        {
            in = in.bandjoin_const({255.0});
        }

        size_t buffer_size;
        void *buffer = in.write_to_memory(&buffer_size);

        auto cleanup = [](void *info)
        {
            g_free(info);
        };

        QImage resultImage(static_cast<const uchar *>(buffer), in.width(), in.height(), in.width() * in.bands(), QImage::Format_RGBA8888, cleanup, buffer);

        QByteArray colorProfile;
        if (in.get_typeof("icc-profile-data") != 0) { // G_TYPE_INVALID is 0
            const void *profile_data = nullptr;
            size_t profile_size = 0;
            if (vips_image_get_blob(in.get_image(), "icc-profile-data", &profile_data, &profile_size) == 0) {
                if (profile_size > 0 && profile_data) {
                    colorProfile = QByteArray(static_cast<const char*>(profile_data), static_cast<int>(profile_size));
                }
            }
        }

        return {std::move(resultImage), std::move(colorProfile), QString()};
    }
    catch (const vips::VError &e)
    {
        return {QImage(), QByteArray(), QString::fromStdString(e.what())};
    }
}
