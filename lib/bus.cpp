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
ActionBus actionBus;

ProgressData::ProgressData()
{
	downloadAction=false;
	eventHandler = NULL;
	lastChangedItem=-1;
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
	if (eventHandler<=0) return;
	ItemState a;
	a.name=getItemName(lastChangedItem);
	a.currentAction=getItemCurrentAction(lastChangedItem);
	a.progress=10000.0f*(getItemProgress(lastChangedItem)/getItemProgressMaximum(lastChangedItem))/112;
	a.totalProgress=10000.0f*(getTotalProgress()/getTotalProgressMax())/112;
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
	callEvent();
	return itemName.size()-1;
}
void ProgressData::clear()
{
	itemName.resize(0);
	itemCurrentAction.resize(0);
	itemProgress.resize(0);
	itemProgressMaximum.resize(0);
	itemState.resize(0);
	currentAction.clear();
	itemChanged.resize(0);
	callEvent();
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
	setItemChanged(itemID);
}

void ProgressData::setItemCurrentAction(int itemID, string action)
{
	if (itemCurrentAction.size()>(unsigned int)itemID) itemCurrentAction.at(itemID)=action;
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
		callEvent();
	}
}

void ProgressData::setItemState(int itemID, int state)
{
	if (itemState.size()>(unsigned int)itemID) itemState.at(itemID)=state;
	setItemChanged(itemID);
}

void ProgressData::increaseIdleTime(int itemID)
{
	if (idleTime.size()>(unsigned int)itemID) idleTime.at(itemID)++;
}

void ProgressData::resetIdleTime(int itemID)
{
	if (idleTime.size()>(unsigned int) itemID) {
		idleTime.at(itemID)=0;
		callEvent();
	}
}

int ProgressData::getIdleTime(int itemID)
{
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
	for (unsigned int i=0; i<itemName.size(); i++)
	{
		setItemState(i, state);
		setItemCurrentAction(i, action);
		setItemProgressMaximum(i, __maxProgress);
		setItemProgress(i,__currentProgress);
		resetIdleTime(i);
	}
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

ActionBus::ActionBus()
{
	_abortActions=false;
	_abortComplete=false;
	eventHandler = NULL;
}

ActionBus::~ActionBus()
{
}

void ActionBus::registerEventHandler(void (*newEventHandler) (ItemState)) {
	eventHandler = newEventHandler;
	printf("ActionBus event handler registered: %p\n", eventHandler);
}
void ActionBus::unregisterEventHandler() {
	eventHandler=NULL;
}
void ActionBus::callEvent() {
	if (eventHandler<=0) return;
	ItemState a;
	int c = currentProcessing();
	if (c==-1) {
		a.name=getActionName(currentProcessingID());
		a.currentAction=_("Idle");
		a.progress=100;
		a.totalProgress=100;
	}
	else {
		a.name=getActionName(currentProcessingID());
		a.currentAction=IntToStr(actions[c].currentProgress())+"/"+IntToStr(actions[c].progressMaximum());
		a.progress=10000.0f*(actions[c].currentProgress()/actions[c].progressMaximum())/112;
		a.totalProgress=10000.0f*(progress()/progressMaximum())/112;
	}
	cout << _("actions total: ") << actions.size() << _(", current: ") << c << "\n" << endl;
	(*eventHandler) (a);

}

int ActionBus::getActionPosition(ActionID actID, bool addIfNone)
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (actions.at(i).actionID==actID) return i;
	}
	if (!addIfNone)	return -1;
	else
	{
		return addAction(actID);
	}

}

void ActionBus::setActionProgress(ActionID actID, double p)
{
	// Imho, this is bad!
	int t=getActionPosition(actID);
	if (t>=0) {
		actions.at(t)._currentProgress=p;
		callEvent();
	}
	/*
	for (unsigned int i=0; i<actions.size(); i++)
	{

	if (currentProcessing()>=0)
		actions.at(currentProcessing())._currentProgress=p;*/
}

void ActionBus::setActionProgressMaximum(ActionID actID, double p)
{
	int t=getActionPosition(actID);
	if (t>=0) {
		actions.at(t)._progressMaximum=p;
		callEvent();
	}
}
unsigned int ActionBus::addAction(ActionID actionID, bool hasPData, bool skip)
{
	int pos = getActionPosition(actionID, false);
	struct ActionState aState;
	aState.actionID=actionID;
	aState.state=ITEMSTATE_WAIT;
	aState.skip=skip;
	aState.skippable=false;
	aState.hasProgressData=hasPData;
	aState._currentProgress=0;
	aState._progressMaximum=0;
	if (pos < 0)
	{
		actions.push_back(aState);
		pos = actions.size()-1;
	}
	else actions.at(pos)=aState;
	callEvent();
	return pos;
}

