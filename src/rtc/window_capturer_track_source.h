/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef CUSTOM_WINDOW_TRACK_SOURCE_H_
#define CUSTOM_WINDOW_TRACK_SOURCE_H_

#include "scalable_track_source.h"
#include "api/video/video_sink_interface.h"
#include "modules/video_capture/video_capture_impl.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/platform_thread.h"

#include "window_capturer_frame.h"
#include "rtc_base/timestamp_aligner.h"

#include "connection_settings.h"

class WindowCapturerTrackSource : public ScalableVideoTrackSource,
                            public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 private:
  typedef rtc::AdaptedVideoTrackSource super;
 public:
  WindowCapturerTrackSource(const ConnectionSettings& cs);
  virtual ~WindowCapturerTrackSource();

  void OnFrame(const webrtc::VideoFrame& frame) override;

  class DesktopCaptureImpl : public webrtc::videocapturemodule::VideoCaptureImpl {
  public:
	  DesktopCaptureImpl(const ConnectionSettings& cs);
	  ~DesktopCaptureImpl() override;

	  void StartCapture();
	  static void CaptureThread(void* obj);
	  void Callback(std::unique_ptr<webrtc::DesktopFrame>&& capturedFrame);
	  bool CaptureProcess();

	  std::unique_ptr<rtc::PlatformThread> captureThread_;
	  webrtc::WindowCapturerFrame frame_;

  };

  rtc::scoped_refptr<DesktopCaptureImpl> dcm_;

  bool is_screencast() const override;
  absl::optional<bool> needs_denoising() const override;
  webrtc::MediaSourceInterface::SourceState state() const override;
  bool remote() const override;

 private:
  rtc::TimestampAligner timestamp_aligner_;

  cricket::VideoAdapter video_adapter_;
};

#endif  // WINDOW_CAPTURER_H_
