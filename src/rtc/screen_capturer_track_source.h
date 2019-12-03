/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef CUSTOM_VIDEO_TRACK_SOURCE_H_
#define CUSTOM_VIDEO_TRACK_SOURCE_H_

#include "media/base/adapted_video_track_source.h"

class ScreenCapturerTrackSource : public rtc::AdaptedVideoTrackSource {
 public:
  ScreenCapturerTrackSource();
  virtual ~ScreenCapturerTrackSource();

  bool is_screencast() const override;
  absl::optional<bool> needs_denoising() const override;
  webrtc::MediaSourceInterface::SourceState state() const override;
  bool remote() const override;

};

#endif  // VIDEO_CAPTURER_H_
