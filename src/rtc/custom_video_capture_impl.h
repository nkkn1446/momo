/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef _CUSTOM_VIDEO_CAPTURE_IMPL
#define _CUSTOM_VIDEO_CAPTURE_IMPL

/*
 * video_capture_impl.h
 */

#include <stddef.h>
#include <stdint.h>

#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "api/video/video_rotation.h"
#include "api/video/video_sink_interface.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_config.h"
#include "modules/video_capture/video_capture_defines.h"
#include "rtc_base/critical_section.h"

namespace webrtc {

// Class definitions
class CustomVideoCaptureImpl : public VideoCaptureModule {
 public:
  /*
   *   Create a video capture module object
   *
   *   id              - unique identifier of this video capture module object
   *   deviceUniqueIdUTF8 -  name of the device. Available names can be found by
   * using GetDeviceName
   */
  static rtc::scoped_refptr<VideoCaptureModule> Create(
      const char* deviceUniqueIdUTF8);

  static DeviceInfo* CreateDeviceInfo();

  // Helpers for converting between (integral) degrees and
  // VideoRotation values.  Return 0 on success.
  static int32_t RotationFromDegrees(int degrees, VideoRotation* rotation);
  static int32_t RotationInDegrees(VideoRotation rotation, int* degrees);

  // Call backs
  void RegisterCaptureDataCallback(
      rtc::VideoSinkInterface<VideoFrame>* dataCallback) override;
  void DeRegisterCaptureDataCallback() override;

  int32_t SetCaptureRotation(VideoRotation rotation) override;
  bool SetApplyRotation(bool enable) override;
  bool GetApplyRotation() override;

  const char* CurrentDeviceName() const override;

  // |capture_time| must be specified in NTP time format in milliseconds.
  VideoFrame IncomingFrame(uint8_t* videoFrame,
                        size_t videoFrameLength,
                        const VideoCaptureCapability& frameInfo,
                        int64_t captureTime = 0);

  // Platform dependent
  int32_t StartCapture(const VideoCaptureCapability& capability) override;
  int32_t StopCapture() override;
  bool CaptureStarted() override;
  int32_t CaptureSettings(VideoCaptureCapability& /*settings*/) override;

 protected:
  CustomVideoCaptureImpl();
  ~CustomVideoCaptureImpl() override;

  char* _deviceUniqueId;  // current Device unique name;
  rtc::CriticalSection _apiCs;
  VideoCaptureCapability _requestedCapability;  // Should be set by platform
                                                // dependent code in
                                                // StartCapture.
  int32_t DeliverCapturedFrame(VideoFrame& captureFrame);
 private:
  void UpdateFrameCount();
  uint32_t CalculateFrameRate(int64_t now_ns);

  // last time the module process function was called.
  int64_t _lastProcessTimeNanos;
  // last time the frame rate callback function was called.
  int64_t _lastFrameRateCallbackTimeNanos;

  rtc::VideoSinkInterface<VideoFrame>* _dataCallBack;

  int64_t _lastProcessFrameTimeNanos;
  // timestamp for local captured frames
  int64_t _incomingFrameTimesNanos[videocapturemodule::kFrameRateCountHistorySize];
  VideoRotation _rotateFrame;  // Set if the frame should be rotated by the
                               // capture module.

  // Indicate whether rotation should be applied before delivered externally.
  bool apply_rotation_;
};
}  // namespace webrtc
#endif  // _CUSTOM_VIDEO_CAPTURE_IMPL
