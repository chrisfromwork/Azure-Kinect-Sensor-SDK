#pragma once

class AzureKinectWrapper
{
public:
    static unsigned int GetDeviceCount();
    static bool TryGetDeviceSerialNumber(unsigned int index, char *serialNum, unsigned int serialNumSize);

    AzureKinectWrapper(ID3D11Device *device);
    ~AzureKinectWrapper();
    bool TryGetShaderResourceViews(unsigned int index,
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
                                   unsigned int &depthBpp);
    bool TryStartStreams(unsigned int index);
    bool TryUpdate();
    void StopStreaming(unsigned int index);

private:
    struct FrameDimensions
    {
        unsigned int width;
        unsigned int height;
        unsigned int bpp;
    };

    struct DeviceResources
    {
        ID3D11Texture2D *rgbTexture;
        ID3D11ShaderResourceView *rgbSrv;
        FrameDimensions rgbFrameDimensions;
        ID3D11Texture2D *irTexture;
        ID3D11ShaderResourceView *irSrv;
        FrameDimensions irFrameDimensions;
        ID3D11Texture2D *depthTexture;
        ID3D11ShaderResourceView *depthSrv;
        FrameDimensions depthFrameDimensions;
    };

    void UpdateResources(k4a_image_t image,
                         ID3D11ShaderResourceView *&srv,
                         ID3D11Texture2D *&tex,
                         FrameDimensions &dim,
                         DXGI_FORMAT format);

    ID3D11Device *device;
    static std::shared_ptr<AzureKinectWrapper> instance;
    std::map<int, k4a_device_t> deviceMap;
    std::map<int, DeviceResources> resourcesMap;
    CRITICAL_SECTION resourcesCritSec;
};
