
#include <mpkg/libmpkg.h>
#include <parted/parted.h>
#include <sys/mount.h>
#include <mpkg/colors.h>
#include <mpkg/menu.h>

#include "raidtool.h"
#include "parted_tools.h"
int createRaid(string md_dev, string level, int count, vector<string> partitions) {
	string cmd = "mdadm --create " + md_dev + " -l " + level + " -n " + IntToStr(count);
	for (unsigned int i=0; i<partitions.size(); ++i) {
		cmd += " " + partitions[i];
	}
	string log = get_tmp_file();
	ncInterface.uninit();
	int ret = system(cmd);
	if (ret == 1) {
		ncInterface.showMsgBox("Создание RAID-массива не удалось:\n" + ReadFile(log));
		return 1;
	}

	deviceCacheActual = false;
	system("dd if=/dev/zero of=" + md_dev + " bs=1024 count=1 > /dev/null 2>/dev/null"); // Clearing partition table
	system("echo 100000 >/proc/sys/dev/raid/speed_limit_min"); // Let's speed up resyncing process (experimental)
	system("mdadm --detail " + md_dev + " > " + log);
	
	ncInterface.showMsgBox("Массив создан: " + ReadFile(log));
	
	deviceCacheActual = false;
	// TODO: sync details, and so on - be more informative, don't dump raw stat to user
	return 0;
}

int assembleRaid(string md_dev, vector<string> partitions) {
	string cmd = "mdadm --assemble " + md_dev;
	for (unsigned int i=0; i<partitions.size(); ++i) {
		cmd += " " + partitions[i];
	}
	ncInterface.uninit();
	int ret = system(cmd);
	if (ret == 0) deviceCacheActual = false;
	return ret;
}

int stopRaidArray(string md_dev) {
	ncInterface.uninit();
	int ret = system("mdadm --stop " + md_dev);
	if (ret == 0) deviceCacheActual = false;
	return ret;
}
vector<RaidArray> detectRaidArrays() {
	vector<string> gp;
	vector<pEntry> pList;
	vector<string> raw;
	vector<RaidArray> ret;
	RaidArray *array = NULL;
	string tmp;
	pList= getGoodPartitions(gp);
	string mdout = get_tmp_file();
	for (unsigned int i=0; i<pList.size(); ++i) {
		if (!pList[i].raid) continue;
		system("mdadm -Q " + pList[i].devname + " | grep undetected | grep /dev/md > " + mdout + " 2>/dev/tty4");
		raw = ReadFileStrings(mdout);
		if (raw.empty()) continue;
		if (raw[0].find("/dev/md")==std::string::npos) continue;
		tmp = raw[0].substr(raw[0].find("/dev/md"));
		if (tmp.find(".")==std::string::npos) continue;
		tmp = tmp.substr(0, tmp.find_first_of("."));
		//ncInterface.showMsgBox("FOUND: " + tmp);
		array = NULL;
		for (unsigned int r=0; r<ret.size(); ++r) {
			if (ret[r].md_dev == tmp) {
				array = &ret[r];
				break;
			}
		}

		//ncInterface.showMsgBox("Total ret: " + IntToStr(ret.size()) + " but can't find " + tmp);
		if (!array) {
			ret.push_back(RaidArray());
			array = &ret[ret.size()-1];
			array->md_dev = tmp;
		}
		array->devices.push_back(pList[i].devname);
		//ncInterface.showMsgBox("Total ret: " + IntToStr(ret.size()) + " found li " + tmp);
	}
	return ret;
}
vector<RaidArray> getActiveRaidArrays() {
	vector<RaidArray> ret;
	string raw = get_tmp_file();
	system("cat /proc/mdstat | grep md | grep active > " + raw);
	vector<string> mdstat = ReadFileStrings(raw);
	vector<string> extra;
	RaidArray *array;
	string tmp;
	for (unsigned int i=0; i<mdstat.size(); ++i) {
		if (mdstat[i].find("md")==0) {
			ret.push_back(RaidArray());
			array = &ret[ret.size()-1];
			array->md_dev = "/dev/" + mdstat[i].substr(0, mdstat[i].find(" : "));
			tmp = mdstat[i].substr(mdstat[i].find(" active ") + strlen(" active "));
			array->level = tmp.substr(0, tmp.find(" "));
			array->extra_description = tmp.substr(tmp.find(" "));
			if (array->extra_description.empty()) continue;
			tmp = array->extra_description.substr(1);
			tmp = cutSpaces(tmp);
			while(!tmp.empty() && tmp.find_first_not_of("][ ")!=std::string::npos) {
				tmp = tmp.substr(tmp.find_first_not_of("][ "));
				if (tmp.find("[")==std::string::npos) break;
				array->devices.push_back("/dev/" + tmp.substr(0, tmp.find("[")));
				if (tmp.find(" ")==std::string::npos) break;
				else {
					tmp = tmp.substr(tmp.find(" "));
				}
			}

		}
	}
	for (unsigned int i=0; i<ret.size(); ++i) {
		ret[i].extra_description = "RAID-массив " + ret[i].level + " [";
		for (unsigned int d=0; d<ret[i].devices.size(); ++d) {
			if (ret[i].devices[d].length()>strlen("/dev/")) ret[i].extra_description += ret[i].devices[d].substr(strlen("/dev/"));
			else ret[i].extra_description += ret[i].devices[d];
			if (d != ret[i].devices.size()-1) ret[i].extra_description += "|";
		}
		ret[i].extra_description += "]";
	}
	return ret;
}

