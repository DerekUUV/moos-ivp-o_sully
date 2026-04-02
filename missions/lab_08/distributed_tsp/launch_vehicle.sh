#!/bin/bash
#----------------------------------------------------------
#  Script: launch_vehicle.sh
#  LastEd: Apr 11th 2021
#----------------------------------------------------------
#  Part 1: Set global var defaults
#----------------------------------------------------------
TIME_WARP=1
JUST_MAKE="no"
SIM="no"

VNAME="gilda"
START_POS="0,0"
LOITER_POS="x=0,y=-75"
IP_ADDR=""
MOOS_PORT="9001"
PSHARE_PORT="9202"
SHORE_IP="10.29.188.206"
SHORE_PSHARE="9200"

#-----------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#-----------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "launch_vehicle.sh [SWITCHES] [time_warp] "
	echo "  --vname=<name>         Vehicle name"
	echo "  --ip=<addr>            Vehicle IP address"
	echo "  --shore=<addr>         Shoreside IP address"
	echo "  --mport=<port>         MOOS port"
	echo "  --pshare=<port>        pShare port"
	echo "  --shore_pshare=<port>  Shoreside pShare port"
	echo "  --start=<x,y>          Start position"
	echo "  --sim                  Simulation mode (vehicle dynamics only, no hardware)"
	echo "  --just_build, -j       Just build files, do not launch"
	echo "  --help, -h             Show this help"
	exit 0;
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_build" -o "${ARGI}" = "-j" ]; then
	JUST_MAKE="yes"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR="${ARGI#--ip=*}"
    elif [ "${ARGI:0:7}" = "--mport" ]; then
	    MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:8}" = "--shore=" ]; then
        SHORE_IP="${ARGI#--shore=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:8}" = "--vname=" ]; then
        VNAME="${ARGI#--vname=*}"
    elif [ "${ARGI:0:8}" = "--start=" ]; then
        START_POS="${ARGI#--start=*}"
    elif [ "${ARGI}" = "--sim" ]; then
        SIM="yes"
    else
	echo "launch_vehicle.sh Bad Arg:" $ARGI " Exit code 1."
	exit 0
    fi
done

if [ -z "${IP_ADDR}" ]; then
    IP_ADDR="localhost"
fi

#-----------------------------------------------------------
#  Part 3: Create the .moos and .bhv files.
#-----------------------------------------------------------
nsplug meta_vehicle.moos targ_$VNAME.moos -f WARP=$TIME_WARP  \
       VNAME=$VNAME          START_POS=$START_POS              \
       VPORT=$MOOS_PORT      PSHARE_PORT=$PSHARE_PORT          \
       VTYPE="kayak"         SHORE_IP=$SHORE_IP                \
       SHORE_PSHARE=$SHORE_PSHARE  IP_ADDR=$IP_ADDR

nsplug meta_vehicle.bhv targ_$VNAME.bhv -f VNAME=$VNAME       \
       START_POS=$START_POS LOITER_POS=$LOITER_POS


if [ ${JUST_MAKE} = "yes" ]; then
    echo "Files assembled; nothing launched; exiting per request."
    exit 0
fi

#-----------------------------------------------------------
#  Part 4: Launch the processes
#-----------------------------------------------------------
echo "Launching $VNAME MOOS Community. WARP is" $TIME_WARP
pAntler targ_$VNAME.moos >& /dev/null &
echo "Done"

#-----------------------------------------------------------
#  Part 5: Launch uMAC and kill everything upon exiting uMAC
#-----------------------------------------------------------
uMAC targ_$VNAME.moos

kill -- -$$