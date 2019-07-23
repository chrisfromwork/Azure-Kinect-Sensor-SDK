#include "pch.h"
#include "AzureKinectPlugin.h"
#include "AzureKinectWrapper.h"
#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsD3D11.h"

#define UNITYDLL EXTERN_C __declspec(dllexport)

static IUnityInterfaces *s_unityInterfaces = nullptr;
static IUnityGraphics *s_graphics = nullptr;
static ID3D11Device *s_device = nullptr;
std::unique_ptr<AzureKinectWrapper> azureKinectWrapper;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    switch (eventType)
    {
    case kUnityGfxDeviceEventInitialize:
    {
        IUnityGraphicsD3D11 *d3d11 = s_unityInterfaces->Get<IUnityGraphicsD3D11>();
        if (d3d11 != nullptr)
        {
            OutputDebugString(L"DirectX device obtained.");
            s_device = d3d11->GetDevice();
        }
        else
        {
            OutputDebugString(L"Failed to obtain DirectX device.");
        }
    }
    break;
    case kUnityGfxDeviceEventShutdown:
        OutputDebugString(L"Destroying DirectX device.");
        s_device = nullptr;
        break;
    }
}

void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces *unityInterfaces)
{
    OutputDebugString(L"UnityPluginLoad Event called for AzureKinect.Unity. UnityInterfaces was null: " +
                      (unityInterfaces == nullptr));
    s_unityInterfaces = unityInterfaces;
    s_graphics = s_unityInterfaces->Get<IUnityGraphics>();
    s_graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UNITY_INTERFACE_API UnityPluginUnload()
{
    OutputDebugString(L"UnityPluginUnload Event called for AzureKinect.Unity.");
    s_graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
    s_graphics = nullptr;
    s_unityInterfaces = nullptr;
    s_device = nullptr;
}

UNITYDLL uint32_t GetDeviceCount()
{
    return AzureKinectWrapper::GetDeviceCount();
}

UNITYDLL bool Initialize()
{
    if (s_device == nullptr)
    {
        OutputDebugString(L"DirectX device not initialized, unable to initialize azure kinect plugin.");
        return false;
    }

    if (azureKinectWrapper == nullptr)
    {
        azureKinectWrapper = std::make_unique<AzureKinectWrapper>(s_device);
    }

    return true;
}

UNITYDLL bool TryStartStreams(unsigned int index)
{
    if (azureKinectWrapper != nullptr)
    {
        return azureKinectWrapper->TryStartStreams(index);
    }

    return false;
}

UNITYDLL bool TryUpdate()
{
    if (azureKinectWrapper != nullptr)
    {
        return azureKinectWrapper->TryUpdate();
    }

    return false;
}

UNITYDLL void StopStreaming(unsigned int index)
{
    if (azureKinectWrapper != nullptr)
    {
        azureKinectWrapper->StopStreaming(index);
    }
}

UNITYDLL bool TryGetDeviceSerialNumber(uint32_t index, char *serialNum, uint32_t serialNumSize)
{
    if (azureKinectWrapper != nullptr)
    {
        return azureKinectWrapper->TryGetDeviceSerialNumber(index, serialNum, serialNumSize);
    }

    return false;
}

UNITYDLL bool TryGetShaderResourceViews(unsigned int index,
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
    if (azureKinectWrapper != nullptr)
    {
        return azureKinectWrapper->TryGetShaderResourceViews(index,
                                                             rgbSrv,
                                                             rgbWidth,
                                                             rgbHeight,
                                                             rgbBpp,
                                                             irSrv,
                                                             irWidth,
                                                             irHeight,
                                                             irBpp,
                                                             depthSrv,
                                                             depthWidth,
                                                             depthHeight,
                                                             depthBpp);
    }

    return false;
}
