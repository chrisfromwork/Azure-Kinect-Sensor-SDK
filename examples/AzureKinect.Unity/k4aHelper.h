#pragma once
#include <k4a/k4a.h>
#include <math.h>

#define INVALID INT32_MIN

typedef struct _pinhole_t
{
    float px;
    float py;
    float fx;
    float fy;

    int width;
    int height;
} pinhole_t;

typedef struct _coordinate_t
{
    int x;
    int y;
} coordinate_t;

static pinhole_t create_pinhole(float field_of_view, int width, int height)
{
    pinhole_t pinhole;

    float f = 0.5f / tanf(0.5f * field_of_view * 3.14159265f / 180.f);

    pinhole.px = (float)width / 2.f;
    pinhole.py = (float)height / 2.f;
    pinhole.fx = f * (float)width;
    pinhole.fy = f * (float)height;
    pinhole.width = width;
    pinhole.height = height;

    return pinhole;
}

static void create_undistortion_lut(const k4a_calibration_t *calibration,
                                    const k4a_calibration_type_t camera,
                                    pinhole_t *pinhole,
                                    k4a_image_t lut)
{
    coordinate_t *lut_data = (coordinate_t *)(void *)k4a_image_get_buffer(lut);

    k4a_float3_t ray;
    ray.xyz.z = 1.f;

    int src_width = calibration->depth_camera_calibration.resolution_width;
    int src_height = calibration->depth_camera_calibration.resolution_height;
    if (camera == K4A_CALIBRATION_TYPE_COLOR)
    {
        src_width = calibration->color_camera_calibration.resolution_width;
        src_height = calibration->color_camera_calibration.resolution_height;
    }

    for (int y = 0, idx = 0; y < pinhole->height; y++)
    {
        ray.xyz.y = ((float)y - pinhole->py) / pinhole->fy;

        for (int x = 0; x < pinhole->width; x++, idx++)
        {
            ray.xyz.x = ((float)x - pinhole->px) / pinhole->fx;

            k4a_float2_t distorted;
            int valid;
            k4a_calibration_3d_to_2d(calibration, &ray, camera, camera, &distorted, &valid);

            coordinate_t src;
            // Remapping via nearest neighbor interpolation
            src.x = (int)floorf(distorted.xy.x + 0.5f);
            src.y = (int)floorf(distorted.xy.y + 0.5f);

            if (valid && src.x >= 0 && src.x < src_width && src.y >= 0 && src.y < src_height)
            {
                lut_data[idx] = src;
            }
            else
            {
                lut_data[idx].x = INVALID;
                lut_data[idx].y = INVALID;
            }
        }
    }
}

static void remap(const k4a_image_t src, const k4a_image_t lut, k4a_image_t dst)
{
    int src_width = k4a_image_get_width_pixels(src);
    int dst_width = k4a_image_get_width_pixels(dst);
    int dst_height = k4a_image_get_height_pixels(dst);

    uint16_t *src_data = (uint16_t *)(void *)k4a_image_get_buffer(src);
    uint16_t *dst_data = (uint16_t *)(void *)k4a_image_get_buffer(dst);
    coordinate_t *lut_data = (coordinate_t *)(void *)k4a_image_get_buffer(lut);

    memset(dst_data, 0, (size_t)dst_width * (size_t)dst_height * sizeof(uint16_t));

    for (int i = 0; i < dst_width * dst_height; i++)
    {
        if (lut_data[i].x != INVALID && lut_data[i].y != INVALID)
        {
            dst_data[i] = src_data[lut_data[i].y * src_width + lut_data[i].x];
        }
    }
}