int raidMainMenu() {
	vector<MenuItem> menuItems;
	menuItems.push_back(MenuItem("ASSEMBLE", "Собрать уже существующий массив из доступных устройств"));
	menuItems.push_back(MenuItem("CREATE", "Создать новый массив"));
	menuItems.push_back(MenuItem("STOP", "Остановить существующий массив"));
	menuItems.push_back(MenuItem("EXIT", "Выход"));
	string ret = "CHECK";
	while (!ret.empty() && ret != "EXIT") {
		ret = ncInterface.showMenu2("Утилита управления RAID-массивами", menuItems);
		if (ret == "ASSEMBLE") {
			raidAssembleMenu();
			continue;
		}
		if (ret == "CREATE") {
			raidCreateMenu();
			continue;
		}
		if (ret == "STOP") {
			raidStopMenu();
			continue;
		}
		/*if (ret == "MANAGE") {
			raidManageMenu();
			continue;
		}*/
	}
	return 0;
}

int raidAssembleMenu() {
	vector<RaidArray> inactiveArrays = detectRaidArrays();
	vector<bool> activated(false, inactiveArrays.size());
	vector<MenuItem> menuItems;
	string tmp;
	for (unsigned int i=0; i<inactiveArrays.size(); ++i) {
		tmp.clear();
		for (unsigned int t=0; t<inactiveArrays[i].devices.size(); ++t) {
			tmp += inactiveArrays[i].devices[t];
			if (t<inactiveArrays[i].devices.size()-1) tmp += ", ";
		}
		menuItems.push_back(MenuItem(inactiveArrays[i].md_dev, tmp));
	}
	if (inactiveArrays.empty()) {
		ncInterface.showMsgBox("Неактивных RAID-разделов обнаружить не удалось");
		return 0;
	}
	menuItems.push_back(MenuItem("OK", "В главное меню"));
	int item = 0;
	while (item >=0 && item != (int) inactiveArrays.size()) {
		item = ncInterface.showMenu("Перед вами список обнаруженных неактивных массивов. Выберите тот, который вы хотите активировать:", menuItems);
		if (item >=0 && item < (int) inactiveArrays.size()) {
			if (activated[item]) {
				ncInterface.showMsgBox("Этот массив уже активирован");
				continue;
			}
			if (assembleRaid(inactiveArrays[item].md_dev, inactiveArrays[item].devices)==0) {
				activated[item] = true;
				menuItems[item].value += " (активен)";

				ncInterface.showMsgBox("Массив " + inactiveArrays[item].md_dev + " активирован");
			}
		}
	}
	return 0;
}
int raidStopMenu() {
	vector<RaidArray> activeArrays;
	vector<MenuItem> menuItems;
	string ret;
        while(true) {
		activeArrays = getActiveRaidArrays();
		menuItems.clear();
		for (unsigned int i=0; i<activeArrays.size(); ++i) {
			menuItems.push_back(MenuItem(activeArrays[i].md_dev, activeArrays[i].extra_description));
		}
		menuItems.push_back(MenuItem("OK", "Готово"));
		ret = ncInterface.showMenu2("Выберите массив для остановки", menuItems);
		if (ret.find("/dev/")==std::string::npos) return 0;
		if (stopRaidArray(ret)) {
			ncInterface.showMsgBox("RAID-массив остановить не удалось");
		}
	}
	return 0;
}

