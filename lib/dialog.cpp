/****************************************************************
 * Basic C++ bingings to dialog utility
 * $Id: dialog.cpp,v 1.14 2007/11/12 21:35:45 i27249 Exp $
 *
 * Developed as part of MOPSLinux package system, but can be used
 * separately
 */
#include "dialog.h"

string Dialog::getReturnValue(string tmp_file)
{
	FILE *returnValues = fopen(tmp_file.c_str(), "r");
	if (!returnValues)
	{
		perror("cannot open temp file");
		abort();
	}
	char *membuff = (char *) malloc(2000);
	string ret;
	memset(membuff, 0, 2000);
	if (fscanf(returnValues, "%s", membuff)!=EOF)
	{
		ret = (string) membuff;
	}
	fclose(returnValues);
	free(membuff);
	mDebug("returned [" + ret + "]");
	return ret;
}

vector<string> Dialog::getReturnValues(string tmp_file, bool quoted)
{
	FILE *returnValues = fopen(tmp_file.c_str(), "r");
	if (!returnValues)
	{
		perror("cannot open temp file");
		abort();
	}
	char *membuff = (char *) malloc(2000);
	vector<string> ret;
	string tmp;
	memset(membuff, 0, 2000);
	while (fscanf(returnValues, "%s", membuff)!=EOF)
	{
		tmp = (string) membuff;
		if (quoted) tmp = tmp.substr(1, tmp.size()-2);
		ret.push_back(tmp);
	}

	free(membuff);
	fclose(returnValues);
	return ret;
}

/*bool Dialog::execYesNo(string header, unsigned int height, unsigned int width, string yes_label, string no_label)
{
	string tmp_file = get_tmp_file();
	string opt;
	if (!yes_label.empty()) opt+=" --yes-label \"" + yes_label + "\" ";
	if (!no_label.empty()) opt+=" --no-label \"" + no_label + "\" ";

	string exec_str = dialog + opt + " --yesno \"" + header + "\" " + IntToStr(height) + " " + IntToStr(width);
	int r = system(exec_str.c_str());
	unlink(tmp_file.c_str());
	if (r==0) return true;
	else return false;
}
void Dialog::execInfoBox(string text, unsigned int height, unsigned int width)
{
	string exec_str = dialog + " --infobox \"" + text + "\" " + IntToStr(height) + " " + IntToStr(width);
	system(exec_str.c_str());
}

void Dialog::execMsgBox(string text, unsigned int height, unsigned int width)
{
	string exec_str = dialog + " --msgbox \"" + text + "\" " + IntToStr(height) + " " + IntToStr(width);
	system(exec_str.c_str());
}*/
string Dialog::execMenu(string header, unsigned int height, unsigned int width, unsigned int menu_height, vector<TagPair> menuItems, string default_item)
{
	if (menuItems.empty())
	{
		mError("Empty item list");
		return "";
	}
	string tmp_file = get_tmp_file();
	string def_item;
	if (!default_item.empty()) 
	{
		for (unsigned int i=0; i<menuItems.size(); i++)
		{
			if (menuItems[i].tag==default_item)
			{
				def_item = "--default-item \"" + default_item + "\"";
				printf("set to %s\n", def_item.c_str());
			}
		}
		if (def_item.empty())
		{
			mError("No such item " + default_item + ", using defaults");
		}
	}
	string exec_str = dialog + " " + def_item + " --menu \"" + header + "\" " + IntToStr(height) + " " + IntToStr(width) + " " + IntToStr(menu_height);
	for (unsigned int i=0; i<menuItems.size(); i++)
	{
		exec_str += " \"" + menuItems[i].tag + "\" \"" + menuItems[i].value+"\"";
	}
	exec_str += " 2>"+tmp_file;
	system(exec_str.c_str());
	string ret = getReturnValue(tmp_file);
	unlink(tmp_file.c_str());
	return ret;
}

/*string Dialog::execInputBox(string header, string default_text, unsigned int height, unsigned int width)
{
	string tmp_file = get_tmp_file();
	string exec_str = dialog + " --inputbox \"" + header + "\" " + IntToStr(height) + " " + IntToStr(width) + " \"" + default_text + "\"" + " 2>"+tmp_file;
	system(exec_str.c_str());
	string ret = getReturnValue(tmp_file);
	unlink(tmp_file.c_str());
	return ret;
}*/

int Dialog::execMenu(string header, vector<string> menuItems)
{
	if (menuItems.empty())
	{
		mError("Empty item list");
		return -1;
	}
	vector<TagPair> mItems;
	for (unsigned int i=0; i<menuItems.size(); i++)
	{
		mItems.push_back(TagPair(IntToStr(i), menuItems[i]));
	}
	string ret = execMenu(header, 0,0,0, mItems);
	if (ret.empty()) return -1;
	return atoi(ret.c_str());
}

string Dialog::resolveComment(string tag)
{
	vector<TagPair> comments;

	comments.push_back(TagPair("ftp", "Онлайн-репозиторий (FTP)"));
	comments.push_back(TagPair("http", "Онлайн-репозиторий (HTTP)"));
	comments.push_back(TagPair("cdrom", "Пакеты на CD/DVD-диске"));
	comments.push_back(TagPair("file", "Пакеты на смонтированной файловой системе"));
	for (unsigned int i=0; i<comments.size(); i++)
	{
		if (comments[i].tag==tag)
			return comments[i].value;
	}
	return (string) "<Неизвестный тип репозитория либо некорректный URL>";
}

