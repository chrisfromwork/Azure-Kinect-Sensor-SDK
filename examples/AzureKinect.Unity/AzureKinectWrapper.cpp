#include "pch.h"
#include "AzureKinectWrapper.h"

std::shared_ptr<AzureKinectWrapper> AzureKinectWrapper::instance = nullptr;

AzureKinectWrapper::AzureKinectWrapper(ID3D11Device *device)
{
    InitializeCriticalSection(&resourcesCritSec);
    this->device = device;
}

AzureKinectWrapper::~AzureKinectWrapper()
{
    DeleteCriticalSection(&resourcesCritSec);
}

unsigned int AzureKinectWrapper::GetDeviceCount()
{
    return k4a_device_get_installed_count();
}

bool AzureKinectWrapper::TryGetDeviceSerialNumber(unsigned int index, char *serialNum, unsigned int serialNumSize)
{
    uint32_t device_count = k4a_device_get_installed_count();
    if (index > device_count - 1)
    {
        return false;
    }

    k4a_device_t device = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_device_open(index, &device))
    {
        return false;
    }

    size_t serialNumLength = 0;
    if (K4A_BUFFER_RESULT_TOO_SMALL != k4a_device_get_serialnum(device, serialNum, &serialNumLength) ||
        serialNumLength > serialNumSize)
    {
        return false;
    }

    k4a_device_close(device);
    device = NULL;
    return true;
}

bool AzureKinectWrapper::TryGetShaderResourceViews(unsigned int index,
                                                   ID3D11ShaderResourceView *&rgbSrv,
                                                   unsigned int &rgbWidth,
                                                   unsigned int &rgbHeight,
                                                   unsigned int &rgbBpp,
                                                   ID3D11ShaderResourceView *&irSrv,
                                                   unsigned int &irWidth,
                                                   unsigned int &irHeight,
                                                   unsigned int &irBpp,
                                                   ID3D11ShaderResourceView *&depthSrv,
                                                   unsigned int &depthWidth,
                                                   unsigned int &depthHeight,
                                                   unsigned int &depthBpp)
{
    EnterCriticalSection(&resourcesCritSec);
    if (resourcesMap.count(index) == 0)
    {
        LeaveCriticalSection(&resourcesCritSec);
        OutputDebugString(L"Resources not created for device: " + index);
        return false;
    }

    rgbSrv = resourcesMap[index].rgbSrv;
    rgbWidth = resourcesMap[index].rgbFrameDimensions.width;
    rgbHeight = resourcesMap[index].rgbFrameDimensions.height;
    rgbBpp = resourcesMap[index].rgbFrameDimensions.bpp;

    irSrv = resourcesMap[index].irSrv;
    irWidth = resourcesMap[index].irFrameDimensions.width;
    irHeight = resourcesMap[index].irFrameDimensions.height;
    irBpp = resourcesMap[index].irFrameDimensions.bpp;

    depthSrv = resourcesMap[index].depthSrv;
    depthWidth = resourcesMap[index].depthFrameDimensions.width;
    depthHeight = resourcesMap[index].depthFrameDimensions.height;
    depthBpp = resourcesMap[index].depthFrameDimensions.bpp;

    LeaveCriticalSection(&resourcesCritSec);
    return true;
}

bool AzureKinectWrapper::TryStartStreams(unsigned int index)
{
    k4a_device_t device = NULL;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    if (deviceMap.count(index) > 0)
    {
        return true;
    }

    if (static_cast<unsigned int>(index) > GetDeviceCount() - 1)
    {
        OutputDebugString(L"Provided index did not exist: " + index);
        goto FailedExit;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &device))
    {
        OutputDebugString(L"Failed to open device: " + index);
        goto FailedExit;
    }

    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    config.depth_mode = K4A_DEPTH_MODE_WFOV_UNBINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    // Make sure the IMU has been stopped before starting capture
    k4a_device_stop_imu(device);

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
    {
        OutputDebugString(L"Failed to start cameras: " + index);
        goto FailedExit;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_imu(device))
    {
        OutputDebugString(L"Failed to start imu: " + index);
    }

    deviceMap[index] = device;
    return true;

