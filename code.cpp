#include <windows.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <iostream>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

int main() {
    HRESULT hr = S_OK;

    // ------------------------
    // Initialize COM & MF
    // ------------------------
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    MFStartup(MF_VERSION);

    // ------------------------
    // Create D3D11 device
    // ------------------------
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &d3dDevice,
        &featureLevel,
        &d3dContext
    );

    // ------------------------
    // Compile NV12->RGB Pixel Shader
    // ------------------------
    const char* shaderSrc = R"(
        Texture2D YTex    : register(t0);
        Texture2D UVTex   : register(t1);
        SamplerState samp : register(s0);

        struct PSInput { float2 uv : TEXCOORD; };

        float4 PSMain(PSInput input) : SV_Target
        {
            float Y = YTex.Sample(samp, input.uv).r;
            float2 CbCr = UVTex.Sample(samp, input.uv * 0.5).rg - 0.5;

            float R = Y + 1.402 * CbCr.y;
            float G = Y - 0.344136 * CbCr.x - 0.714136 * CbCr.y;
            float B = Y + 1.772 * CbCr.x;

            return float4(R, G, B, 1.0);
        }
    )";

    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    hr = D3DCompile(
        shaderSrc,
        strlen(shaderSrc),
        nullptr,
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        0,
        0,
        &psBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            std::cout << "Shader compile error: " << (char*)errorBlob->GetBufferPointer() << "\n";
            errorBlob->Release();
        }
        return -1;
    }

    ID3D11PixelShader* nv12ToRgbPS = nullptr;
    hr = d3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &nv12ToRgbPS);
    psBlob->Release();
    if (FAILED(hr)) { std::cout << "Failed to create pixel shader\n"; return -1; }

    std::cout << "NV12->RGB pixel shader compiled successfully\n";

    // ------------------------
    // Media Foundation: read NV12 frame
    // ------------------------
    IMFAttributes* attrs = nullptr;
    MFCreateAttributes(&attrs, 1);

    IMFDXGIDeviceManager* dxgiManager = nullptr;
    UINT resetToken = 0;
    MFCreateDXGIDeviceManager(&resetToken, &dxgiManager);
    dxgiManager->ResetDevice(d3dDevice, resetToken);

    attrs->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgiManager);
    attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    IMFSourceReader* reader = nullptr;
    MFCreateSourceReaderFromURL(L"video.mp4", nullptr, &reader);
    attrs->Release();

    IMFMediaType* mediaType = nullptr;
    MFCreateMediaType(&mediaType);
    mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType);
    mediaType->Release();

    std::cout << "Decoder configured for NV12 output\n";

    // ------------------------
    // Read first NV12 frame and prepare shader resources
    // ------------------------
    DWORD streamIndex, flags;
    LONGLONG timestamp;

    IMFSample* sample = nullptr;
    hr = reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &timestamp, &sample);

    if (SUCCEEDED(hr) && sample) {
        IMFMediaBuffer* buffer = nullptr;
        sample->ConvertToContiguousBuffer(&buffer);

        ID3D11Texture2D* nv12Tex = nullptr;
        buffer->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&nv12Tex);

        if (nv12Tex) {
            // Create shader resource views for Y and UV planes
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            // Y plane
            srvDesc.Format = DXGI_FORMAT_R8_UNORM;
            ID3D11ShaderResourceView* ySRV = nullptr;
            d3dDevice->CreateShaderResourceView(nv12Tex, &srvDesc, &ySRV);

            // UV plane
            srvDesc.Format = DXGI_FORMAT_R8G8_UNORM;
            ID3D11ShaderResourceView* uvSRV = nullptr;
            d3dDevice->CreateShaderResourceView(nv12Tex, &srvDesc, &uvSRV);

            std::cout << "Created SRVs for NV12 shader\n";

            // Bind to pixel shader
            d3dContext->PSSetShader(nv12ToRgbPS, nullptr, 0);
            d3dContext->PSSetShaderResources(0, 1, &ySRV);
            d3dContext->PSSetShaderResources(1, 1, &uvSRV);

            // Cleanup
            ySRV->Release();
            uvSRV->Release();
            nv12Tex->Release();
        }

        buffer->Release();
        sample->Release();
    }

    // ------------------------
    // Cleanup
    // ------------------------
    if (reader) reader->Release();
    if (nv12ToRgbPS) nv12ToRgbPS->Release();
    if (d3dContext) d3dContext->Release();
    if (d3dDevice) d3dDevice->Release();

    MFShutdown();
    CoUninitialize();

    std::cout << "Shutdown complete\n";
    return 0;
}
