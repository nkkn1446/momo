#include "customcapturer.h"

#include <memory>

#include "api/video/i420_buffer.h"
#include "common_types.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "base/logging.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv.h"

#include <algorithm> // fmod

CustomVideoCapturer::CustomVideoCapturer()
  : now_rendering(false)
{
}

CustomVideoCapturer::~CustomVideoCapturer()
{
}

void CustomVideoCapturer::Render(){
    if (!now_rendering) return;
    int buf_width = 640;
    int buf_height = 480;

    int64_t timems = rtc::TimeMillis();

    rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(buf_width, buf_height);

    webrtc::I420Buffer::SetBlack(buffer.get());

    webrtc::VideoFrame frame(buffer, 0, timems, webrtc::kVideoRotation_0);
    frame.set_ntp_time_ms(0);

    OnFrame(frame, buf_width, buf_height);
}

cricket::CaptureState CustomVideoCapturer::Start(const cricket::VideoFormat& capture_format)
{
    if (capture_state() == cricket::CS_RUNNING) {
        return capture_state();
    }

    now_rendering = true;
    SetCaptureFormat(&capture_format);

    return cricket::CS_RUNNING;
}

void CustomVideoCapturer::Stop()
{
    now_rendering = false;
    if (capture_state() == cricket::CS_STOPPED) {
        return;
    }

    SetCaptureFormat(NULL);
    SetCaptureState(cricket::CS_STOPPED);
}


bool CustomVideoCapturer::IsRunning()
{
    return capture_state() == cricket::CS_RUNNING;
}

bool CustomVideoCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs)
{
    if (!fourccs) return false;
    fourccs->push_back(cricket::FOURCC_I420);
    return true;
}

bool CustomVideoCapturer::GetBestCaptureFormat(const cricket::VideoFormat& desired, cricket::VideoFormat* best_format)
{
    if (!best_format) return false;

    // Use the desired format as the best format.
    best_format->width = desired.width;
    best_format->height = desired.height;
    best_format->fourcc = cricket::FOURCC_I420;
    best_format->interval = desired.interval;
    return true;
}
