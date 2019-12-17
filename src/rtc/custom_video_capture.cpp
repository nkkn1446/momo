/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "custom_video_capture.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <regex>

#include <new>
#include <string>

#include "api/scoped_refptr.h"
#include "media/base/video_common.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"

namespace webrtc {
CustomVideoCaptureModule::CustomVideoCaptureModule()
    : CustomVideoCaptureImpl(),
      _deviceId(-1),
      _buffersAllocatedByDevice(-1),
      _currentWidth(-1),
      _currentHeight(-1),
      _currentFrameRate(-1),
      _captureStarted(false),
      _captureVideoType(VideoType::kI420),
      _pool(NULL) {}

int32_t CustomVideoCaptureModule::Init(const char* deviceUniqueIdUTF8) {
  int len = strlen((const char*)deviceUniqueIdUTF8);
  _deviceUniqueId = new (std::nothrow) char[len + 1];
  if (_deviceUniqueId) {
    memcpy(_deviceUniqueId, deviceUniqueIdUTF8, len + 1);
  }

  int fd;
  char device[32];
  bool found = false;

  /* detect /dev/video [0-63] entries */
  int n;
  for (n = 0; n < 64; n++) {
    sprintf(device, "/dev/video%d", n);
    if ((fd = open(device, O_RDONLY)) != -1) {
      // query device capabilities
      struct v4l2_capability cap;
      if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
        if (cap.bus_info[0] != 0) {
          if (strncmp((const char*)cap.bus_info,
                      (const char*)deviceUniqueIdUTF8,
                      strlen((const char*)deviceUniqueIdUTF8)) ==
              0)  // match with device id
          {
            close(fd);
            found = true;
            break;  // fd matches with device unique id supplied
          }
        }
      }
      close(fd);  // close since this is not the matching device
    }
  }
  if (!found) {
    RTC_LOG(LS_INFO) << "no matching device found";
    return -1;
  }
  _deviceId = n;  // store the device id
  return 0;
}

CustomVideoCaptureModule::~CustomVideoCaptureModule() {
  StopCapture();
}

int32_t CustomVideoCaptureModule::StartCapture(
    const VideoCaptureCapability& capability) {
  if (_captureStarted) {
    if (capability.width == _currentWidth &&
        capability.height == _currentHeight &&
        _captureVideoType == capability.videoType) {
      return 0;
    } else {
      StopCapture();
    }
  }

  rtc::CritScope cs(&_captureCritSect);

  // Supported video formats in preferred order.
  // If the requested resolution is larger than VGA, we prefer MJPEG. Go for
  // I420 otherwise.
  const int nFormats = 5;
  unsigned int fmts[nFormats];
  if (capability.width > 640 || capability.height > 480) {
    fmts[0] = V4L2_PIX_FMT_MJPEG;
    fmts[1] = V4L2_PIX_FMT_YUV420;
    fmts[2] = V4L2_PIX_FMT_YUYV;
    fmts[3] = V4L2_PIX_FMT_UYVY;
    fmts[4] = V4L2_PIX_FMT_JPEG;
  } else {
    fmts[0] = V4L2_PIX_FMT_YUV420;
    fmts[1] = V4L2_PIX_FMT_YUYV;
    fmts[2] = V4L2_PIX_FMT_UYVY;
    fmts[3] = V4L2_PIX_FMT_MJPEG;
    fmts[4] = V4L2_PIX_FMT_JPEG;
  }

  const int PIPE_BUF_SIZE=256;
  char  buf[PIPE_BUF_SIZE];
  std::string cmd = "ffmpeg -f v4l2 -i /dev/video0 2>&1";
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
		  _currentWidth = atoi(match.str(1).c_str());
		  _currentHeight = atoi(match.str(2).c_str());
          }
  }

  cmd="ffmpeg -f v4l2 -i /dev/video0 -f image2pipe -vcodec rawvideo - 2>&1";
  if ( (fp_=popen(cmd.c_str(),"r")) ==NULL) {
	  return -1;
  }

  _captureVideoType = VideoType::kYUY2;

  // Trying to set frame rate, before check driver capability.
  bool driver_framerate_support = true;
  struct v4l2_streamparm streamparms;
  memset(&streamparms, 0, sizeof(streamparms));
  streamparms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  // check the capability flag is set to V4L2_CAP_TIMEPERFRAME.
  if (streamparms.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
	  _currentFrameRate = capability.maxFPS;
  }
  // If driver doesn't support framerate control, need to hardcode.
  // Hardcoding the value based on the frame size.
  if (!driver_framerate_support) {
    if (_currentWidth >= 800 && _captureVideoType != VideoType::kMJPEG) {
      _currentFrameRate = 15;
    } else {
      _currentFrameRate = 30;
    }
  }

  if (!AllocateVideoBuffers()) {
    RTC_LOG(LS_INFO) << "failed to allocate video capture buffers";
    return -1;
  }

  // start capture thread;
  if (!_captureThread) {
    quit_ = false;
    _captureThread.reset(
        new rtc::PlatformThread(CustomVideoCaptureModule::CaptureThread, this,
                                "CaptureThread", rtc::kHighPriority));
    _captureThread->Start();
  }

  _captureStarted = true;
  return 0;
}

