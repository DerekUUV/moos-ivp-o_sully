/************************************************************/
/*    NAME: OSully                                          */
/*    ORGN: MIT                                             */
/*    FILE: BHV_ZigLeg.h                                   */
/*    DATE:                                                 */
/************************************************************/

#ifndef ZigLeg_HEADER
#define ZigLeg_HEADER

#include <string>
#include "IvPBehavior.h"

class BHV_ZigLeg : public IvPBehavior {
public:
  BHV_ZigLeg(IvPDomain);
  ~BHV_ZigLeg() {};

  bool         setParam(std::string, std::string);
  void         onSetParamComplete();
  void         onCompleteState();
  void         onIdleState();
  void         onHelmStart();
  void         postConfigStatus();
  void         onRunToIdleState();
  void         onIdleToRunState();
  IvPFunction* onRunState();

protected: // Configuration parameters
  double m_zig_angle;     // heading offset in degrees (default 45)
  double m_zig_duration;  // how long to apply the zig (default 10 sec)
  double m_zig_delay;     // seconds after waypoint before zig starts (default 5)

protected: // State variables
  double m_last_wpt_index;  // last known WPT_INDEX value
  double m_wpt_change_time; // helm time when WPT_INDEX last changed
  double m_zig_start_time;  // helm time when zig began
  double m_zig_heading;     // captured heading at zig start
};

#define IVP_EXPORT_FUNCTION

extern "C" {
  IVP_EXPORT_FUNCTION IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_ZigLeg(domain);}
}
#endif
