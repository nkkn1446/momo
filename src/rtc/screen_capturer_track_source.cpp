/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "screen_capturer_track_source.h"
#include "rtc_base/logging.h"

#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"

#include "native_buffer.h"

#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "third_party/libyuv/include/libyuv/scale.h"

ScreenCapturerTrackSource::DesktopCaptureImpl::DesktopCaptureImpl() : 
	frame_() {}
ScreenCapturerTrackSource::DesktopCaptureImpl::~DesktopCaptureImpl() {}

void ScreenCapturerTrackSource::DesktopCaptureImpl::StartCapture() {
	if (!captureThread_) {
		captureThread_.reset(
				new rtc::PlatformThread(DesktopCaptureImpl::CaptureThread, this,
					"CaptureThread", rtc::kHighPriority));
		captureThread_->Start();
	}
}

void ScreenCapturerTrackSource::DesktopCaptureImpl::CaptureThread(void* obj) {
	auto* capture = static_cast<DesktopCaptureImpl*>(obj);
	while (capture->CaptureProcess()) {
	}
}

void ScreenCapturerTrackSource::DesktopCaptureImpl::Callback(std::unique_ptr<webrtc::DesktopFrame>&& frame) {
	unsigned char* data = frame->data();
	webrtc::DesktopSize size = frame->size();
	webrtc::VideoCaptureCapability frameInfo;

	frameInfo.width = size.width();
	frameInfo.height = size.height();
	frameInfo.videoType = webrtc::VideoType::kARGB;

	IncomingFrame(data, size.width() * size.height() * webrtc::DesktopFrame::kBytesPerPixel, frameInfo);
}
	
bool ScreenCapturerTrackSource::DesktopCaptureImpl::CaptureProcess() {
	frame_.Capturer(std::bind(
				&DesktopCaptureImpl::Callback,
				this,
				std::placeholders::_1));
	return true;

}

ScreenCapturerTrackSource::ScreenCapturerTrackSource() :
      dcm_(new rtc::RefCountedObject<DesktopCaptureImpl>()) {
  dcm_->RegisterCaptureDataCallback(this);
  dcm_->StartCapture();
}

ScreenCapturerTrackSource::~ScreenCapturerTrackSource() {
  dcm_->StopCapture();
  dcm_->DeRegisterCaptureDataCallback();
  dcm_ = nullptr;
}

void ScreenCapturerTrackSource::OnFrame(const webrtc::VideoFrame& frame) {
  OnCapturedFrame(frame);
}
