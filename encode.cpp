#include <windows.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <mftransform.h>

#include <d3d11.h>
#include <dxgi1_2.h>

#include <iostream>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "d3d11.lib")

int main()
{
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return -1;

    // =========================
    // D3D11 DEVICE
    // =========================
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;

    D3D_FEATURE_LEVEL level;

    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &d3dDevice,
        &level,
        &d3dContext
    );
    if (FAILED(hr)) return -1;

    // =========================
    // DXGI MANAGER
    // =========================
    IMFDXGIDeviceManager* dxgiManager = nullptr;
    UINT resetToken = 0;

    MFCreateDXGIDeviceManager(&resetToken, &dxgiManager);
    dxgiManager->ResetDevice(d3dDevice, resetToken);

    // =========================
    // SOURCE READER ATTRIBUTES
    // =========================
    IMFAttributes* srcAttr = nullptr;
    MFCreateAttributes(&srcAttr, 1);
    srcAttr->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgiManager);

    IMFSourceReader* reader = nullptr;

    hr = MFCreateSourceReaderFromURL(
        L"test.mp4",
        srcAttr,
        &reader
    );
    if (FAILED(hr)) return -1;

    // =========================
    // FORCE NV12 OUTPUT
    // =========================
    IMFMediaType* type = nullptr;
    MFCreateMediaType(&type);

    type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);

    hr = reader->SetCurrentMediaType(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        nullptr,
        type
    );
    if (FAILED(hr)) return -1;

    type->Release();

    // =========================
    // GET STREAM INFO
    // =========================
    IMFMediaType* curType = nullptr;
    reader->GetCurrentMediaType(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        &curType
    );

    UINT32 width = 0, height = 0;
    MFGetAttributeSize(curType, MF_MT_FRAME_SIZE, &width, &height);

    UINT32 fpsNum = 30, fpsDen = 1;
    MFGetAttributeRatio(curType, MF_MT_FRAME_RATE, &fpsNum, &fpsDen);

    curType->Release();

    // =========================
    // SINK WRITER ATTRIBUTES (IMPORTANT FIX)
    // =========================
    IMFAttributes* sinkAttr = nullptr;
    MFCreateAttributes(&sinkAttr, 2);

    sinkAttr->SetUnknown(MF_SINK_WRITER_D3D_MANAGER, dxgiManager);
    sinkAttr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    IMFSinkWriter* writer = nullptr;

    hr = MFCreateSinkWriterFromURL(
        L"out.mp4",
        nullptr,
        sinkAttr,
        &writer
    );
    if (FAILED(hr)) return -1;

    // =========================
    // OUTPUT (H264)
    // =========================
    IMFMediaType* outType = nullptr;
    MFCreateMediaType(&outType);

    outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);

    MFSetAttributeSize(outType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(outType, MF_MT_FRAME_RATE, fpsNum, fpsDen);

    outType->SetUINT32(MF_MT_AVG_BITRATE, 5000000);
    outType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    outType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    DWORD streamIndex = 0;
    hr = writer->AddStream(outType, &streamIndex);
    if (FAILED(hr)) return -1;

    // =========================
    // INPUT (NV12)
    // =========================
    IMFMediaType* inType = nullptr;
    MFCreateMediaType(&inType);

    inType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);

    MFSetAttributeSize(inType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(inType, MF_MT_FRAME_RATE, fpsNum, fpsDen);

    inType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    inType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    hr = writer->SetInputMediaType(streamIndex, inType, nullptr);
    if (FAILED(hr)) return -1;

    // =========================
    // START WRITING
    // =========================
    hr = writer->BeginWriting();
    if (FAILED(hr)) return -1;

    std::cout << "Encoding started...\n";

    // =========================
    // LOOP
    // =========================
    IMFSample* sample = nullptr;
    DWORD flags = 0;
    LONGLONG ts = 0;

    while (true)
    {
        hr = reader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            nullptr,
            &flags,
            &ts,
            &sample
        );

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            break;

        if (FAILED(hr))
            break;

        if (!sample)
            continue;

        sample->SetSampleTime(ts);

        hr = writer->WriteSample(streamIndex, sample);

        if (FAILED(hr))
        {
            std::cout << "Write failed: 0x" << std::hex << hr << "\n";
        }

        sample->Release();
    }

    // =========================
    // FINALIZE
    // =========================
    writer->Finalize();

    // =========================
    // CLEANUP
    // =========================
    writer->Release();
    reader->Release();
    srcAttr->Release();
    sinkAttr->Release();
    dxgiManager->Release();
    d3dContext->Release();
    d3dDevice->Release();

    MFShutdown();

    std::cout << "DONE\n";
    return 0;
}
