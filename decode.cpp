#include <windows.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <iostream>


#include <d3d11.h>
#include <dxgi1_2.h>       // for DXGI device manager



int main() {
    HRESULT hr = S_OK;

    // 1️⃣ Initialize COM
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cout << "COM init failed\n";
        return -1;
    }
    std::cout << "COM initialized\n";

    // 2️⃣ Start Media Foundation
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cout << "MFStartup failed\n";
        CoUninitialize();
        return -1;
    }
    std::cout << "Media Foundation started\n";


    // Create a D3D11 device 
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

    // Create a DXGI Device Manager for Media Foundation
    IMFDXGIDeviceManager* dxgiManager = nullptr;
    UINT resetToken = 0;

    hr = MFCreateDXGIDeviceManager(&resetToken, &dxgiManager);
    hr = dxgiManager->ResetDevice(d3dDevice, resetToken);




    // 3️⃣ Open file

    //but tell first Source Reader to use DXGI surfaces
    IMFAttributes* attrs = nullptr;
    MFCreateAttributes(&attrs, 1);

    attrs->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgiManager);
    attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);


    IMFSourceReader* reader = nullptr;

    hr = MFCreateSourceReaderFromURL(L"video.mp4", attrs, &reader);
    if (FAILED(hr)) {
        std::cout << "Failed to open video.mp4\n";
    }
    attrs->Release();
    std::cout << "Opened video file\n";

    // 4️⃣ Configure decoder output format (NV12)
    IMFMediaType* mediaType = nullptr;

    hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) {
        std::cout << "Failed to create media type\n";
    }

    mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);

    hr = reader->SetCurrentMediaType(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        nullptr,
        mediaType
    );

    if (FAILED(hr)) {
        std::cout << "Failed to set NV12 output\n";
    }

    std::cout << "Decoder configured for NV12 output\n";

    mediaType->Release();
    mediaType = nullptr;

    // 5️⃣ Try to read a decoded frame
    DWORD streamIndex;
    DWORD flags;
    LONGLONG timestamp;
    while (true)
    {
        IMFSample* sample = nullptr;

        hr = reader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            &streamIndex,
            &flags,
            &timestamp,
            &sample
        );

        if (FAILED(hr)) {
            std::cout << "ReadSample failed (decoder probably failed)\n";
            goto shutdown;
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            std::cout << "End of stream reached\n";
            break;
        }

        if (sample) {

            IMFMediaBuffer* buffer = nullptr;
            sample->GetBufferByIndex(0, &buffer);

            IMFDXGIBuffer* dxgiBuffer = nullptr;
            buffer->QueryInterface(IID_PPV_ARGS(&dxgiBuffer));

            ID3D11Texture2D* tex = nullptr;
            hr =  dxgiBuffer->GetResource(IID_PPV_ARGS(&tex));


            if (SUCCEEDED(hr))
            {
                std::cout << "GPU frame (zero-copy)\n";

                // tex is your GPU NV12 texture — ready for rendering
                tex->Release();
            }

            else
                std::cout << "CPU frame (system memory)\n";

            std::cout << "Frame timestamp: " << timestamp << "\n";
            buffer->Release();
            sample->Release();
        }
        else {
            std::cout << "No sample returned\n";
        }
    }
shutdown:

    if (reader) reader->Release();

    MFShutdown();
    CoUninitialize();

    std::cout << "Shutdown complete\n";

    return 0;
}
