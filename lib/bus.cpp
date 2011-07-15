/************************************************************
 * MPKG package management system
 * Message bus implementation
 * $Id: bus.cpp,v 1.15 2007/05/23 14:16:03 i27249 Exp $
 * *********************************************************/
#include "bus.h"
#include <iostream>
string currentStatus;
string currentItem;
ProgressData pData;
bool _abortActions = false;
//ActionBus actionBus;

ProgressData::ProgressData()
{
	downloadAction=false;
	eventHandler = NULL;
	lastChangedItem=-1;
	sendEvents = true;
}

ProgressData::~ProgressData(){}
void ProgressData::registerEventHandler(void (*newEventHandler) (ItemState)) {
	eventHandler = newEventHandler;
	printf("ProgressData event handler registered: %p\n", eventHandler);
}
void ProgressData::unregisterEventHandler() {
	eventHandler=NULL;
}
void ProgressData::callEvent() {
	if (eventHandler<=0 || !sendEvents) return;
	ItemState a;
	a.name=getItemName(lastChangedItem);
	a.currentAction=getItemCurrentAction(lastChangedItem);
	a.progress=10000.0f*(getItemProgress(lastChangedItem)/getItemProgressMaximum(lastChangedItem))/100;
	a.totalProgress=10000.0f*(getTotalProgress()/getTotalProgressMax())/100;
	a.itemOrder = getItemOrder(lastChangedItem);
	int64_t totalMem = 0;
	for (size_t i=0; i<itemName.size(); ++i) {
		totalMem += itemName[i].size();
	}
	for (size_t i=0; i<itemCurrentAction.size(); ++i) {
		totalMem += itemCurrentAction[i].size();
	}
	
	// This stuff is useful for debug: counts and shows events
	eventCounter++;
	//fprintf(stderr, "[%d] callEvent: %s, queue size: %Ld, total mem: %Ld (%s)\n", eventCounter, a.currentAction.c_str(), itemName.size(), totalMem, humanizeSize(totalMem).c_str());
	//fprintf(stderr, "[%d] callEvent: %s/%s, progress: %d, totalProgress: %d, max: %f\n", eventCounter, a.name.c_str(), a.currentAction.c_str(), a.progress, a.totalProgress, getTotalProgressMax());
	(*eventHandler) (a);
}
int ProgressData::addItem(string iName, double maxProgress, int iState)
{
	itemName.push_back(iName);
	itemCurrentAction.push_back("waiting");
	itemChanged.push_back(true);
	itemProgress.push_back(0);
	idleTime.push_back(0);
	itemProgressMaximum.push_back(maxProgress);
	itemState.push_back(iState);
	itemOrder.push_back(ITEMORDER_MIDDLE);
	//callEvent();
	return itemName.size()-1;
}
void ProgressData::clear()
{
	sendEvents = false;
	itemName.resize(0);
	itemCurrentAction.resize(0);
	itemOrder.resize(0);
	itemProgress.resize(0);
	itemProgressMaximum.resize(0);
	itemState.resize(0);
	currentAction.clear();
	itemChanged.resize(0);
	sendEvents = true;
	//callEvent();
}
unsigned int ProgressData::size()
{
	return itemName.size();
}
double ProgressData::getTotalProgressMax()
{
	double ret=0;
	for (unsigned int i=0; i<itemProgressMaximum.size(); i++)
	{
		ret+=itemProgressMaximum.at(i);
	}
	return ret;
}

double ProgressData::getTotalProgress()
{
	double ret=0;
	for (unsigned int i=0; i<itemProgress.size(); i++)
	{
		ret+=itemProgress.at(i);
	}
	return ret;
}

void ProgressData::setItemChanged(int itemID)
{
	if (itemChanged.size()> (unsigned int) itemID) itemChanged.at(itemID)=0;
	resetIdleTime(itemID);
	lastChangedItem=itemID;
	callEvent();
}

void ProgressData::setItemUnchanged(int itemID)
{
	if (itemChanged.size()>(unsigned int) itemID && itemChanged.at(itemID)<500) itemChanged.at(itemID)++;
}