int raidCreateMenu() {
	if (!ncInterface.showYesNo("ВНИМАНИЕ: ПРОЧТИТЕ ВНИМАТЕЛЬНО!\nВы намереваетесь создать один или несколько RAID-массивов. Для этого вы должны сначала создать разделы типа fd (Linux RAID autodetect) на необходимых жестких дисках.\nХотя массив можно создать поверх любых типов разделов, и даже поверх самих устройств - делать этого не рекомендуется, и данная утилита такого не поддерживает.\n\nПродолжить?")) return 1;
	string mdstat = get_tmp_file();
	vector<RaidArray> activeArrays = getActiveRaidArrays();

	vector<pEntry> pList;
	vector<string> gp;
	pList = getGoodPartitions(gp);
	string md_dev = "/dev/md" + IntToStr(activeArrays.size());
	// extra check
	unsigned int shift = 0;
check_free_array:
	for (unsigned int i=0; i<activeArrays.size(); ++i) {
		if (md_dev == activeArrays[i].md_dev) {
			shift++;
			md_dev = "/dev/md" + IntToStr(activeArrays.size()+shift);
			goto check_free_array;
		}
	}

	vector<MenuItem> usedPartitions;
	for (unsigned int i=0; i<pList.size(); ++i) {
		if (pList[i].raid) usedPartitions.push_back(MenuItem(pList[i].devname, pList[i].size + "Mb"));
	}
	usedPartitions.push_back(MenuItem("Готово", "Создать массив"));
	ncInterface.showExMenu("Выберите разделы, которые вы хотите объединить в RAID\nВНИМАНИЕ: ВСЕ ДАННЫЕ ДА ЭТИХ РАЗДЕЛАХ БУДУТ УНИЧТОЖЕНЫ!", usedPartitions);

	vector<MenuItem> raidLevels;
	raidLevels.push_back(MenuItem("0", "RAID0 (Stripe)", "Дисковый массив без избыточного хранения данных. Информация разбивается на блоки, которые одновременно записываются на отдельные диски, что обеспечивает практически линейное повышение производительности.\nПлюсы: максимально возможная производительность, 100% использование места\nМинусы: поломка хотя бы одного диска ведет к потере всей информации"));
	raidLevels.push_back(MenuItem("1", "RAID1 (Mirror)", "Дисковый массив с дублированием информации (зеркалированием данных). В простейшем случае два накопителя содержат одинаковую информацию и являются одним логическим диском. При выходе из строя одного диска его функции выполняет другой. Для реализации массива требуется не меньше двух винчестеров." ));
	raidLevels.push_back(MenuItem("10", "RAID10 (Stripe+Mirror)", "Комбинация двух предыдущих: зеркало с повышенной производительностью"));
	raidLevels.push_back(MenuItem("5", "RAID5 (Distributed Parity)", "Самый распространенный уровень. Блоки данных и контрольные суммы циклически записываются на все диски массива, отсутствует выделенный диск для хранения информации о четности, нет асимметричности конфигурации дисков. Допустим выход из строя одного диска"));
	raidLevels.push_back(MenuItem("6", "RAID6 (Two Independent Distributed Parity Schemes)", "То же что и RAID5, только позволяет выход из строя сразу двух дисков. Производительность ниже чем у RAID5"));
	string raidLevel = ncInterface.showMenu2("Выберите тип RAID", raidLevels);

	vector<string> up;
	for (unsigned int i=0; i<usedPartitions.size()-1; ++i) {
		if (usedPartitions[i].flag) up.push_back(usedPartitions[i].tag);
	}
	deviceCacheActual=false;
	unsigned int raid_size = up.size();
	//if (raidLevel == "5") raid_size = up.size()-1;
	//if (raidLevel == "6") raid_size = up.size()-2;
	return createRaid(md_dev, raidLevel, raid_size, up);
}

int runRaidTool() {
	return raidMainMenu();
}
