#include "pch.h"
#include "AzureKinectPlugin.h"
#include "AzureKinectWrapper.h"
#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsD3D11.h"

#define UNITYDLL EXTERN_C __declspec(dllexport)

static IUnityInterfaces *unityInterfaces = nullptr;
static IUnityGraphics *graphics = nullptr;
static ID3D11Device *device = nullptr;
std::unique_ptr<AzureKinectWrapper> azureKinectWrapper;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    switch (eventType)
    {
    case kUnityGfxDeviceEventInitialize:
    {
        IUnityGraphicsD3D11 *d3d11 = unityInterfaces->Get<IUnityGraphicsD3D11>();
        if (d3d11 != nullptr)
        {
            OutputDebugString(L"DirectX device obtained.");
            device = d3d11->GetDevice();
        }
        else
        {
            OutputDebugString(L"Failed to obtain DirectX device.");
        }
    }
    break;
    case kUnityGfxDeviceEventShutdown:
        OutputDebugString(L"Destroying DirectX device.");
        device = nullptr;
        break;
    }
}

void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces *unityInterfaces)
{
    OutputDebugString(L"UnityPluginLoad Event called for AzureKinect.Unity. UnityInterfaces was null: " + (unityInterfaces == nullptr));
    this->unityInterfaces = unityInterfaces;
    graphics = this->unityInterfaces->Get<IUnityGraphics>();
    graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UNITY_INTERFACE_API UnityPluginUnload()
{
    OutputDebugString(L"UnityPluginUnload Event called for AzureKinect.Unity.");
    graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
    graphics = nullptr;
    unityInterfaces = nullptr;
    device = nullptr;
}

UNITYDLL uint32_t GetDeviceCount()
{
    return AzureKinectWrapper::GetDeviceCount();
}

UNITYDLL bool TryGetDeviceSerialNumber(uint32_t index, char *serialNum, uint32_t serialNumSize)
{
    return AzureKinectWrapper::TryGetDeviceSerialNumber(index, serialNum, serialNumSize);
}

UNITYDLL bool Initialize()
{
    if (device == nullptr)
    {
        OutputDebugString(L"DirectX device not initialized, unable to initialize azure kinect plugin.");
        return false;
    }

    if (azureKinectWrapper == nullptr)
    {
        azureKinectWrapper = std::make_unique<AzureKinectWrapper>(device);
    }

    return true;
}

UNITYDLL bool TryStartStreams(unsigned int index)
{
    if (azureKinectWrapper != nullptr)
    {
        azureKinectWrapper->TryStartStreams(index);
        return true;
    }

    return false;
}

UNITYDLL bool TryUpdate()
{
    if (azureKinectWrapper != nullptr)
    {
        azureKinectWrapper->TryUpdate();
        return true;
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

UNITYDLL bool TryGetShaderResources(unsigned int index,
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
        return azureKinectWrapper->TryGetShaderResourceViews(
			index,
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
