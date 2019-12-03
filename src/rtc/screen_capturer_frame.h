/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>

#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "modules/desktop_capture/desktop_region.h"
//#include "modules/desktop_capture/mock_desktop_capturer_callback.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/logging.h"

#if defined(WEBRTC_WIN)
#include "modules/desktop_capture/win/screen_capturer_win_directx.h"
#endif  // defined(WEBRTC_WIN)

namespace webrtc {

class ScreenCapturerFrame {
 public:
  void SetUp();
  webrtc::DesktopCapturer::SourceList
	  GetScreenListAndSelectScreen();
  void Capturer();
 protected:

  std::unique_ptr<DesktopCapturer> capturer_;
  //MockDesktopCapturerCallback callback_;
};

}