void ActionBus::setSkippable(ActionID actID, bool flag)
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (actions.at(i).actionID==actID) actions.at(i).skippable=flag;
	}
}

bool ActionBus::skippable(ActionID actID)
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (actions.at(i).actionID==actID) return actions.at(i).skippable;
	}
	return false;
}

unsigned int ActionBus::completed()
{
	unsigned int ret=0;
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (this->getActionState(i)!=ITEMSTATE_WAIT && this->getActionState(i)!=ITEMSTATE_INPROGRESS) ret++;
	}
	return ret;
}

	
unsigned int ActionBus::pending()
{
	unsigned int ret=0;
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (getActionState(i)==ITEMSTATE_WAIT || getActionState(i)==ITEMSTATE_INPROGRESS) ret++;
	}
	return ret;
}
unsigned int ActionBus::size()
{
	return actions.size();
}

unsigned int ActionBus::getActionState(unsigned int pos)
{
	if (pos<actions.size())
	{
		return actions.at(pos).state;
	}
	else
	{
		//mError("Action position is out of range");
		return 100;
	}
}
bool ActionBus::idle()
{
	bool is_idle=true;
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (getActionState(i)==ITEMSTATE_INPROGRESS) is_idle=false;
	}
	return is_idle;
}

int ActionBus::currentProcessing()
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (getActionState(i)==ITEMSTATE_INPROGRESS) return i;
	}
	return -1;
}

ActionID ActionBus::currentProcessingID()
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (getActionState(i)==ITEMSTATE_INPROGRESS) return actions.at(i).actionID;
	}
	return ACTIONID_NONE;
}

string getActionName(ActionID id) {
	switch(id) {
		case ACTIONID_NONE: return _("None");
		case ACTIONID_DBLOADING: return _("Loading database");
		case ACTIONID_GETPKGLIST: return _("Retrieving package list");
		case ACTIONID_QUEUEBUILD: return _("Building queue");
		case ACTIONID_DBUPDATE: return _("Updating repository data");
		case ACTIONID_MD5CHECK: return _("Checking MD5");
		case ACTIONID_CACHECHECK: return _("Checking cache");
		case ACTIONID_PURGE: return _("Purging package");
		case ACTIONID_REMOVE: return _("Removing package");
		case ACTIONID_INSTALL: return _("Installing package");
		case ACTIONID_DOWNLOAD: return _("Downloading");
		default: return _("Unknown action type");
	}
}


void ActionBus::setCurrentAction(ActionID actID)
{
	bool found=false;
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (getActionState(i)==ITEMSTATE_INPROGRESS)
		{
			mError("Incorrect use of ActionBus detected: multiple processing, autofix by setting flag ITEMSTATE_FINISHED");
			setActionState(i, ITEMSTATE_FINISHED);
		}
	}
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (actions.at(i).actionID==actID)
		{
			setActionState(i,ITEMSTATE_INPROGRESS);
			found=true;
		}
	}
	if (!found)
	{
		actions.at(addAction(actID)).state=ITEMSTATE_INPROGRESS;
		mError("Seems to incorrect use of setCurrentAction: no such action found (added)");
	}
	callEvent();
}

void ActionBus::setActionState(ActionID actID, unsigned int state)
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (actions.at(i).actionID==actID){
		       setActionState(i,state);
		}
	}
	callEvent();
}

void ActionBus::setActionState(unsigned int pos, unsigned int state)
{
	if (pos<actions.size()) {
		actions.at(pos).state=state;
		callEvent();
	}
	
}



void ActionBus::skipAction(ActionID actID)
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (actions.at(i).actionID==actID) actions.at(i).skip=true;
	}
	callEvent();
}

void ActionBus::abortActions()
{
	_abortActions=true;
}

bool ActionBus::abortComplete()
{
	return _abortComplete;
}

void ActionBus::clear()
{
	actions.resize(0);
	_abortActions=false;
	_abortComplete=false;
	callEvent();
}

bool ActionBus::skipped(ActionID actID)
{
	for (unsigned int i=0; i<actions.size(); i++)
	{
		if (actions.at(i).actionID==actID) return actions.at(i).skip;
	}
	return false;
}

double ActionBus::progress()
{
	double ret=0;
	for (unsigned int i=0; i<actions.size(); i++)
	{
		ret = ret + actions.at(i).currentProgress();
	}
	
	return ret;
}

double ActionBus::progressMaximum()
{
	double ret = 0;
	for (unsigned int i=0; i<actions.size(); i++)
	{
		ret = ret + actions.at(i).progressMaximum();
	}
	return ret;
}

ActionState::ActionState()
{
	skippable=false;
}

ActionState::~ActionState()
{
}

double ActionState::currentProgress()
{
	return _currentProgress;
}

double ActionState::progressMaximum()
{
	return _progressMaximum;
}

