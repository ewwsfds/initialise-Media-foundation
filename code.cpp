#include <windows.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <iostream>

int main() {
    HRESULT hr = S_OK;

    // 1️⃣ Initialize COM
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "COM init failed: " << std::hex << hr << "\n";
        return -1;
    }
    std::cout << "COM initialized successfully!\n";

    // 2️⃣ Initialize Media Foundation
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cerr << "MFStartup failed: " << std::hex << hr << "\n";
        CoUninitialize();
        return -1;
    }
    std::cout << "Media Foundation started successfully!\n";

    // 3️⃣ Create a source reader for the video file
    IMFSourceReader* pReader = nullptr;
    hr = MFCreateSourceReaderFromURL(L"video.mp4", nullptr, &pReader);
    if (FAILED(hr)) {
        std::cerr << "Failed to open video.mp4: " << std::hex << hr << "\n";
    }
    else {
        std::cout << "Video file opened successfully!\n";
    }

    // 4️⃣ Set the output media type to NV12 (common GPU-friendly format)
    if (pReader) {
        IMFMediaType* pType = nullptr;
        hr = MFCreateMediaType(&pType);
        if (SUCCEEDED(hr)) {
            hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
            if (FAILED(hr)) {
                std::cerr << "Failed to set media type: " << std::hex << hr << "\n";
            }
            else {
                hr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType);
                if (FAILED(hr)) {
                    std::cerr << "Failed to apply NV12 media type: " << std::hex << hr << "\n";
                }
                else {
                    std::cout << "Output media type set to NV12 successfully!\n";
                }
            }
            pType->Release();
        }
        else {
            std::cerr << "Failed to create media type: " << std::hex << hr << "\n";
        }
    }

    // 5️⃣ Clean up
    if (pReader) pReader->Release();

    MFShutdown();
    CoUninitialize();
    std::cout << "Media Foundation shut down successfully!\n";

    return 0;
}