void ProgressData::setItemName(int itemID, string name)
{
	if (itemName.size()>(unsigned int) itemID) itemName.at(itemID)=name;
	//setItemChanged(itemID);
}

void ProgressData::setItemCurrentAction(int itemID, string action, ItemOrder itemOrder_mark)
{
	if (itemCurrentAction.size()>(size_t)itemID) itemCurrentAction.at(itemID)=action;
	if (itemOrder.size()>(size_t)itemID) itemOrder.at(itemID)=itemOrder_mark;
	setItemChanged(itemID);
}

void ProgressData::setItemProgress(int itemID, double progress)
{
	if (itemProgress.size()>(unsigned int) itemID) itemProgress.at(itemID)=progress;
	setItemChanged(itemID);
}

void ProgressData::setItemProgressMaximum(int itemID, double progress)
{
	if (itemProgressMaximum.size()>(unsigned int) itemID) {
		itemProgressMaximum.at(itemID)=progress;
		//callEvent();
	}
}

void ProgressData::setItemState(int itemID, int state)
{
	if (itemState.size()>(unsigned int)itemID) itemState.at(itemID)=state;
	if (state==ITEMSTATE_FINISHED || state == ITEMSTATE_FAILED) setItemProgress(itemID, getItemProgressMaximum(itemID));
	//setItemChanged(itemID);
}

void ProgressData::increaseIdleTime(int itemID)
{
	if (idleTime.size()>(unsigned int)itemID) idleTime.at(itemID)++;
}

void ProgressData::resetIdleTime(int itemID)
{
	if (idleTime.size()>(unsigned int) itemID) {
		idleTime.at(itemID)=0;
		//callEvent();
	}
}

int ProgressData::getIdleTime(int itemID)
{
	// seems that this stuff doesn't used in any sort of actions, reset them
	if (idleTime.size()>(unsigned int) itemID) return idleTime.at(itemID);
	else return 0;
}
bool ProgressData::itemUpdated(int itemID)
{
	if (itemChanged.size()>(unsigned int) itemID && itemChanged.at(itemID)<499) return true;
	else return false;
}
void ProgressData::resetItems(string action, double __currentProgress, double __maxProgress, int state)
{
	sendEvents = false;
	for (unsigned int i=0; i<itemName.size(); i++)
	{
		setItemState(i, state);
		setItemCurrentAction(i, action);
		setItemProgressMaximum(i, __maxProgress);
		setItemProgress(i,__currentProgress);
	}
	sendEvents = true;
}
void ProgressData::increaseItemProgress(int itemID)
{
	if (itemProgress.size()>(unsigned int) itemID)
	{
		itemProgress.at(itemID) = itemProgress.at(itemID) + 1;
		callEvent();
	}
	else mError("Item ID is out of range");
}

string ProgressData::getCurrentAction()
{
	return currentAction;
}
void ProgressData::setCurrentAction(string action)
{
	currentAction=action;
}

string ProgressData::getItemName(int itemID)
{
	if (itemName.size()>(unsigned int) itemID) return itemName.at(itemID);
	else return "noSuchItem";
}

string ProgressData::getItemCurrentAction(int itemID)
{
	if (itemCurrentAction.size()>(unsigned int)itemID) return itemCurrentAction.at(itemID);
	else return "noSuchItem";
}

ItemOrder ProgressData::getItemOrder(int itemID) {
	if (itemOrder.size()>(size_t)itemID) return itemOrder.at(itemID);
	else return ITEMORDER_MIDDLE;
}

double ProgressData::getItemProgress(int itemID)
{
	if (itemProgress.size()>(unsigned int)itemID) return itemProgress.at(itemID);
	else return 0;
}
double ProgressData::getItemProgressMaximum(int itemID)
{
	if (itemProgressMaximum.size()>(unsigned int)itemID) return itemProgressMaximum.at(itemID);
	else return 0;
}
int ProgressData::getItemState(int itemID)
{
	if (itemState.size()>(unsigned int) itemID) return itemState.at(itemID);
	else return ITEMSTATE_WAIT;
}

int ProgressData::getCurrentItem() {
	if (lastChangedItem<(int) size()) return lastChangedItem;
	else return -1;
}


