@echo off
REM		The folder structure for the NIST Q3 2008 FDCC GPOs has changed from earlier versions.
REM		There used to be top-level folders named "Both", "XP" and "Vista".  The "collectResources.ps1"
REM		PowerShell script used to help set up the Set_FDCC_LGPO build environment depended on this naming
REM		convention to pick up the Registry.pol and GptTmpl.inf files and rename them appropriately.
REM		The new folder structure includes folders that contain " XP " and " Vista " in their names for the
REM		XP- and Vista-specific policies, but the common folders no longer contain "Both" in their names.
REM		This batch file creates a quick workaround by creating directory junctions that contain " Both ".

REM		This script requires Windows Vista (depends on the built-in MKLINK command).

set _BASE_DIR=C:\Downloads\NIST\FDCC_Q3_2008_Final_GPOs\

set _SUBDIR_1=FDCC v1.0 Q3 2008 Account Policy
set _SUBDIR_2=FDCC v1.0 Q3 2008 Additional Settings
set _SUBDIR_3=FDCC v1.0 Q3 2008 IE7 Settings

pushd "%_BASE_DIR%"

mklink /J "FDCC v1.0 Q3 2008 _ Both 1" "%_SUBDIR_1%"
mklink /J "FDCC v1.0 Q3 2008 _ Both 2" "%_SUBDIR_2%"
mklink /J "FDCC v1.0 Q3 2008 _ Both 3" "%_SUBDIR_3%"

popd
