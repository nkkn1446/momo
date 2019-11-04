#ifndef _CUSTOMVIDEOCAPTURER_H
#define _CUSTOMVIDEOCAPTURER_H

#include "media/base/videocapturer.h"

class CustomVideoCapturer :
        public cricket::VideoCapturer
{
public:
    explicit CustomVideoCapturer();
    virtual ~CustomVideoCapturer();

    // cricket::VideoCapturer implementation.
    virtual cricket::CaptureState Start(const cricket::VideoFormat& capture_format) override;
    virtual void Stop() override;
    virtual bool IsRunning() override;
    virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override;
    virtual bool GetBestCaptureFormat(const cricket::VideoFormat& desired, cricket::VideoFormat* best_format) override;
    virtual bool IsScreencast() const override { return false; }

    void Render();

private:
    bool now_rendering;
};

#endif // _CUSTOMVIDEOCAPTURER_H
