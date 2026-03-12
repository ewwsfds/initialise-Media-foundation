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

    // 3️⃣ Open file
    IMFSourceReader* reader = nullptr;

    hr = MFCreateSourceReaderFromURL(L"video.mp4", nullptr, &reader);
    if (FAILED(hr)) {
        std::cout << "Failed to open video.mp4\n";
    }

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
            std::cout << "Frame timestamp: " << timestamp << "\n";
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
