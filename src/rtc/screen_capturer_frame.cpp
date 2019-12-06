/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "screen_capturer_frame.h"

namespace webrtc {

ScreenCapturerFrame::ScreenCapturerFrame() {
  capturer_ = DesktopCapturer::CreateScreenCapturer(
      DesktopCaptureOptions::CreateDefault());
  RTC_DCHECK(capturer_);

  capturer_->Start(this);
}

webrtc::DesktopCapturer::SourceList
ScreenCapturerFrame::GetScreenListAndSelectScreen() {
  webrtc::DesktopCapturer::SourceList screens;
  capturer_->GetSourceList(&screens);
  for (const auto& screen : screens) {
    capturer_->SelectSource(screen.id);
  }
  return screens;
}

void ScreenCapturerFrame::Capturer(const CaptureCallback& callback) {
  frame_.reset();
  capturer_->CaptureFrame();
  callback(std::unique_ptr<DesktopFrame>(frame_.release()));
}

void ScreenCapturerFrame::OnCaptureResult(DesktopCapturer::Result result,
		std::unique_ptr<DesktopFrame> frame) {
  if (static_cast<uint32_t>(result) != 0)
	  RTC_LOG(LS_INFO) << result;
  frame_ = std::move(frame);
}

}  // namespace webrtc
