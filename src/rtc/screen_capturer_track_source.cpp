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

ScreenCapturerTrackSource::ScreenCapturerTrackSource()
    : AdaptedVideoTrackSource(4) {}
ScreenCapturerTrackSource::~ScreenCapturerTrackSource() {}

bool ScreenCapturerTrackSource::is_screencast() const {
  return true;
}

absl::optional<bool> ScreenCapturerTrackSource::needs_denoising() const {
  return false;
}

webrtc::MediaSourceInterface::SourceState ScreenCapturerTrackSource::state()
    const {
  return SourceState::kLive;
}

bool ScreenCapturerTrackSource::remote() const {
  return false;
}

