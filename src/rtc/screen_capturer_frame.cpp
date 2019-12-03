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

#include "modules/desktop_capture/linux/screen_capturer_x11.h"

namespace webrtc {

void ScreenCapturerFrame::SetUp() {
  capturer_ = DesktopCapturer::CreateScreenCapturer(
      DesktopCaptureOptions::CreateDefault());
  RTC_DCHECK(capturer_);
  RTC_LOG(LS_INFO) << __FUNCTION__;
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

void ScreenCapturerFrame::Capturer() {
  // Assume that Start() treats the screen as invalid initially.
  std::unique_ptr<DesktopFrame> frame;
  //EXPECT_CALL(callback_,
  //            OnCaptureResultPtr(DesktopCapturer::Result::SUCCESS, _))
  //    .WillOnce(SaveUniquePtrArg(&frame));

  //capturer_->Start(&callback_);
  capturer_->CaptureFrame();
}

}  // namespace webrtc
