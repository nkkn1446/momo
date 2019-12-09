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
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <stdint.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "modules/desktop_capture/linux/window_capturer_x11.h"

#include "modules/desktop_capture/desktop_frame.h"
#include "modules/desktop_capture/linux/window_list_utils.h"
#include "modules/desktop_capture/linux/x_error_trap.h"
#include "modules/desktop_capture/linux/x_window_property.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include <X11/extensions/XShm.h>

#include <memory>
#include <vector>

#include "modules/desktop_capture/desktop_geometry.h"
#include "rtc_base/constructor_magic.h"

namespace webrtc {

ScreenCapturerFrame::ScreenCapturerFrame() {
  auto options = DesktopCaptureOptions::CreateDefault();
  x_display_ = options.x_display();
  capturer_ = DesktopCapturer::CreateWindowCapturer(options);
  RTC_DCHECK(capturer_);
	RTC_LOG(LS_INFO) << __FUNCTION__;

  capturer_->Start(this);
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

void ScreenCapturerFrame::Capturer(const CaptureCallback& callback) {
	RTC_LOG(LS_INFO) << __FUNCTION__;
  frame_.reset();
  // TODO:ここで落ちてる
  // capturer_->CaptureFrame();
  DesktopCapturer::SourceList sources;
  capturer_->GetSourceList(&sources);
  RTC_LOG(LS_INFO) << sources.size();

  // Verify that we can select and capture each window.
  for (auto it = sources.begin(); it != sources.end(); ++it) {
    frame_.reset();
  // XAtomCache cache(XOpenDisplay(NULL));
  Display* display = x_display_->display();
  XAtomCache cache(display);

  auto root_window_ = RootWindow(display, DefaultScreen(display));
  if (root_window_ == BadValue) {
    RTC_LOG(LS_ERROR) << "Unable to get the root window";
  }

  auto gc_ = XCreateGC(display, root_window_, 0, NULL);
  if (gc_ == NULL) {
    RTC_LOG(LS_ERROR) << "Unable to get graphics context";
  }

  x_display_->AddEventHandler(ConfigureNotify, static_cast<webrtc::WindowCapturerX11*>(capturer_.get()));

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

    //XWindowProperty<uint8_t> icc_profile_property(cache.display(), it->id,
    //                                              cache.IccProfile());
  Atom atom = cache.IccProfile();
  if (atom == None)
      RTC_LOG(LS_INFO) << "None";
  bool is_valid_ = false;
  unsigned long size_ = 0;
  unsigned char* data_;
  const int kBitsPerByte = 8;
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after;  // NOLINT: type required by XGetWindowProperty
  int status = XGetWindowProperty((Display*)cache.display(), (Window)it->id, atom, 0L, ~0L, False,
                                  AnyPropertyType, &actual_type, &actual_format,
                                  &size_, &bytes_after, &data_);
  if (XGetAtomName(cache.display(), atom))
      RTC_LOG(LS_INFO) << XGetAtomName(cache.display(), atom);
  if (actual_type == None)
      RTC_LOG(LS_INFO) << "None";
  if (status == Success) {
      RTC_LOG(LS_INFO) << actual_format;
  if ((sizeof(uint8_t) * kBitsPerByte) == actual_format) {
    is_valid_ = true;
  } else {
      RTC_LOG(LS_INFO) << "Failed to get icc profile";
    size_ = 0;
  }
  }
    if (is_valid_) {
    } else {
      RTC_LOG(LS_INFO) << "Failed to get icc profile";
    }
    if (size_) {
    } else {
      RTC_LOG(LS_INFO) << "Failed to get icc profile";
    }
    //if (icc_profile_property.is_valid() && icc_profile_property.size() > 0) {
    //  auto icc_profile_ = std::vector<uint8_t>(
    //      icc_profile_property.data(),
    //      icc_profile_property.data() + icc_profile_property.size());
    //} else {
    //  RTC_LOG(LS_INFO) << "Failed to get icc profile";
    //}

    if (capturer_->SelectSource(it->id)) {
	RTC_LOG(LS_INFO) << __FUNCTION__;
      // capturer_->CaptureFrame();
    }

    // If we failed to capture a window make sure it no longer exists.
    if (!frame_.get()) {
      DesktopCapturer::SourceList new_list;
      for (auto new_list_it = new_list.begin(); new_list_it != new_list.end();
           ++new_list_it) {
	RTC_LOG(LS_INFO) << __FUNCTION__;
      }
    }
  }
  //callback(std::unique_ptr<DesktopFrame>(frame_.release()));
}

void ScreenCapturerFrame::OnCaptureResult(DesktopCapturer::Result result,
		std::unique_ptr<DesktopFrame> frame) {
	RTC_LOG(LS_INFO) << __FUNCTION__;
  if (static_cast<uint32_t>(result) != 0) {
	  RTC_LOG(LS_INFO) << result;
	  return;
  }
  frame_ = std::move(frame);
}

}  // namespace webrtc
