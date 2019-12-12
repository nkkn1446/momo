/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "custom_server_pixel_buffer.h"

#include <X11/Xutil.h>
#include <regex>
#include <stdint.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "modules/desktop_capture/desktop_frame.h"
#include "modules/desktop_capture/linux/window_list_utils.h"
#include "modules/desktop_capture/linux/x_error_trap.h"
#include "modules/desktop_capture/linux/x_window_property.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace webrtc {

namespace {

// Returns the number of bits |mask| has to be shifted left so its last
// (most-significant) bit set becomes the most-significant bit of the word.
// When |mask| is 0 the function returns 31.
uint32_t MaskToShift(uint32_t mask) {
  int shift = 0;
  if ((mask & 0xffff0000u) == 0) {
    mask <<= 16;
    shift += 16;
  }
  if ((mask & 0xff000000u) == 0) {
    mask <<= 8;
    shift += 8;
  }
  if ((mask & 0xf0000000u) == 0) {
    mask <<= 4;
    shift += 4;
  }
  if ((mask & 0xc0000000u) == 0) {
    mask <<= 2;
    shift += 2;
  }
  if ((mask & 0x80000000u) == 0)
    shift += 1;

  return shift;
}

}  // namespace

CustomServerPixelBuffer::CustomServerPixelBuffer() {}

CustomServerPixelBuffer::~CustomServerPixelBuffer() {
  Release();
}

void CustomServerPixelBuffer::Release() {
  if (x_image_) {
    XDestroyImage(x_image_);
    x_image_ = nullptr;
  }
  if (x_shm_image_) {
    XDestroyImage(x_shm_image_);
    x_shm_image_ = nullptr;
  }
  if (shm_pixmap_) {
    XFreePixmap(display_, shm_pixmap_);
    shm_pixmap_ = 0;
  }
  if (shm_gc_) {
    XFreeGC(display_, shm_gc_);
    shm_gc_ = nullptr;
  }

  ReleaseSharedMemorySegment();

  window_ = 0;

  if (fp_)
    pclose(fp_);
}

void CustomServerPixelBuffer::ReleaseSharedMemorySegment() {
  if (!shm_segment_info_)
    return;
  if (shm_segment_info_->shmaddr != nullptr)
    shmdt(shm_segment_info_->shmaddr);
  if (shm_segment_info_->shmid != -1)
    shmctl(shm_segment_info_->shmid, IPC_RMID, 0);
  delete shm_segment_info_;
  shm_segment_info_ = nullptr;
}

bool CustomServerPixelBuffer::Init(XAtomCache* cache, Window window) {
  Release();
  display_ = cache->display();

  XWindowAttributes attributes;
  if (!GetWindowRect(display_, window, &window_rect_, &attributes)) {
    return false;
  }
  // window_size_ = window_rect_.size();

  const int PIPE_BUF_SIZE=256;
  char  buf[PIPE_BUF_SIZE];
  std::string cmd = "ffmpeg -i /home/tnakao/sample.mp4 2>&1";
  if ( (fp_=popen(cmd.c_str(),"r")) ==NULL) {
          return -1;
  }
  std::string data;
  while(fgets(buf, PIPE_BUF_SIZE, fp_) != NULL) {
          data+=std::string(buf);
  }
  pclose(fp_);

  const char* pattern = " ([0-9]+)x([0-9]+)[,| ]";
  std::regex re(pattern);
  std::cmatch match;
  if ( std::regex_search(data.c_str(), match, re) ) {
          if(match.size()>=3){
        	  window_size_.set(atoi(match.str(1).c_str()),atoi(match.str(2).c_str()));
          }
  }

  cmd="ffmpeg -i /home/tnakao/sample.mp4 -f image2pipe -pix_fmt argb -vcodec rawvideo - 2>&1";
  if ( (fp_=popen(cmd.c_str(),"r")) ==NULL) {
	  return -1;
  }

  if (cache->IccProfile() != None) {
    // |window| is the root window when doing screen capture.
    XWindowProperty<uint8_t> icc_profile_property(cache->display(), window,
                                                  cache->IccProfile());
    if (icc_profile_property.is_valid() && icc_profile_property.size() > 0) {
      icc_profile_ = std::vector<uint8_t>(
          icc_profile_property.data(),
          icc_profile_property.data() + icc_profile_property.size());
    } else {
      RTC_LOG(LS_WARNING) << "Failed to get icc profile";
    }
  }

  window_ = window;
  // InitShm(attributes);

  return true;
}