/*
void Dialog::execAddableList(string header, vector<string> *menuItems, string tagLimiter)
{
	// Legend:
	// tagLimiter, for exapmle, may be equal to "://" - this means that the value of "ftp://something.com" will be splitted in next way:
	// 	ftp will be a value (or assotiated comment using internal database), and something.com will be a tag.
	string tmp_file, exec_str, value;
	vector<TagPair> menuList;
	vector<string> tmpList;
	int ret;
	unsigned int pos;
	tmp_file = get_tmp_file();

begin:
	exec_str = dialog + " --ok-label \"Удалить\" --cancel-label \"Продолжить\" --extra-button --extra-label \"Добавить\" --menu \"" + header + "\" " + \
			   IntToStr(0) + " " + IntToStr(0) + " " + IntToStr(0);

	menuList.clear();
	if (!tagLimiter.empty())
	{
		for (unsigned int i=0; i<menuItems->size(); i++)
		{
			pos = menuItems->at(i).find(tagLimiter);
			if (pos!=std::string::npos)
			{
				menuList.push_back(TagPair(menuItems->at(i), resolveComment(menuItems->at(i).substr(0, pos))));
			}
			else
			{
				menuList.push_back(TagPair(menuItems->at(i), "Некорректный URL"));
			}
		}
	}
	for (unsigned int i=0; i<menuList.size(); i++)
	{
		exec_str += " \"" + menuList[i].tag + "\" \"" + menuList[i].value + "\" ";
	}
	exec_str += " 2>"+tmp_file;
	ret = system(exec_str.c_str());
	printf("returned %i\n", ret);
	switch(ret)
	{
		case 256: // OK button
			printf("Ok\n");
			return;
			break;
		case 768: // Add button
			value = execInputBox("Введите URL репозитория:", "");
			if (!value.empty())
			{
				menuItems->push_back(value);
			}
			goto begin;
			break;
		case 0:	// Delete button
			mError("delete button");
			value = getReturnValue(tmp_file);
			if (!value.empty())
			{
				if (menuList.size()==1)
				{
					//execMsgBox("Список не может быть пустым. Сначала добавьте еще что-нибудь");
					goto begin;
				}
				for (unsigned int i=0; i<menuList.size(); i++)
				{
					if (menuList[i].tag == value)
					{
						tmpList.clear();
						for (unsigned int t=0; t<menuItems->size(); t++)
						{
							if (menuItems->at(t)!=value)
							{
								tmpList.push_back(menuItems->at(t));
							}
						}
						*menuItems = tmpList;
						tmpList.clear();
						mDebug("Deleted " + value);
						goto begin;
					}
				}
				mError("out of cycle");
				goto begin;
			}
			else
			{
				mDebug("empty value");
			}
			goto begin;
			break;
		default: // Cancel, ESC, and other errors
			mError(exec_str);
			mDebug("Returned " +  IntToStr(ret));
			
			return;		
			if (execYesNo("Действительно прервать?")) abort();
			else goto begin;
	}
}*/

/*void Dialog::execGauge(string text, unsigned int height, unsigned int width, int value)
{
	mDebug("Executing gauge");
	string exec_str = dialog + " --gauge \"" + text + "\" " + IntToStr(height) + " " + IntToStr(width);
	if (fd) {
		pclose(fd);
		fd=NULL;
	}
	fd=popen(exec_str.c_str(), "w");
	setGaugeValue(value);
	
}
void Dialog::setGaugeValue(int value)
{
	if (fd)
	{
		fprintf(fd, "%d\n", value);
		fflush(fd);
	}
	else mError("Gauge doesn't exist");

}
void Dialog::closeGauge()
{
	mDebug("Closing gauge");
	if (fd)
	{
		pclose(fd);
		fd=NULL;
	}
	else mError("Gauge doesn't exist");
}*/
bool Dialog::execCheckList(string header, unsigned int height, unsigned int width, unsigned int menu_height, vector<TagPair>* menuItems)
{
	if (menuItems->empty())
	{
		mError("Empty item list");
		return false;
	}
	string tmp_file = get_tmp_file();
	string exec_str = dialog + " --checklist \"" + header + "\" " + IntToStr(height) + " " + IntToStr(width) + " " + IntToStr(menu_height);
	for (unsigned int i=0; i<menuItems->size(); i++)
	{
		exec_str += " \"" + menuItems->at(i).tag + "\" \"" + menuItems->at(i).value+"\" ";
		if (menuItems->at(i).checkState) exec_str += "on";
		else exec_str += "off";
	}
	exec_str += " 2>"+tmp_file;
	int d_ret = system(exec_str.c_str());
	vector<string> ret = getReturnValues(tmp_file);
	unlink(tmp_file.c_str());
	if (d_ret!=0)
		return false;
	for (unsigned int i=0; i<menuItems->size(); i++)
	{
		menuItems->at(i).checkState=false;
	}
	for (unsigned int i=0; i<ret.size(); i++)
	{
		for (unsigned int t=0; t<menuItems->size(); t++)
		{
			if (ret[i]==menuItems->at(t).tag)
			{
				menuItems->at(t).checkState=true;
			}
		}
	}
	return true;
}

Dialog::Dialog(string title, string backtitle)
{
	fd=NULL;
	setTitle(title,backtitle);
/*	dialog = "dialog ";
	if (!backtitle.empty()) dialog += " --backtitle \"" + backtitle + "\" ";
	if (!title.empty()) dialog += " --title \"" + title + "\" ";*/
}
void Dialog::setTitle(string title, string backtitle)
{
	dialog = "dialog ";
	if (!backtitle.empty()) dialog += " --backtitle \"" + backtitle + "\" ";
	if (!title.empty()) dialog += " --title \"" + title + "\" ";
}

Dialog::~Dialog()
{
	if (fd)
	{
		pclose(fd);
		fd=NULL;
	}

}
