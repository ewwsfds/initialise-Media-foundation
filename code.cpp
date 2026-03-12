#include <windows.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <iostream>

int main() {
    HRESULT hr = S_OK;

    // Initialize COM
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "COM init failed\n";
        return -1;
    }

    // Initialize Media Foundation platform
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cerr << "MFStartup failed\n";
        CoUninitialize();
        return -1;
    }

    std::cout << "Media Foundation initialized successfully!\n";

    // Your MF code would go here (decoder, reader, etc.)

    // Shut down Media Foundation
    MFShutdown();
    CoUninitialize();
    return 0;
}
