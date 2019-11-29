/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "screen_capture_track_source.h"

ScreenCaptureTrackSource::ScreenCaptureTrackSource()
    : AdaptedVideoTrackSource(4) {}
ScreenCaptureTrackSource::~ScreenCaptureTrackSource() {}

bool ScreenCaptureTrackSource::is_screencast() const {
  return true;
}

absl::optional<bool> ScreenCaptureTrackSource::needs_denoising() const {
  return false;
}

webrtc::MediaSourceInterface::SourceState ScreenCaptureTrackSource::state()
    const {
  return SourceState::kLive;
}

bool ScreenCaptureTrackSource::remote() const {
  return false;
}

