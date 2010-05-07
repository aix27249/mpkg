/************************************************************
 * MPKG package management system
 * Message bus
 * $Id: bus.h,v 1.12 2007/05/17 15:12:36 i27249 Exp $
 * *********************************************************/

#ifndef BUS_H_
#define BUS_H_

#include "debug.h"
#include <vector>
using namespace std;
#define ITEMSTATE_WAIT 0
#define ITEMSTATE_INPROGRESS 1
#define ITEMSTATE_FINISHED 2
#define ITEMSTATE_FAILED 3
#define ITEMSTATE_ABORTED 4
#define IDLE_MAX 500
struct ItemState {
	string name, currentAction;
	int progress, totalProgress;
	int itemState;
};


class ProgressData
{
	public:
		//static void setProgress(unsigned int itemNum, double progress);
		//static void setProgressMaximum(unsigned int itemNum, double progress);
		void setItemName(int itemID, string name);
		void setItemCurrentAction(int itemID, string action);
		void setItemProgress(int itemID, double progress);
		void increaseItemProgress(int itemID);
		void setItemProgressMaximum(int itemID, double progress);
		void setItemState(int itemID, int state);
		void increaseIdleTime(int itemID);
		void resetIdleTime(int itemID);
		int getIdleTime(int itemID);
		bool itemUpdated(int itemID);
		void setItemChanged(int itemID);
		void setItemUnchanged(int itemID);
		void resetItems(string action="waiting", double __currentProgress=0, double __maxProgress=0, int state=ITEMSTATE_WAIT);

		int addItem(string iName, double maxProgress, int iState=ITEMSTATE_WAIT);


		string getItemName(int itemID);
		string getItemCurrentAction(int itemID);
		double getItemProgress(int itemID);
		double getItemProgressMaximum(int itemID);
		int getItemState(int itemID);
		int getCurrentItem();
		double getTotalProgress();
		double getTotalProgressMax();
		unsigned int size();
		void clear();

		string getCurrentAction();
		void setCurrentAction(string action);
		ProgressData();
		~ProgressData();

		bool downloadAction;
		void registerEventHandler(void (*newEventHandler) (ItemState));
		void unregisterEventHandler();


	private:

		void callEvent();
		vector<string> itemName;
		vector<string> itemCurrentAction;
		vector<double> itemProgress;
		vector<double> itemProgressMaximum;
		vector<int>itemState;
		vector<unsigned int >itemChanged;
		
		vector<int> idleTime;
		void (*eventHandler) (ItemState);
		bool sendEvents;
		int eventCounter;
		
		string currentAction;
		int lastChangedItem;
};

/*struct ItemState
{
	unsigned int progress;
	string name;
	unsigned short state;
	bool skipItem;
};

class ProcessState
{
	public:
		vector<ItemState> items;
		string name;
		unsigned short state;
};
*/
typedef enum {
	ACTIONID_NONE = 0,
	ACTIONID_DOWNLOAD,
	ACTIONID_INSTALL,
	ACTIONID_REMOVE,
	ACTIONID_PURGE,
	ACTIONID_CACHECHECK,
	ACTIONID_MD5CHECK,
	ACTIONID_DBUPDATE,
	ACTIONID_QUEUEBUILD,
	ACTIONID_GETPKGLIST,
	ACTIONID_DBLOADING,
	ACTIONID_DOWNLOAD_LINKING
} ActionID;

class ActionState
{
	public:
		double _currentProgress;
		double _progressMaximum;
		ActionID actionID;
		string actionName;
		unsigned int state;
		bool aborted;
		bool skip;
		bool skippable;
		bool hasProgressData;
		double currentProgress();
		double progressMaximum();

		ActionState();
		~ActionState();
};


class ActionBus
{
	public:
	vector<ActionState> actions;
	unsigned int getActionState(unsigned int pos);

	bool _abortActions;
	bool _abortComplete;
	ActionBus();
	~ActionBus();
	void setSkippable(ActionID actID, bool flag);
	bool skippable(ActionID actID);
	int getActionPosition(ActionID actID, bool addIfNone=true);
	unsigned int addAction(ActionID actionID, bool hasPData=true, bool skip=false);
	unsigned int pending();
	unsigned int completed();
	unsigned int size();
	bool idle();
	void clear();
	bool skipped(ActionID actID);
	ActionID currentProcessingID();
	int currentProcessing(); // returns vector number of processing action in actions array (if exists), otherwise return -1
	void skipAction(ActionID actID);
	void abortActions();
	bool abortComplete();
	void setCurrentAction(ActionID actID);
	void setActionProgress(ActionID actID, double p);
	void setActionProgressMaximum(ActionID actID, double p);
	void setActionState(ActionID actID, unsigned int state=ITEMSTATE_FINISHED);
	void setActionState(unsigned int pos, unsigned int state=ITEMSTATE_FINISHED);

	double progress();
	double progressMaximum();
	void registerEventHandler(void (*newEventHandler) (ItemState));
	void unregisterEventHandler();
	void (*eventHandler) (ItemState);
	void callEvent();
};

string getActionName(ActionID id);

extern ActionBus actionBus;
extern string currentStatus;
extern string currentItem;
extern ProgressData pData;
//extern double dlProgress;
//extern double dlProgressMax;


#endif // BUS_H_
