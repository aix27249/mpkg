/* New AgiliaLinux setup: cache update tool. Works not very well, but should work.
 *
*/

#include <nwidgets/ncurses_if.h>
#include <mpkgsupport/mpkgsupport.h>
#include "default_paths.h"
#include "helper_functions.h"
#include <mpkg-parted/mpkg-parted.h>

bool cacheIndex(string pkgsource, string repoURL) {
	if (pkgsource=="cdrom://") {
		
	}
	return true;
}

int mountMedia(string iso_image, CursesInterface &ncInterface)
{
	system("umount -l /var/log/mount  2>/dev/tty4 >/dev/tty4"); 
	
	ncInterface.setSubtitle(_("Searching for CD/DVD drive"));
	if (iso_image.empty()) ncInterface.showInfoBox(_("Performing CD/DVD drive autodetection..."));
	vector<string> devList = getCdromList();

	// Trying to mount
	if (!iso_image.empty()) {
		return 0;
	}
	string cmd, cdromDevice;
	for (size_t i=0; i<devList.size(); i++)
	{
		cmd = "mount -t iso9660 " + devList[i]+" /var/log/mount 2>/dev/tty4 >/dev/tty4";
		if (system(cmd.c_str())==0)
		{
			ncInterface.showInfoBox(_("CD/DVD drive found at ") + devList[i]);
			system("rm -f /dev/cdrom 2>/dev/tty4 >/dev/tty4; ln -s " + devList[i] + " /dev/cdrom 2>/dev/tty4 >/dev/tty4");
			WriteFile(SETUPCONFIG_CDROM, devList[i]);
			cdromDevice=devList[i];
			system("echo Found DVD-ROM drive: " + cdromDevice + " >/dev/tty4");
			return 0;
		}
	}
	// Failed? Select manually please
	if (!ncInterface.showYesNo(_("CD/DVD drive autodetection failed. Specify device name manually?"))) {
		return 1;
	}
	
	string manualMount = ncInterface.showInputBox(_("Please, enter device name (for example, /dev/scd0):"), "/dev/");
	if (manualMount.empty())
	{
		return 1;
	}
	cmd = "mount -t iso9660 " + manualMount + " /var/log/mount 2>/var/log/mpkg-lasterror.log >/dev/tty4";
	if (system(cmd)!=0)
	{
		ncInterface.showMsgBox(_("The drive you have specified has failed to mount\n"));
		return 1;
	}

	return 0;
}
string getCDSource(string predefined, CursesInterface &ncInterface)
{
	string rList;
	if (predefined =="dvd") ncInterface.setSubtitle(_("Installation from DVD"));
	else if (predefined.find("iso")==0) ncInterface.setSubtitle(_("Installation from ISO image"));
	else ncInterface.setSubtitle("Installation from DVD");
	string iso_file;
	if (predefined.find("iso:///")==0) {
		iso_file = predefined.substr(6);
		if (iso_file.empty()) return rList;
		ncInterface.setSubtitle(_("Installing from CD/DVD"));
		mountMedia(iso_file, ncInterface);
	}
	else mountMedia("", ncInterface);
	string cdromDevice = ReadFile(SETUPCONFIG_CDROM);
	system("umount " + cdromDevice + " 2>/dev/tty4 >/dev/tty4");

	string last_indexed_cd=_("<none>");
	int disc_number=0;
	string volname;
	vector<string> rep_locations;
	string rep_location;
	system(" mkdir " + CDROM_MOUNTPOINT);
		ncInterface.showInfoBox(_("Mounting CD/DVD..."));
		if (predefined=="dvd") system("mount " + cdromDevice + " /var/log/mount 2>/dev/tty4 >/dev/tty4");
		else {
			system("mount -o loop " + iso_file + " /var/log/mount 2>/dev/tty4 >/dev/tty4");
		}

		volname = getCdromVolname(&rep_location);
		if (!volname.empty() && !rep_location.empty())
		{
			ncInterface.showInfoBox(_("Loading data from disc ") + volname + _("\nData path: ") + rep_location);
			if (volname!=last_indexed_cd)
			{
				if (cacheCdromIndex(volname, rep_location))
				{
				
					rList = "cdrom://"+volname+"/"+rep_location;
					rep_locations.push_back(rep_location);

					last_indexed_cd=volname;
					disc_number++;
					ncInterface.showInfoBox(_("Data loaded successfully.\nDisc label: ") + volname + _("\nPackage path: ") + rep_location);
				}
				else ncInterface.showMsgBox(_("Failed to read repository index from this disc"));
			}
			else ncInterface.showMsgBox(_("This disc already indexed"));
		}
		else ncInterface.showMsgBox(_("This disc doesn't recognized as AgiliaLinux installation disc"));
		if (predefined=="dvd") system("umount " + cdromDevice + " 2>/dev/tty4 >/dev/tty4");
		else system("umount " + iso_file + " 2>/dev/tty4 >/dev/tty4");
	
	// Here was an index retriveal, but I disabled it

	return rList;
}

string getCdromRepo(CursesInterface &ncInterface) {
	// Now we need to access DVD drive.
	return getCDSource("dvd", ncInterface);
}

string getISORepo(string iso_path, CursesInterface &ncInterface) {
	return getCDSource(iso_path, ncInterface);
	return "";
}
int main(int argc, char **argv) {
	// First step: read package sources, and convert some of them. Then, store it to SETUPCONFIG_REPOSITORYLIST
	vector<string> pkgsources = ReadFileStrings(SETUPCONFIG_PKGSOURCE);
	vector<string> repositoryList;
	string repo;
	dialogMode = true;
	CursesInterface ncInterface;
	for (size_t i=0; i<pkgsources.size(); ++i) {
		if (pkgsources[i]=="cdrom://") {
			repo = getCdromRepo(ncInterface);
			if (!repo.empty()) repositoryList.push_back(repo);
			else repositoryList.push_back("");
		}
		else if (pkgsources[i].find("iso://")==0) {
			repo = getISORepo(pkgsources[i], ncInterface);
			if (!repo.empty()) repositoryList.push_back(repo);
			else repositoryList.push_back("");
		}
		else repositoryList.push_back(pkgsources[i]);	
	}
	WriteFileStrings(SETUPCONFIG_REPOSITORYLIST, repositoryList);
	
	// Second step: connect to all package sources and download packages.xml.gz
	for (size_t i=0; i<pkgsources.size() && i<repositoryList.size(); ++i) {
		ncInterface.showInfoBox(_("Caching index from ") + repositoryList[i]);
		cacheIndex(pkgsources[i], repositoryList[i]);
	}
	
	return 0;
}


