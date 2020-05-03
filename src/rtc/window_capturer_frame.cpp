/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "window_capturer_frame.h"
#include "custom_window_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_capturer_differ_wrapper.h"

namespace webrtc {

WindowCapturerFrame::WindowCapturerFrame(const std::string& window_title) {
  auto options = DesktopCaptureOptions::CreateDefault();
  capturer_ = std::unique_ptr<DesktopCapturer>(new CustomWindowCapturer(options));
  if (capturer_ && options.detect_updated_region()) {
  	capturer_.reset(new DesktopCapturerDifferWrapper(std::move(capturer_)));
  }
  RTC_DCHECK(capturer_);

  capturer_->Start(this);

  DesktopCapturer::SourceList sources;
  capturer_->GetSourceList(&sources);
  auto it = find_if(sources.begin(), sources.end(), [window_title](const auto& source){return source.title.find(window_title) != std::string::npos;});
  if (it == sources.end()) {
          RTC_LOG(LS_INFO) << __FUNCTION__;
  }
  if (!capturer_->SelectSource(it->id)) {
          RTC_LOG(LS_INFO) << __FUNCTION__;
  }
}

webrtc::DesktopCapturer::SourceList
WindowCapturerFrame::GetScreenListAndSelectScreen() {
  webrtc::DesktopCapturer::SourceList screens;
  capturer_->GetSourceList(&screens);
  for (const auto& screen : screens) {
    capturer_->SelectSource(screen.id);
  }
  return screens;
}

void WindowCapturerFrame::Capturer(const CaptureCallback& callback) {
  frame_.reset();
  capturer_->CaptureFrame();
  callback(std::unique_ptr<DesktopFrame>(frame_.release()));
}

void WindowCapturerFrame::OnCaptureResult(DesktopCapturer::Result result,
		std::unique_ptr<DesktopFrame> frame) {
  if (static_cast<uint32_t>(result) != 0)
	  RTC_LOG(LS_INFO) << result;
  frame_ = std::move(frame);
}

}  // namespace webrtc