FailedExit:
    if (device != NULL)
    {
        k4a_device_close(device);
    }
    return false;
}

bool AzureKinectWrapper::TryUpdate()
{
    if (deviceMap.size() == 0)
    {
        OutputDebugString(L"No devices created, update failed");
        return false;
    }

    k4a_capture_t capture = NULL;
    bool observedFailure = false;
    for (auto pair : deviceMap)
    {
        auto device = pair.second;
        k4a_image_t image;

        switch (k4a_device_get_capture(device, &capture, 0))
        {
        case K4A_WAIT_RESULT_SUCCEEDED:
            break;
        case K4A_WAIT_RESULT_TIMEOUT:
            OutputDebugString(L"Timed out waiting for capture: " + pair.first);
            observedFailure = true;
            continue;
        case K4A_WAIT_RESULT_FAILED:
            OutputDebugString(L"Failed to capture: " + pair.first);
            observedFailure = true;
            continue;
        }

        EnterCriticalSection(&resourcesCritSec);
        if (resourcesMap.count(pair.first) == 0)
        {
            resourcesMap[pair.first] = DeviceResources{
                nullptr, nullptr, {}, nullptr, nullptr, {}, nullptr, nullptr, {}
            };
        }

        DeviceResources &resources = resourcesMap[pair.first];
        LeaveCriticalSection(&resourcesCritSec);

        image = k4a_capture_get_color_image(capture);
        if (image)
        {
            UpdateResources(image,
                            resources.rgbSrv,
                            resources.rgbTexture,
                            resources.rgbFrameDimensions,
                            DXGI_FORMAT_R8G8B8A8_UNORM);
            k4a_image_release(image);
        }
        else
        {
            OutputDebugString(L"Failed to capture rgb image: " + pair.first);
        }

        image = k4a_capture_get_ir_image(capture);
        if (image != NULL)
        {
            UpdateResources(image,
                            resources.irSrv,
                            resources.irTexture,
                            resources.irFrameDimensions,
                            DXGI_FORMAT_R16_UNORM);
            k4a_image_release(image);
        }
        else
        {
            OutputDebugString(L"Failed to capture ir image: " + pair.first);
        }

        image = k4a_capture_get_depth_image(capture);
        if (image != NULL)
        {
            UpdateResources(image,
                            resources.depthSrv,
                            resources.depthTexture,
                            resources.depthFrameDimensions,
                            DXGI_FORMAT_R16_UNORM);
            k4a_image_release(image);
        }
        else
        {
            OutputDebugString(L"Failed to capture depth image: " + pair.first);
        }

        k4a_capture_release(capture);
    }

    return !observedFailure;
}

void AzureKinectWrapper::StopStreaming(unsigned int index)
{
    if (deviceMap.count(index) > 0)
    {
        OutputDebugString(L"Closed device: " + index);
        k4a_device_close(deviceMap[index]);
        deviceMap.erase(index);
    }
    else
    {
        OutputDebugString(L"Asked to close unknown device: " + index);
    }
}

void AzureKinectWrapper::UpdateResources(k4a_image_t image,
                                         ID3D11ShaderResourceView *&srv,
                                         ID3D11Texture2D *&tex,
                                         FrameDimensions &dim,
                                         DXGI_FORMAT format)
{
    EnterCriticalSection(&resourcesCritSec);
    dim.height = k4a_image_get_height_pixels(image);
    dim.width = k4a_image_get_width_pixels(image);
    auto stride = k4a_image_get_stride_bytes(image);
    dim.bpp = stride / dim.width;
    auto buffer = k4a_image_get_buffer(image);

    if (tex == nullptr)
    {
        DirectXHelper::CreateTexture(device, buffer, dim.width, dim.height, dim.bpp, format);
    }

    if (srv == nullptr)
    {
        DirectXHelper::CreateShaderResourceView(device, tex, format);
    }
    else
    {
        DirectXHelper::UpdateShaderResourceView(device, srv, buffer, stride);
    }

    LeaveCriticalSection(&resourcesCritSec);
}