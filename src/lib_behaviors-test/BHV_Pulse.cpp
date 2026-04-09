/************************************************************/
/*    NAME: OSully                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_Pulse.cpp                                    */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include <cstdlib>
#include "MBUtils.h"
#include "BuildUtils.h"
#include "XYCircle.h"
#include "BHV_Pulse.h"

using namespace std;

//---------------------------------------------------------------
// Constructor

BHV_Pulse::BHV_Pulse(IvPDomain domain) :
  IvPBehavior(domain)
{
  // Provide a default behavior name
  IvPBehavior::setParam("name", "defaultname");

  // Declare the behavior decision space
  m_domain = subDomain(m_domain, "course,speed");

  // Configuration parameter defaults
  m_pulse_range    = 20;  // meters
  m_pulse_duration = 4;   // seconds

  // initial variable values
  m_last_wpt_index   = -1;  // no waypoint seen yet
  m_wpt_change_time  = -1;  // no waypoint transition yet
  m_pulse_start_time = -1;  // no active pulse

  // Add any variables this behavior needs to subscribe for
  addInfoVars("NAV_X, NAV_Y");
  addInfoVars("WPT_INDEX", "no_warning");
}

//---------------------------------------------------------------
// Procedure: setParam()

bool BHV_Pulse::setParam(string param, string val)
{
  // Convert the parameter to lower case for more general matching
  param = tolower(param);

  // Get the numerical value of the param argument for convenience once
  double double_val = atof(val.c_str());
  
  if((param == "pulse_range") && isNumber(val)) {
    m_pulse_range = double_val;
    return(true);
  }
  else if((param == "pulse_duration") && isNumber(val)) {
    m_pulse_duration = double_val;
    return(true);
  }

  // If not handled above, pass to base class (handles pwt, name, condition, etc.)
  return IvPBehavior::setParam(param, val);
}

//---------------------------------------------------------------
// Procedure: onSetParamComplete()

void BHV_Pulse::onSetParamComplete()
{
}

//---------------------------------------------------------------
// Procedure: onHelmStart()

void BHV_Pulse::onHelmStart()
{
}

//---------------------------------------------------------------
// Procedure: onIdleState()

void BHV_Pulse::onIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onCompleteState()

void BHV_Pulse::onCompleteState()
{
}

//---------------------------------------------------------------
// Procedure: postConfigStatus()

void BHV_Pulse::postConfigStatus()
{
}

//---------------------------------------------------------------
// Procedure: onIdleToRunState()

void BHV_Pulse::onIdleToRunState()
{
}

//---------------------------------------------------------------
// Procedure: onRunToIdleState()

void BHV_Pulse::onRunToIdleState()
{
}

//---------------------------------------------------------------
// Procedure: onRunState()

IvPFunction* BHV_Pulse::onRunState()
{
  // current position 
  bool ok1, ok2, ok3;
  double nav_x     = getBufferDoubleVal("NAV_X",     ok1);
  double nav_y     = getBufferDoubleVal("NAV_Y",     ok2);
  double wpt_index = getBufferDoubleVal("WPT_INDEX", ok3);
  if(!ok1 || !ok2) {
    postWMessage("No NAV_X or NAV_Y in info buffer");
    return(0);
  }

  double curr_time = getBufferCurrTime();

  // Detect a waypoint index change and record the time
  if(ok3 && (wpt_index != m_last_wpt_index)) {
    m_last_wpt_index  = wpt_index;
    m_wpt_change_time = curr_time;
    m_pulse_start_time = -1;  // undoes current pulse if actively done
  }

  // 5 seconds after waypoint change, begin the pulse
  if(m_wpt_change_time > 0 && m_pulse_start_time < 0) {
    if((curr_time - m_wpt_change_time) >= 20)     //*********CHANGE TIME DELAY HERE */
      m_pulse_start_time = curr_time;
  }

  // If a pulse is active, expand and post it
  if(m_pulse_start_time > 0) {
    double elapsed = curr_time - m_pulse_start_time;

    if(elapsed <= m_pulse_duration) {
      // Expand radius linearly over the pulse duration
      double radius = (elapsed / m_pulse_duration) * m_pulse_range;
      XYCircle circle(nav_x, nav_y, radius);
      circle.set_color("edge", "yellow");
      circle.set_color("fill", "yellow");
      circle.set_transparency(0.3);
      circle.set_label("pulse");
      postMessage("VIEW_CIRCLE", circle.get_spec());
    } else {
      // Pulse complete — erase and reset
      XYCircle circle(nav_x, nav_y, 0);
      circle.set_label("pulse");
      circle.set_active(false);
      postMessage("VIEW_CIRCLE", circle.get_spec());
      m_pulse_start_time = -1;
      m_wpt_change_time  = -1;  // wait for next waypoint
    }
  }

  return(0);
}

