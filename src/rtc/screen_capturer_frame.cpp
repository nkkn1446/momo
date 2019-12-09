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
#include <X11/extensions/Xfixes.h>
#include "modules/desktop_capture/linux/window_capturer_x11.h"

namespace webrtc {

ScreenCapturerFrame::ScreenCapturerFrame() {
  options_ = DesktopCaptureOptions::CreateDefault();
  Display* display = options_.x_display()->display();
  auto atom_cache_ = std::make_unique<XAtomCache>(display);

  auto x11 = std::make_unique<WindowCapturerX11>(options_);
  x11->Start(this);

  auto root_window_ = RootWindow(display, DefaultScreen(display));
  if (root_window_ == BadValue) {
    RTC_LOG(LS_ERROR) << "Unable to get the root window";
  }

  auto gc_ = XCreateGC(display, root_window_, 0, NULL);
  if (gc_ == NULL) {
    RTC_LOG(LS_ERROR) << "Unable to get graphics context";
  }

  options_.x_display()->AddEventHandler(ConfigureNotify, x11.get());

  // Check for XFixes extension. This is required for cursor shape
  // notifications, and for our use of XDamage.
  int xfixes_event_base_ = -1;
  int xfixes_error_base_ = -1;
  if (XFixesQueryExtension(display, &xfixes_event_base_,
                           &xfixes_error_base_)) {
  } else {
    RTC_LOG(LS_INFO) << "X server does not support XFixes.";
  }

  // Register for changes to the dimensions of the root window.
  XSelectInput(display, root_window_, StructureNotifyMask);

  DesktopCapturer::SourceList sources;
  x11->GetSourceList(&sources);
  auto it = sources.begin();
  RTC_LOG(LS_INFO) << sources.size();
  frame_.reset();
  if (!x11->SelectSource(it->id)) {
          RTC_LOG(LS_INFO) << __FUNCTION__;
  }

  capturer_ = std::move(x11);
//  capturer_ = DesktopCapturer::CreateScreenCapturer(
//options_);
//  RTC_DCHECK(capturer_);

//  capturer_->Start(this);
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
  //Display* display = options_.x_display()->display();
  //auto atom_cache_ = std::make_unique<XAtomCache>(display);

  //auto root_window_ = RootWindow(display, DefaultScreen(display));
  //if (root_window_ == BadValue) {
  //  RTC_LOG(LS_ERROR) << "Unable to get the root window";
  //}

  //auto gc_ = XCreateGC(display, root_window_, 0, NULL);
  //if (gc_ == NULL) {
  //  RTC_LOG(LS_ERROR) << "Unable to get graphics context";
  //}

  //options_.x_display()->AddEventHandler(ConfigureNotify, static_cast<ScreenCapturerX11*>(capturer_.get()));

  //// Check for XFixes extension. This is required for cursor shape
  //// notifications, and for our use of XDamage.
  //int xfixes_event_base_ = -1;
  //int xfixes_error_base_ = -1;
  //if (XFixesQueryExtension(display, &xfixes_event_base_,
  //                         &xfixes_error_base_)) {
  //} else {
  //  RTC_LOG(LS_INFO) << "X server does not support XFixes.";
  //}

  //// Register for changes to the dimensions of the root window.
  //XSelectInput(display, root_window_, StructureNotifyMask);

  //XServerPixelBuffer x_server_pixel_buffer_;
  //if (!x_server_pixel_buffer_.Init(atom_cache_.get(),
  //                                 DefaultRootWindow(display))) {
  //  RTC_LOG(LS_ERROR) << "Failed to initialize pixel buffer.";
  //}

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
