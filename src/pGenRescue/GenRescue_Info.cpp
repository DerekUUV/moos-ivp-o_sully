/****************************************************************/
/*   NAME: DerekOSullivan                                       */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: GenRescue_Info.cpp                                     */
/*   DATE: 13mar26                                              */
/****************************************************************/

#include <cstdlib>
#include <iostream>
#include "GenRescue_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  pGenRescue receives SWIMMER_ALERT messages from the shoreside, ");
  blk("  builds a greedy nearest-neighbor path to unrescued swimmers, ");
  blk("  and publishes SURVEY_UPDATE waypoints for pHelmIvP.          ");
  blk("  Swimmers are marked rescued on FOUND_SWIMMER or proximity.   ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: pGenRescue file.moos [OPTIONS]                             ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch pGenRescue with the given process name               ");
  blk("      rather than pGenRescue.                                     ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of pGenRescue.                  ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showExampleConfigAndExit

void showExampleConfigAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("pGenRescue Example MOOS Configuration                             ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = pGenRescue                                        ");
  blk("{                                                               ");
  blk("  AppTick   = 4                                                 ");
  blk("  CommsTick = 4                                                 ");
  blk("}                                                               ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showInterfaceAndExit

void showInterfaceAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("pGenRescue INTERFACE                                              ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  SWIMMER_ALERT = x=<val>, y=<val>, id=<val>                   ");
  blk("  FOUND_SWIMMER = id=<val>, finder=<vname>                      ");
  blk("  NAV_X         = <double>                                      ");
  blk("  NAV_Y         = <double>                                      ");
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  SURVEY_UPDATE = points=<seglist_spec>                         ");
  blk("  RETURN        = true  (when all swimmers rescued)             ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("pGenRescue", "gpl");
  exit(0);
}
