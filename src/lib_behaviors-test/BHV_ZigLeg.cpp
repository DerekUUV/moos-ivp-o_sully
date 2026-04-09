/************************************************************/
/*    NAME: OSully                                          */
/*    ORGN: MIT                                             */
/*    FILE: BHV_ZigLeg.cpp                                 */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include <cstdlib>
#include "MBUtils.h"
#include "BuildUtils.h"
#include "ZAIC_PEAK.h"
#include "BHV_ZigLeg.h"

using namespace std;

//---------------------------------------------------------------
// Constructor

BHV_ZigLeg::BHV_ZigLeg(IvPDomain domain) :
  IvPBehavior(domain)
{
  IvPBehavior::setParam("name", "defaultname");

  // Only need to influence course (heading)
  m_domain = subDomain(m_domain, "course");

  // Configuration parameter defaults
  m_zig_angle    = 45;  // degrees
  m_zig_duration = 10;  // seconds
  m_zig_delay    = 5;   // seconds

  // State variable initialization
  m_last_wpt_index  = -1;
  m_wpt_change_time = -1;
  m_zig_start_time  = -1;
  m_zig_heading     = 0;

  addInfoVars("NAV_HEADING");
  addInfoVars("WPT_INDEX", "no_warning");
}

//---------------------------------------------------------------
// Procedure: setParam()

bool BHV_ZigLeg::setParam(string param, string val)
{
  param = tolower(param);
  double double_val = atof(val.c_str());

  if((param == "zig_angle") && isNumber(val)) {
    m_zig_angle = double_val;
    return(true);
  }
  else if((param == "zig_duration") && isNumber(val)) {
    m_zig_duration = double_val;
    return(true);
  }
  else if((param == "zig_delay") && isNumber(val)) {
    m_zig_delay = double_val;
    return(true);
  }

  return IvPBehavior::setParam(param, val);
}

//---------------------------------------------------------------
// Procedure: onSetParamComplete()

void BHV_ZigLeg::onSetParamComplete()
{
}

//---------------------------------------------------------------
// Procedure: onHelmStart()

void BHV_ZigLeg::onHelmStart()
{
}

//---------------------------------------------------------------
// Procedure: onIdleState()

void BHV_ZigLeg::onIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onCompleteState()

void BHV_ZigLeg::onCompleteState()
{
}

//---------------------------------------------------------------
// Procedure: postConfigStatus()

void BHV_ZigLeg::postConfigStatus()
{
}

//---------------------------------------------------------------
// Procedure: onIdleToRunState()

void BHV_ZigLeg::onIdleToRunState()
{
}

//---------------------------------------------------------------
// Procedure: onRunToIdleState()

void BHV_ZigLeg::onRunToIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onRunState()

IvPFunction* BHV_ZigLeg::onRunState()
{
  bool ok1, ok2;
  double nav_heading = getBufferDoubleVal("NAV_HEADING", ok1);
  double wpt_index   = getBufferDoubleVal("WPT_INDEX",   ok2);

  if(!ok1) {
    postWMessage("No NAV_HEADING in info buffer");
    return(0);
  }

  double curr_time = getBufferCurrTime();

  // Detect a waypoint index change — skip the very first reading (launch)
  if(ok2 && (wpt_index != m_last_wpt_index)) {
    if(m_last_wpt_index >= 0) {    // only trigger on actual waypoint arrivals
      m_wpt_change_time = curr_time;
      m_zig_start_time  = -1;      // cancel any active zig
    }
    m_last_wpt_index = wpt_index;
  }

  // 5 seconds after waypoint change, start the zig
  if((m_wpt_change_time > 0) && (m_zig_start_time < 0)) {
    if((curr_time - m_wpt_change_time) >= m_zig_delay) {
      m_zig_start_time = curr_time;
      m_zig_heading    = nav_heading;  // capture heading at zig start
    }
  }

  // If a zig is active, build and return an IvP heading objective function
  if(m_zig_start_time > 0) {
    double elapsed = curr_time - m_zig_start_time;

    if(elapsed <= m_zig_duration) {
      // Desired heading offset from the captured heading at zig start
      double desired_hdg = m_zig_heading + m_zig_angle;
      // Normalize to [0, 360)
      while(desired_hdg < 0)   desired_hdg += 360;
      while(desired_hdg >= 360) desired_hdg -= 360;

      ZAIC_PEAK zaic(m_domain, "course");
      zaic.setSummit(desired_hdg);
      zaic.setPeakWidth(0);
      zaic.setBaseWidth(180);
      zaic.setSummitDelta(50);
      zaic.setMinMaxUtil(0, 100);

      IvPFunction* ipf = zaic.extractIvPFunction();
      if(ipf)
        ipf->setPWT(m_priority_wt);
      return(ipf);
    }
    else {
      // Zig complete — reset and wait for next waypoint
      m_zig_start_time  = -1;
      m_wpt_change_time = -1;
    }
  }

  return(0);
}