int32_t CustomVideoCaptureModule::StopCapture() {
  if (_captureThread) {
    {
      rtc::CritScope cs(&_captureCritSect);
      quit_ = true;
    }
    // Make sure the capture thread stop stop using the critsect.
    _captureThread->Stop();
    _captureThread.reset();
  }

  rtc::CritScope cs(&_captureCritSect);
  if (_captureStarted) {
    _captureStarted = false;

    DeAllocateVideoBuffers();
  }

  return 0;
}

// critical section protected by the caller

bool CustomVideoCaptureModule::AllocateVideoBuffers() {
  struct v4l2_requestbuffers rbuffer;
  memset(&rbuffer, 0, sizeof(v4l2_requestbuffers));

  rbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  rbuffer.memory = V4L2_MEMORY_MMAP;
  rbuffer.count = kNoOfV4L2Bufffers;

  _buffersAllocatedByDevice = rbuffer.count;

  // Map the buffers
  _pool = new Buffer[rbuffer.count];

  for (unsigned int i = 0; i < rbuffer.count; i++) {
    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(v4l2_buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = i;

    //_pool[i].start = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE,
    //                      MAP_SHARED, _deviceFd, buffer.m.offset);

    if (MAP_FAILED == _pool[i].start) {
      for (unsigned int j = 0; j < i; j++)
        munmap(_pool[j].start, _pool[j].length);
      return false;
    }

    _pool[i].length = buffer.length;
  }
  return true;
}

bool CustomVideoCaptureModule::DeAllocateVideoBuffers() {
  // unmap buffers
  for (int i = 0; i < _buffersAllocatedByDevice; i++)
    munmap(_pool[i].start, _pool[i].length);

  delete[] _pool;

  return true;
}

bool CustomVideoCaptureModule::CaptureStarted() {
  return _captureStarted;
}

void CustomVideoCaptureModule::CaptureThread(void* obj) {
  CustomVideoCaptureModule* capture = static_cast<CustomVideoCaptureModule*>(obj);
  while (capture->CaptureProcess()) {
  }
}
bool CustomVideoCaptureModule::CaptureProcess() {
  {
    rtc::CritScope cs(&_captureCritSect);

    if (quit_) {
      return false;
    }

    if (_captureStarted) {
      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(struct v4l2_buffer));
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;

      VideoCaptureCapability frameInfo;
      frameInfo.width = _currentWidth;
      frameInfo.height = _currentHeight;
      frameInfo.videoType = _captureVideoType;

      int height = _currentHeight;
      int width = _currentWidth;
      unsigned int screen_size = width*height*2;
      std::vector<uint8_t> src(screen_size);
      auto size = fread((void*)src.data(), 1, screen_size, fp_);
      RTC_LOG(LS_INFO) << size;
      if(size!=screen_size){
	      RTC_LOG(LS_INFO) << "hoge";
	      return true;
      }

      // convert to to I420 if needed
      VideoFrame captureFrame = IncomingFrame((unsigned char*)src.data(), screen_size, frameInfo);
      DeliverCapturedFrame(captureFrame);
    }
  }
  usleep(0);
  return true;
}

int32_t CustomVideoCaptureModule::CaptureSettings(
    VideoCaptureCapability& settings) {
  settings.width = _currentWidth;
  settings.height = _currentHeight;
  settings.maxFPS = _currentFrameRate;
  settings.videoType = _captureVideoType;

  return 0;
}
}  // namespace webrtc
