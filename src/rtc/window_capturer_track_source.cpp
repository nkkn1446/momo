/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "window_capturer_track_source.h"
#include "rtc_base/logging.h"

#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"

#include "native_buffer.h"

#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "third_party/libyuv/include/libyuv/scale.h"

WindowCapturerTrackSource::DesktopCaptureImpl::DesktopCaptureImpl() : 
	frame_() {}
WindowCapturerTrackSource::DesktopCaptureImpl::~DesktopCaptureImpl() {}

void WindowCapturerTrackSource::DesktopCaptureImpl::StartCapture() {
	if (!captureThread_) {
		captureThread_.reset(
				new rtc::PlatformThread(DesktopCaptureImpl::CaptureThread, this,
					"CaptureThread", rtc::kHighPriority));
		captureThread_->Start();
	}
}

void WindowCapturerTrackSource::DesktopCaptureImpl::CaptureThread(void* obj) {
	auto* capture = static_cast<DesktopCaptureImpl*>(obj);
	while (capture->CaptureProcess()) {
	}
}

void WindowCapturerTrackSource::DesktopCaptureImpl::Callback(std::unique_ptr<webrtc::DesktopFrame>&& frame) {
	unsigned char* data = frame->data();
	webrtc::DesktopSize size = frame->size();
	webrtc::VideoCaptureCapability frameInfo;

	frameInfo.width = size.width();
	frameInfo.height = size.height();
	frameInfo.videoType = webrtc::VideoType::kARGB;

	IncomingFrame(data, size.width() * size.height() * webrtc::DesktopFrame::kBytesPerPixel, frameInfo);
}
	
bool WindowCapturerTrackSource::DesktopCaptureImpl::CaptureProcess() {
	frame_.Capturer(std::bind(
				&DesktopCaptureImpl::Callback,
				this,
				std::placeholders::_1));
	return true;

}

WindowCapturerTrackSource::WindowCapturerTrackSource() :
      AdaptedVideoTrackSource(4),
      dcm_(new rtc::RefCountedObject<DesktopCaptureImpl>()) {
  dcm_->RegisterCaptureDataCallback(this);
  dcm_->StartCapture();
}

WindowCapturerTrackSource::~WindowCapturerTrackSource() {
  dcm_->StopCapture();
  dcm_->DeRegisterCaptureDataCallback();
  dcm_ = nullptr;
}

//void WindowCapturerTrackSource::OnFrame(const webrtc::VideoFrame& frame) {
//  OnCapturedFrame(frame);
//}

bool WindowCapturerTrackSource::is_screencast() const {
  return false;
}

absl::optional<bool> WindowCapturerTrackSource::needs_denoising() const {
  return false;
}

webrtc::MediaSourceInterface::SourceState WindowCapturerTrackSource::state()
    const {
  return SourceState::kLive;
}

bool WindowCapturerTrackSource::remote() const {
  return false;
}

void WindowCapturerTrackSource::OnFrame(
    const webrtc::VideoFrame& frame) {
  const int64_t timestamp_us = frame.timestamp_us();
  const int64_t translated_timestamp_us =
      timestamp_aligner_.TranslateTimestamp(timestamp_us, rtc::TimeMicros());

  int adapted_width;
  int adapted_height;
  int crop_width;
  int crop_height;
  int crop_x;
  int crop_y;
  if (!AdaptFrame(frame.width(), frame.height(), timestamp_us, &adapted_width,
                  &adapted_height, &crop_width, &crop_height, &crop_x,
                  &crop_y)) {
    return;
  }

  if (useNativeBuffer() && frame.video_frame_buffer()->type() ==
                               webrtc::VideoFrameBuffer::Type::kNative) {
    NativeBuffer* frame_buffer =
        dynamic_cast<NativeBuffer*>(frame.video_frame_buffer().get());
    frame_buffer->SetScaledSize(adapted_width, adapted_height);
    OnFrame(frame);
    return;
  }

  rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer =
      frame.video_frame_buffer();

  // Video adapter has requested a down-scale. Allocate a new buffer and
  // return scaled version.
  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
      webrtc::I420Buffer::Create(adapted_width, adapted_height);
  i420_buffer->ScaleFrom(*buffer->ToI420());
  //webrtc::I420Buffer::SetBlack(i420_buffer.get()); // this works!
  //int x = 20, y = 50;
  //int w = 50 + (random()%20);
  //int h = 90 + (random()%50);  //    i420_buffer.get()->width(), i420_buffer.get()->height(),

  //libyuv::I420Rect(i420_buffer.get()->MutableDataY(), i420_buffer.get()->StrideY(),
  //      	  i420_buffer.get()->MutableDataU(), i420_buffer.get()->StrideU(),
  //      	  i420_buffer.get()->MutableDataV(), i420_buffer.get()->StrideV(),
  //      	  x, y, w, h,
  //      	  random()%200, random()&128,random()%128);
  buffer = i420_buffer;


  super::OnFrame(webrtc::VideoFrame::Builder()
		  .set_video_frame_buffer(buffer)
		  .set_rotation(frame.rotation())
		  .set_timestamp_us(translated_timestamp_us)
		  .build());
}