void CustomServerPixelBuffer::InitShm(const XWindowAttributes& attributes) {
  Visual* default_visual = attributes.visual;
  int default_depth = attributes.depth;

  int major, minor;
  Bool have_pixmaps;
  if (!XShmQueryVersion(display_, &major, &minor, &have_pixmaps)) {
    // Shared memory not supported. CaptureRect will use the XImage API instead.
    return;
  }

  bool using_shm = false;
  shm_segment_info_ = new XShmSegmentInfo;
  shm_segment_info_->shmid = -1;
  shm_segment_info_->shmaddr = nullptr;
  shm_segment_info_->readOnly = False;
  x_shm_image_ = XShmCreateImage(display_, default_visual, default_depth,
                                 ZPixmap, 0, shm_segment_info_,
                                 window_rect_.width(), window_rect_.height());
  if (x_shm_image_) {
    shm_segment_info_->shmid =
        shmget(IPC_PRIVATE, x_shm_image_->bytes_per_line * x_shm_image_->height,
               IPC_CREAT | 0600);
    if (shm_segment_info_->shmid != -1) {
      void* shmat_result = shmat(shm_segment_info_->shmid, 0, 0);
      if (shmat_result != reinterpret_cast<void*>(-1)) {
        shm_segment_info_->shmaddr = reinterpret_cast<char*>(shmat_result);
        x_shm_image_->data = shm_segment_info_->shmaddr;

        XErrorTrap error_trap(display_);
        using_shm = XShmAttach(display_, shm_segment_info_);
        XSync(display_, False);
        if (error_trap.GetLastErrorAndDisable() != 0)
          using_shm = false;
        if (using_shm) {
          RTC_LOG(LS_VERBOSE)
              << "Using X shared memory segment " << shm_segment_info_->shmid;
        }
      }
    } else {
      RTC_LOG(LS_WARNING) << "Failed to get shared memory segment. "
                             "Performance may be degraded.";
    }
  }

  if (!using_shm) {
    RTC_LOG(LS_WARNING)
        << "Not using shared memory. Performance may be degraded.";
    ReleaseSharedMemorySegment();
    return;
  }

  if (have_pixmaps)
    have_pixmaps = InitPixmaps(default_depth);

  shmctl(shm_segment_info_->shmid, IPC_RMID, 0);
  shm_segment_info_->shmid = -1;

  RTC_LOG(LS_VERBOSE) << "Using X shared memory extension v" << major << "."
                      << minor << " with" << (have_pixmaps ? "" : "out")
                      << " pixmaps.";
}

bool CustomServerPixelBuffer::InitPixmaps(int depth) {
  if (XShmPixmapFormat(display_) != ZPixmap)
    return false;

  {
    XErrorTrap error_trap(display_);
    shm_pixmap_ = XShmCreatePixmap(
        display_, window_, shm_segment_info_->shmaddr, shm_segment_info_,
        window_rect_.width(), window_rect_.height(), depth);
    XSync(display_, False);
    if (error_trap.GetLastErrorAndDisable() != 0) {
      // |shm_pixmap_| is not not valid because the request was not processed
      // by the X Server, so zero it.
      shm_pixmap_ = 0;
      return false;
    }
  }

  {
    XErrorTrap error_trap(display_);
    XGCValues shm_gc_values;
    shm_gc_values.subwindow_mode = IncludeInferiors;
    shm_gc_values.graphics_exposures = False;
    shm_gc_ = XCreateGC(display_, window_,
                        GCSubwindowMode | GCGraphicsExposures, &shm_gc_values);
    XSync(display_, False);
    if (error_trap.GetLastErrorAndDisable() != 0) {
      XFreePixmap(display_, shm_pixmap_);
      shm_pixmap_ = 0;
      shm_gc_ = 0;  // See shm_pixmap_ comment above.
      return false;
    }
  }

  return true;
}

void CustomServerPixelBuffer::Synchronize() {
  if (shm_segment_info_ && !shm_pixmap_) {
    // XShmGetImage can fail if the display is being reconfigured.
    XErrorTrap error_trap(display_);
    // XShmGetImage fails if the window is partially out of screen.
    xshm_get_image_succeeded_ =
        XShmGetImage(display_, window_, x_shm_image_, 0, 0, AllPlanes);
  }
}

bool CustomServerPixelBuffer::CaptureRect(const DesktopRect& rect,
                                     DesktopFrame* frame) {
  XImage* image;
  uint8_t* data;
  int height;

  //if (shm_pixmap_) {
  //        XCopyArea(display_, window_, shm_pixmap_, shm_gc_, rect.left(),
  //      		  rect.top(), rect.width(), rect.height(), rect.left(),
  //      		  rect.top());
  //        XSync(display_, False);
  //}

  //image = x_shm_image_;
  //data = reinterpret_cast<uint8_t*>(image->data) +
  //rect.top() * image->bytes_per_line +
  //rect.left() * image->bits_per_pixel / 8;
  
  auto width = window_size_.width();
  height = window_size_.height();
  unsigned int screen_size = width*height*DesktopFrame::kBytesPerPixel;
  std::vector<uint8_t> src(screen_size);
  auto size = fread((void*)src.data(), 1, screen_size, fp_);
  RTC_LOG(LS_INFO) << size;
  if(size!=screen_size){
	  RTC_LOG(LS_INFO) << "hoge";
	  return true;
  }
  data = src.data();

  // TODO:ffmpegに差し替える
  // int src_stride = image->bytes_per_line;
  int src_stride = width * DesktopFrame::kBytesPerPixel;
  int dst_x = rect.left(), dst_y = rect.top();

  uint8_t* dst_pos = frame->data() + frame->stride() * dst_y;
  dst_pos += dst_x * DesktopFrame::kBytesPerPixel;

  height = rect.height();
  int row_bytes = rect.width() * DesktopFrame::kBytesPerPixel;
  for (int y = 0; y < height; ++y) {
    memcpy(dst_pos, data, row_bytes);
    data += src_stride;
    dst_pos += frame->stride();
  }

  if (!icc_profile_.empty())
    frame->set_icc_profile(icc_profile_);

  return true;
}

}  // namespace webrtc
