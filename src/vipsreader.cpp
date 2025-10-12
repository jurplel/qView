#include "vipsreader.h"
#include <QtGlobal>

#include <vips/vips8>
#include <vips/memory.h>
#include <vips/operation.h>
#include <glib.h>

#include <cstddef>
#include <string>


void VipsReader::init()
{
    if (VIPS_INIT(nullptr) != 0)
    {
        qFatal("Failed to initialize VIPS");
    }
    // TODO: Tweak caching depending on preloading mode, maybe fully
    // configurable in future
    vips_cache_set_max_mem(1024 * 1024 * 1024);
    vips_cache_set_max(10000);
}

void VipsReader::shutdown()
{
    vips_shutdown();
}

// TODO: Lazily load full-res image by using isThumbnail
vips::VImage VipsReader::readFile(const QString &fileName, bool isThumbnail)
{
    // TODO: magick fallback is not working well, avif for example didn't
    // seem to work
    vips::VImage in = isThumbnail
            ? vips::VImage::thumbnail(fileName.toUtf8().constData(), 256) // TODO: magic number
            : vips::VImage::new_from_file(fileName.toUtf8().constData(),
                                            vips::VImage::option()->set("access", VIPS_ACCESS_SEQUENTIAL));

    if (in.interpretation() == VIPS_INTERPRETATION_ERROR) {
        throw vips::VError("Vips Error: Cannot interpret image");
    }


    return in;
}

size_t VipsReader::getMemoryUsage()
{
    return vips_tracked_get_mem();
}

int VipsReader::getCacheSize()
{
    return vips_cache_get_size();
}

void VipsReader::preload(const QString &fileName)
{
    vips::VImage image = readFile(fileName, false);
    (void)image.width(); // todo stupid
    // TODO: this basically does nothing afaik, since we don't actually trigger the pipeline
}

VipsReader::ReadResult VipsReader::read(const QString &fileName, const QString &targetIccProfileFileName)
{
    ReadResult result;
    try
    {
        vips::VImage in = readFile(fileName, false);

        // TODO: Open questions:
        // - Can we do this lazily?
        // - Should we do all this in preloading too?

        // If we have no embedded ICC profile, interpret as sRGB
        const bool hasEmbeddedProfile = in.get_typeof("icc-profile-data") != 0;
        if (!hasEmbeddedProfile && in.interpretation() != VIPS_INTERPRETATION_sRGB) {
            in = in.colourspace(VIPS_INTERPRETATION_sRGB);
        }

        // Transform the image color space from the embedded profile to the target profile
        const auto iccTransformOptions = vips::VImage::option()->set("embedded", true);
        if (!targetIccProfileFileName.isEmpty()) {
            in = in.icc_transform(targetIccProfileFileName.toUtf8().constData(), iccTransformOptions);
        } else {
            // In the absence of a monitor profile, treat as sRGB
            in = in.icc_transform("sRGB", iccTransformOptions);
        }

        // Strip away non-standard color channels (non RGB or RGBA)
        if (in.bands() > 4 && in.has_alpha())
        {
            in = in.extract_band(0, vips::VImage::option()->set("n", 4));
        } else if (in.bands() > 3 && !in.has_alpha()) {
            in = in.extract_band(0, vips::VImage::option()->set("n", 3));
        }

        // Add const alpha channel if it is not present
        if (!in.has_alpha()) {
            in = in.bandjoin_const({255.0});
        }

        // RGBA -> ARGB
        vips::VImage a = in.extract_band(3);
        vips::VImage r = in.extract_band(0);
        vips::VImage g = in.extract_band(1);
        vips::VImage b = in.extract_band(2);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        in = in.bandjoin({b,g,r,a});
#elif Q_BYTE_ORDER == Q_BIG_ENDIAN
        in = in.bandjoin({a,r,g,b});
#endif

        // Already premultiplied for some reason

        // Write result to buffer
        size_t buffer_size;
        void *buffer = in.write_to_memory(&buffer_size);

        auto cleanup = [](void *info) {
            g_free(info);
        };

        result.image = QImage(static_cast<const uchar *>(buffer), in.width(), in.height(), in.width() * 4, QImage::Format_ARGB32_Premultiplied, cleanup, buffer);
        if (result.image.isNull()) {
            throw vips::VError("Produced null QImage during conversion");
        }
    }
    catch (const vips::VError &e)
    {
        // TODO: Is this memory valid?
        result.error = e.what();
    }
    return result;
}
