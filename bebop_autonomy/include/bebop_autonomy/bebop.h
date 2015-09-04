#ifndef BEBOP_H
#define BEBOP_H

#define BEBOP_ERR_STR_SZ  150

#include <sys/syscall.h>
#include <sys/types.h>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

extern "C"
{
  #include "libARSAL/ARSAL.h"
  #include "libARController/ARController.h"
}

// Forward declarations
extern "C"
{
  struct ARDISCOVERY_Device_t;
}
namespace ros
{
  class NodeHandle;
}

namespace bebop_autonomy
{

namespace util
{
inline long int GetLWPId()
{
  return syscall(SYS_gettid);
}

}  // namespace util

// Forward declarations
class BebopArdrone3Config;
class VideoDecoder;
namespace cb
{
  class AbstractCommand;
}  // namespace cb
#define FORWARD_DECLARATIONS
#include "bebop_autonomy/autogenerated/common_state_callback_includes.h"
#include "bebop_autonomy/autogenerated/ardrone3_state_callback_includes.h"
#include "bebop_autonomy/autogenerated/ardrone3_setting_callback_includes.h"
#undef FORWARD_DECLARATIONS

class Bebop
{
private:
  static const char* LOG_TAG;
  boost::atomic<bool> is_connected_;
  boost::atomic<bool> is_streaming_started_;
  ARDISCOVERY_Device_t* device_ptr_;
  ARCONTROLLER_Device_t* device_controller_ptr_;
  eARCONTROLLER_ERROR error_;
  eARCONTROLLER_DEVICE_STATE device_state_;
  ARSAL_Sem_t state_sem_;
  boost::shared_ptr<VideoDecoder> video_decoder_ptr_;

  // Navdata Callbacks
#define DEFINE_SHARED_PTRS
#include "bebop_autonomy/autogenerated/common_state_callback_includes.h"
#include "bebop_autonomy/autogenerated/ardrone3_state_callback_includes.h"
#include "bebop_autonomy/autogenerated/ardrone3_setting_callback_includes.h"
#undef DEFINE_SHARED_PTRS

//  boost::mutex callback_map_mutex_;
  typedef std::map<eARCONTROLLER_DICTIONARY_KEY, boost::shared_ptr<cb::AbstractCommand> > callback_map_t;
  typedef std::pair<eARCONTROLLER_DICTIONARY_KEY, boost::shared_ptr<cb::AbstractCommand> > callback_map_pair_t;
  callback_map_t callback_map_;

  // sync
  mutable boost::condition_variable frame_avail_cond_;
  mutable boost::mutex frame_avail_mutex_;
  mutable bool is_frame_avail_;

  static void BatteryStateChangedCallback(uint8_t percent, void *bebop_void_ptr);
  static void StateChangedCallback(eARCONTROLLER_DEVICE_STATE new_state, eARCONTROLLER_ERROR error, void *bebop_void_ptr);
  static void CommandReceivedCallback(eARCONTROLLER_DICTIONARY_KEY cmd_key, ARCONTROLLER_DICTIONARY_ELEMENT_t* element_dict_ptr, void* bebop_void_ptr);
  static void FrameReceivedCallback(ARCONTROLLER_Frame_t *frame, void *bebop_void_ptr_);

  // nothrow
  void Cleanup();

  void ThrowOnInternalError(const std::string& message = std::string());
  void ThrowOnCtrlError(const eARCONTROLLER_ERROR& error, const std::string& message = std::string());

public:

  inline ARSAL_Sem_t* GetStateSemPtr() {return &state_sem_;}
  inline const ARCONTROLLER_Device_t* GetControllerCstPtr() const {return device_controller_ptr_;}

  inline bool IsConnected() const {return is_connected_;}
  inline bool IsStreamingStarted() const {return is_streaming_started_;}

  Bebop(ARSAL_Print_Callback_t custom_print_callback = 0);
  ~Bebop();

  void Connect(ros::NodeHandle& nh, ros::NodeHandle& priv_nh);
  void StartStreaming();

  // Disable all data callback and streaming (nothrow)
  void StopStreaming();
  // nothrow
  void Disconnect();

  void RequestAllSettings();
  void ResetAllSettings();
  void UpdateSettings(const bebop_autonomy::BebopArdrone3Config& config);

  void Takeoff();
  void Land();
  void Emergency();
  void FlatTrim();
  // false: Stop, true: Start
  void NavigateHome(const bool& start_stop);
  void AnimationFlip(const uint8_t& anim_id);

  // -1..1
  void Move(const double& roll, const double& pitch, const double& gaz_speed, const double& yaw_speed);
  void MoveCamera(const double& tilt, const double& pan);

  // This function is blocking and runs in the caller's thread's context
  // which is different from FrameReceivedCallback's context
  bool GetFrontCameraFrame(std::vector<uint8_t>& buffer, uint32_t &width, uint32_t &height) const;
  uint32_t GetFrontCameraFrameWidth() const;
  uint32_t GetFrontCameraFrameHeight() const;

};

}  // namespace bebop_autonomy


#endif  // BEBOP_H
