# collectResources.ps1
#
# PowerShell script to collect policy files from NIST's backed-up GPOs to incorporate into
# Set_FDCC_LGPO.
#
# This script needs to be executed only one time to copy NIST's GPO files to the
# Set_FDCC_LGPO project's folder structure.  It does not need to be executed for each build
# of the executable, and the Visual Studio project does not run it.
#
# --------------------------------------------------------------------------------
#
# Instructions:
#
# * Download the FDCC GPOs from http://fdcc.nist.gov/download_fdcc.html
#
# * Expand the files in the zip file, keeping the folder structure intact.
#   (The full name of the root folder where the files are expanded should not
#   contain any of the following letter sequences:  "XP", "Vista", "Both",
#   "Machine", "User".  The script looks for those words in the full path to
#   determine what to do with the policy files.)
#
# * Change the value of the $sourceFolder variable below to ensure that it is set to
#   the full name of the root folder where the NIST GPO files were expanded.
#
# * In a PowerShell prompt, "cd" to the folder containing this script within the
#   Set_FDCC_LGPO project.
#
# * [Added for Q3 2008]:  Run "Q3-2008-FolderNameFixup.cmd".
# * [Fixup not needed for Q1 2009]
#
# * Make sure that the current folder contains a "res" subfolder, and that "PolicyFiles.rc"
#   is writable.
#
# * At the PowerShell prompt, run the following command line:
#       .\collectResources.ps1
#
# --------------------------------------------------------------------------------
#
# PowerShell notes:
#
#   PowerShell's default execution policy does not allow scripts to run.  To run this script,
#   you need to change the execution policy.  You can change the policy via the
#   Set-ExecutionPolicy cmdlet (which requires admin rights -- requires elevation on Vista)
#   or via Group Policy (the ADM is downloadable).  Execution policy options include:
#      * "Restricted" -- the default - no scripts run);
#      * "AllSigned" -- all scripts must be signed by a trusted publisher.  This script is not
#                signed, so with this execution policy you'll need to sign the script yourself.
#      * "RemoteSigned" -- scripts from remote systems must be signed; local scripts can execute.
#                See http://blogs.msdn.com/powershell/archive/2007/03/07/how-does-the-remotesigned-execution-policy-work.aspx
#                for more information about RemoteSigned.  You can remove the ZoneIdentifier tag from
#                a file through the file's Properties dialog in Explorer, clicking the "Unblock" button,
#                or with "streams -d filename" using streams.exe from Sysinternals.
#      * "Unrestricted" -- any script can run (not recommended).
#
#   Some PowerShell notational tips:
#      %{ ... }   means "for each object in the pipeline, run the stuff in the curly brackets"
#      ?{ ... }   means Where-Object:  "each object that matches the criteria in the curly brackets, output it to the next stage of the pipeline"
#      $_         refers to the current object in the pipeline (e.g., in a ForEach-Object or Where-Object statement)
#
# --------------------------------------------------------------------------------
#
# Assumptions:
#
#   * Future GPO downloads from NIST will continue the pattern of using "XP", "Vista" or "Both" in the
#     folder names to indicate target operating system.
#   [2008-06-26]:  Q3 2008 breaks that assumption.  There used to be top-level folders named "XP", "Vista" and "Both".  Now the
#   XP- and Vista-specific GPOs still have XP and Vista in the folder names, but the common ones no longer have "Both" in the name.
#   Quick fixes:
#       * Create "Q3-2008-FolderNameFixup.cmd" to create directory junctions to the common folders, where the Junction names contain " Both ".
#       * Previous assumption was that the full path to a target file would contain "\XP\", "\Both\", etc. (I.e., a folder just named XP, Vista, or Both.)
#         New assumption:  the name is contained within the folder, preceded and followed by a space character.
# --------------------------------------------------------------------------------


# sourceFolder:  root folder containing the policy files from NIST.  Change this if needed.
$sourceFolder = "C:\Downloads\NIST\FDCC v1.0 Q1 2009 Revised GPO's"

# Don't change these variables -- the rest of the Set_FDCC_LGPO Visual Studio project expects these values.
$targFolder = ".\res\"
$RCFilename = "PolicyFiles.rc"


# Define a function to find registry.pol files under the source folder, with folder names matching the OS (e.g.,
# "XP", "Vista" or "Both") and policy section ("User" or "Machine").  Copy them to the target folder,
# renaming them using the OS, section, and an incrementing integer.  (Keeps "registry.pol" at the end of the
# new file name.)
function GetRegistryPol($os, $section)
{
	$filename = "registry.pol"
	$ix = 0
	dir $sourceFolder -recurse -include $filename |
		?{ $_.FullName.Contains("\" + $os + "\") -and $_.FullName.Contains("\" + $section + "\") } |
		%{ Copy-Item $_ ($targFolder + $os + "." + $section + "." + $ix++ + "." + $filename) }
}

# Define a function to find security templates (GptTmpl.inf) under the source folder, with folder names matching
# the OS (e.g., "XP", "Vista", or "Both").  Copy them to the target folder, renaming them using
# the OS and an incrementing integer.  (Keeps "GptTmpl.inf" at the end of the new file name.)
function GetGptTmplInf($os)
{
	$filename = "GptTmpl.inf"
	$ix = 0
	dir $sourceFolder -recurse -include $filename |
		?{ $_.FullName.Contains("\" + $os + "\") } |
		%{ Copy-Item $_ ($targFolder + $os + "." + $ix++ + "." + $filename) }
}

# Script starts execution here:

# Remove previously-collected instances of registry.pol and GptTmpl.inf files from the target folder.
Remove-Item res\*.registry.pol
Remove-Item res\*.GptTmpl.inf

# Collect registry.pol files from the source folder for XP, Vista and Both, and for both User and
# Machine configuration.
GetRegistryPol "XP"    "User"
GetRegistryPol "XP"    "Machine"
GetRegistryPol "Vista" "User"
GetRegistryPol "Vista" "Machine"
GetRegistryPol "Both"  "User"
GetRegistryPol "Both"  "Machine"

# Collect security templates (GptTmpl.inf) for Xp, Vista, and Both.
GetGptTmplInf "XP"
GetGptTmplInf "Vista"
GetGptTmplInf "Both"


# Overwrite PolicyFiles.rc to incorporate these registry.pol and GptTmpl.inf files as embedded resources
# within the project's target executable:

$filename = "GptTmpl.inf"
dir ("res\*." + $filename) |
	%{ $_.Name.Substring(0, $_.Name.length - ($filename.Length + 1)) + "`t`t$filename`tres\\" + $_.Name } |
	Out-File $RCFilename -encoding ascii
$filename = "registry.pol"
dir ("res\*." + $filename) |
	%{ $_.Name.Substring(0, $_.Name.length - ($filename.Length + 1)) + "`t`t$filename`tres\\" + $_.Name } |
	Out-File $RCFilename -encoding ascii -append
