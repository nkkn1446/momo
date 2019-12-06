/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef CUSTOM_VIDEO_TRACK_SOURCE_H_
#define CUSTOM_VIDEO_TRACK_SOURCE_H_

#include "rtc/scalable_track_source.h"
#include "api/video/video_sink_interface.h"
#include "modules/video_capture/video_capture_impl.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/platform_thread.h"

#include "screen_capturer_frame.h"

class ScreenCapturerTrackSource : public ScalableVideoTrackSource,
                            public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  ScreenCapturerTrackSource();
  virtual ~ScreenCapturerTrackSource();

  void OnFrame(const webrtc::VideoFrame& frame) override;

  class DesktopCaptureImpl : public webrtc::videocapturemodule::VideoCaptureImpl {
  public:
	  DesktopCaptureImpl();
	  ~DesktopCaptureImpl() override;

	  void StartCapture();
	  static void CaptureThread(void* obj);
	  void Callback(std::unique_ptr<webrtc::DesktopFrame>&& capturedFrame);
	  bool CaptureProcess();

	  std::unique_ptr<rtc::PlatformThread> captureThread_;
	  webrtc::ScreenCapturerFrame frame_;

  };

  rtc::scoped_refptr<DesktopCaptureImpl> dcm_;
};

#endif  // VIDEO_CAPTURER_H_