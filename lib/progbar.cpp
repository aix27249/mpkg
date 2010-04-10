#include <ncursesw/ncurses.h>
#include "debug.h"
#include "progbar.h"
#define DEF_NAME (int)766456
#define DEF_TYTLE
//#define DEBUGX
using namespace std;
static void fillwin(WINDOW *win, string ch)
{
	int y, x;
	int y1, x1;
	getmaxyx(win, y1, x1);
	for (y = 0; y < y1; y++) {
		wmove(win, y, 0);
		for (x = 0; x < x1; x++)
			waddstr(win, ch.c_str());
	}
}
progressbar::progressbar()
{
	//printf("1\n");
	mDebug("Constructor 1");
	//printf("1.1\n");
	initialized=false;
	progressMax=100; //всего елментов
	currentProgress=0;  //прошло елементов (напрмер кол-во пакетов в установке)
	frontbar=""; // tyt_progress; 	    //текст перед прогресс баром
	fronttext="";            //текст перед именем пакета 	
	title="";	    //текст в левом верхнем углу экрана
	subtitle="";	    //текст в центральном окне посередине
	_list_bar=false; //флаг загрузки списка имен      
	_initcurs=false; //флаг загрузки монитора
	count_position=0;
	progressBarLength=0;
	progressBarPosition=0;
	_alreadyshow=false;
}
progressbar::progressbar(progressbar& pf)
{
	mDebug("Constructor 2");
	initialized=false;

	progressMax=pf.progressMax; //всего елментов
	currentProgress=pf.currentProgress;  //прошло елементов (напрмер кол-во пакетов в установке)
	frontbar=pf.frontbar;     //текст перед прогресс баром
	fronttext=pf.fronttext;      //текст перед именем пакета 	
	title=pf.title;	    //текст в левом верхнем углу экрана
	subtitle=pf.subtitle;	    //текст в центральном окне посередине
	_list_bar=pf._list_bar; //флаг загрузки списка имен  
    	_initcurs=pf._initcurs;
	count_position=pf.count_position;
	progressBarLength=0;
	progressBarPosition=0; // копируем тока теги
	_alreadyshow=pf._alreadyshow;
}

progressbar::progressbar(char* Title)
{
	mDebug("Constructor 3");
	initialized=false;

	progressMax=100; //всего елментов
	currentProgress=0;  //прошло елементов (напрмер кол-во пакетов в установке)
	frontbar=""; // tyt_progress; 	    //текст перед прогресс баром
	fronttext="";            //текст перед именем пакета 	
	set_title(Title);	    //текст в левом верхнем углу экрана
	subtitle="";	    //текст в центральном окне посередине
	_list_bar=false; //флаг загрузки списка имен      
	_initcurs=false; //флаг загрузки монитора
	count_position=0;
	progressBarLength=0;
	progressBarPosition=0;
	_alreadyshow=false;
}
progressbar::progressbar(string Title,string SubTitle,string FrontText,string FrontBar)
{
	mDebug("Constructor 4");
	initialized=false;

	progressMax=100; //всего елментов
	currentProgress=0;  //прошло елементов (напрмер кол-во пакетов в установке)
	set_frontbar(FrontBar); // tyt_progress; 	    //текст перед прогресс баром
	set_fronttext(FrontText);            //текст перед именем пакета 	
	set_title(Title);	    //текст в левом верхнем углу экрана
	set_subtitle(SubTitle);	    //текст в центральном окне посередине
	_list_bar=false; //флаг загрузки списка имен      
	_initcurs=false; //флаг загрузки монитора
	count_position=0;
	progressBarLength=0;
	progressBarPosition=0;
	_alreadyshow=false;
}
progressbar::progressbar(int PackedCounter,string Title,string SubTitle,string FrontText,string FrontBar)
{
	mDebug("Constructor 5");
	initialized=false;
	progressMax=PackedCounter; //всего елментов
	currentProgress=0;  //прошло елементов (напрмер кол-во пакетов в установке)
	set_frontbar(FrontBar); // tyt_progress; 	    //текст перед прогресс баром
	set_fronttext(FrontText);            //текст перед именем пакета 	
	set_title(Title);	    //текст в левом верхнем углу экрана
	set_subtitle(SubTitle);	    //текст в центральном окне посередине
	_list_bar=false; //флаг загрузки списка имен      
	_initcurs=false; //флаг загрузки монитора
	count_position=0;
	progressBarLength=0;
	progressBarPosition=0;
	_alreadyshow=false;
}
void progressbar::init_curses()
{
	mDebug("Initialization start");
	setlocale(LC_ALL,"");
	if(_initcurs!=true)
	{
		if(!initscr()) {
			fprintf(stderr,"type: initscr() failed\n\n");
			exit (1);
		}
	}
	else {
		fprintf(stderr,"WARING: initscr allredy ON\n\n");
	}
	start_color();      //включаем цвет
	// Будущие цвета - включим после полного перехода на ncurses
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
    	init_pair(2, COLOR_BLUE, COLOR_WHITE);
    	init_pair(3, COLOR_RED, COLOR_WHITE);
    	init_pair(4, COLOR_CYAN, COLOR_BLACK);
    	init_pair(5, COLOR_WHITE, COLOR_BLACK);
    	init_pair(6, COLOR_MAGENTA, COLOR_GREEN);
    	init_pair(7, COLOR_BLUE, COLOR_BLACK);
    	init_pair(8, COLOR_WHITE, COLOR_BLACK);
	init_pair(9, COLOR_BLACK, COLOR_BLUE);
	init_pair(10,COLOR_BLACK,COLOR_WHITE);
	init_pair(11,COLOR_WHITE,COLOR_GREEN);
	init_pair(12,COLOR_CYAN,COLOR_RED);
	
	// Цвета режима совместимости
	// Формат: initpair(номер пары, foreground, background)
	/*
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
    	init_pair(2, COLOR_BLUE, COLOR_WHITE);
    	init_pair(3, COLOR_RED, COLOR_WHITE);
    	init_pair(4, COLOR_CYAN, COLOR_BLACK);
    	init_pair(5, COLOR_WHITE, COLOR_BLACK);
    	init_pair(6, COLOR_MAGENTA, COLOR_GREEN);
    	init_pair(7, COLOR_BLUE, COLOR_BLACK);
    	init_pair(8, COLOR_BLACK, COLOR_GREEN);
	init_pair(9, COLOR_BLACK, COLOR_BLUE);
	init_pair(10,COLOR_BLACK,COLOR_WHITE);
	init_pair(11,COLOR_BLACK,COLOR_GREEN);
	init_pair(12,COLOR_CYAN,COLOR_RED);
	*/



    	curs_set(0); //cсделаем физический курсор не видимым :)
    	noecho();    //не показываем ввод
	keypad(stdscr, TRUE);//перехватываем клавиатуру
	//можно еще какие то стартовые сюда добавить для мышки
	return;
}
int progressbar::bar_strlen(char** menu)
{
	int caunt=0;
	while(menu){
		caunt++;
	}
	return caunt;
}
int progressbar::set_fronttext(string name)
{
	fronttext=name;
	return 0;
}
int progressbar::set_fronttext(char* name)
{
	string str(name);
	fronttext=str;
	return 0;
}
int progressbar::set_fronttext(int def)
{
	if(def!= DEF_NAME)return -1;
	def==DEF_NAME ? fronttext="Распаковывается: " : " "; 
	return 0;
}
void progressbar::reg_packed(string Ititle, int count)
{
	init_curses();
	bkgd(COLOR_PAIR(1));
	title=Ititle;
#ifdef DEBUGX	
	getch();
#endif
	progressMax=count;
}
void progressbar::reg_paced(string Ititle, vector<string> list_packed)
{
	init_curses();
	bkgd(COLOR_PAIR(1));
	title=Ititle;
#ifdef DEBUGX
	getch();
#endif	
	progressMax=list_packed.size();
	list_bar=list_packed;
	if(progressMax>0)_list_bar=true;
}
void progressbar::reg_packed(string Ititle, char** list_packed)
{
	init_curses();
	bkgd(COLOR_PAIR(1));
	title=Ititle;
#ifdef DEBUGX
	getch();
#endif
	progressMax=strlen(*list_packed);
	for(int i=0;i<progressMax;i++)
	{
		list_bar.push_back(*(list_packed+i));
	}
	if(progressMax>0)_list_bar=true;
}
int progressbar::set_frontbar(string name)
{
	frontbar=name;
	return 0;
}
int progressbar::set_frontbar(char* name)
{
	string str(name);
	frontbar=str;
	return 0;
}
int progressbar::set_frontbar(int def)
{	
	if(def!= DEF_NAME)return -1;
	def==DEF_NAME ? frontbar=" " : " "; 
	return 0;
}
int progressbar::set_title(string name)
{
	wbkgd(TitleWin, A_BOLD | COLOR_PAIR(11));
	fillwin(TitleWin, " ");
	wmove(TitleWin,0,0);
	if (utf8strlen(name)<utf8strlen(title)) {
		string s;
		for (unsigned int i=0; i<utf8strlen(title); i++) {
			s+=" ";
			if (s.size()>300) {
				abort();
			}
		}
		wprintw(TitleWin, s.c_str());
	}
	wmove(TitleWin,0,0);
	title=name;
	wprintw(TitleWin,title.c_str());
	wrefresh(TitleWin);
	wrefresh(Win);
	return 0;
}
int progressbar::set_title(char* name)
{
	return set_title((string) name);
}
int progressbar::set_subtitle(string name)
{
	Init();
	Show();
	string s;
	for (int i=0; i<=subwincols; i++) {
		s+=" ";
	}
	wmove(SubTitleWin, 0, 0);
	wprintw(SubTitleWin, s.c_str());
	// First, erase old subtitle
	subtitle=name;
	wmove(SubTitleWin,0,(int) ((double) (subwincols-utf8strlen(subtitle))/(double) 2));
	wprintw(SubTitleWin,subtitle.c_str());
	wrefresh(SubTitleWin);
	return 0;
}
int progressbar::set_subtitle(char* name)
{
	return set_subtitle ( (string) name);
}

template <class T>T WAbs(T t1,T t2){
	return t1>=0 ? t1 : t2;
}
/***************************
 * Реализация интерфейса   *
 * *************************/

/*---------------------------------------Function settings any texts------------------------------------*/	
	int progressbar::SetTitle(string Title)
	{
		Init();
		Show();
		mDebug("Setting title " + Title);
		set_title(Title);
		return 0;
	}
	int progressbar::SetSubtitle(string SubTitle)
	{
		Init();
		Show();
		set_subtitle(SubTitle);
		return 0;
	}
	int  progressbar:: SetText(string Text)
	{
		Init();
		Show();
		wmove(TextWin,0,0);
		wrefresh(TextWin);
		wprintw(TextWin,Text.c_str());
		if (utf8strlen(text)>utf8strlen(Text)) {
			for (unsigned int i=0; i<utf8strlen(text)-utf8strlen(Text); i++) {
				wprintw(TextWin, " ");
			}
		}

		text=Text;
		wmove(TextWin,0,0);
		wrefresh(TextWin);
		return 0;
	}
/*---------------------------------------Function managements progress bar-------------------------------*/
	int progressbar::IncreaseProgress() {
		return SetProgress(currentProgress+1);
	}
	int  progressbar::SetProgress(int count) // Выставляет текущую позицию прогресс-бара в соответствии со значением count, вычисляя физический размер относительно виртуального
	{
		if (!dialogMode) return -1;
		if (count > progressMax) {
			//mError("Overflow with subtitle " + subtitle + ", size: " + IntToStr(count) + ", progressMax = " + IntToStr(progressMax));
			return -1;
		}
		Init();
		Show();
		//int physicalShift=0; // Физическое смещение относительно текущей позиции. Длину прогрессбара делаем -2 чтобы не затирало рамку
		int newCurrentPosition =(int)  ((double) ((double) (progressBarLength-2) / (double) progressMax) * (double) count);
		//physicalShift = newCurrentPosition - progressBarPosition;
		// Итак, мы имеем число, на которое надо увеличить или уменьшить текущий прогресс
		wmove(BarWin, 1,1);
		wattron(BarWin,COLOR_PAIR(11));
		for (int i=0; i<newCurrentPosition; i++) {
			wprintw(BarWin, " ");
		}
		wattroff(BarWin, COLOR_PAIR(11));
		wattron(BarWin,COLOR_PAIR(10));
		for (int i=newCurrentPosition; i<progressBarLength-2; i++) {
			wprintw(BarWin, " ");
		}
		wattroff(BarWin, COLOR_PAIR(10));
		wrefresh(BarWin);

		//currentProgress = count; // запишем текущий прогресс чтоб был
		_alreadyshow=true;
		return 0;
	}
	int  progressbar::SetProgressMax(int count)
	{
		// Попытаемся посчитать а сколько же есть текущий виртуальный прогресс в пересчете на новый?
		
		SetProgress(0); // Сброс текущего прогресса
		progressMax=count;
		return 0;
	}
/*--------------------------------------Function managements condishen screen-----------------------------*/
	bool  progressbar::Init()
	{
		if (!dialogMode) return false;
		if (initialized) return true;
		if(!_initcurs)
		{
			init_curses();
			_initcurs=true;
		}
		winlines=LINES;
		wincols=COLS;
		text_lines=2;
		subwincols=(int)( (double)wincols * ( (double) 3/5) );
		starty_subwin=(int)( ((double)winlines * (  (double)7/20)));
		startx_subwin=(int)( (double)wincols * ( (double) 1/5)  );
		progressBarLength=(int)((double)subwincols*((double)5/6));
		height_bar=3;

		subwinlines= height_bar+text_lines+3; //(int)( (double)winlines * ((double) 3/8 ) );
		startx_bar=(int)   (startx_subwin+((double)1/12)*((double)subwincols));
		starty_bar= (int) (starty_subwin+subwinlines-height_bar-1);
		startx_text=startx_bar-1;
		starty_text=starty_bar-text_lines;
		count_position=0;
		initialized=true;
		return true;
	}
	bool  progressbar::Hide()
	{
		printf("Hiding\n");
		mDebug("Hiding");
		delwin(TitleWin);
		delwin(SubWin);
		delwin(Shadow);
		delwin(SubTitleWin);
		delwin(BarWin);
		delwin(Win);
		unreg();
#ifdef DEBUGX
	//	sleep(1);
#endif
		return true;
	}
void progressbar::Refresh()
{
		Init();
		Show();
		wrefresh(Win);
		wrefresh(Shadow);
		wrefresh(SubWin);
		wrefresh(SubTitleWin);
		wrefresh(TitleWin);
		wrefresh(BarWin);
		wrefresh(TextWin);

}
	bool  progressbar::Show(bool force)
	{
		if (!dialogMode) return false;
		if (_alreadyshow && !force) return true;
		if (!initialized) Init();
		mDebug("Start showing something");
		Win		=	subwin(stdscr,LINES-1,COLS,1,0);
		fillwin(Win, "▒");
		TitleWin	=	subwin(stdscr,1,COLS,0,0);
		fillwin(TitleWin, " ");
		wbkgd(TitleWin, A_BOLD | COLOR_PAIR(11));
		SubWin		=	subwin(stdscr,subwinlines,subwincols,starty_subwin,startx_subwin);
		fillwin(SubWin, " ");
		BarWin  	=       subwin(stdscr,height_bar,progressBarLength,starty_bar,startx_bar);
		fillwin(BarWin, " ");
		TextWin		=	subwin(stdscr,text_lines,progressBarLength+2,starty_text,startx_text);
		fillwin(TextWin, " ");
		Shadow  	=       subwin(stdscr,subwinlines,subwincols,starty_subwin+1,startx_subwin+1);	
		fillwin(Shadow, " ");
		SubTitleWin 	= 	subwin(stdscr,1,subwincols,starty_subwin-1,startx_subwin);
		fillwin(SubTitleWin, " ");
		/* Win */
		wmove(Win,0,0);
		wbkgd(Win,COLOR_PAIR(8));
		mDebug("title = [" + title+"]");
		waddstr(TitleWin,title.c_str());
//		wprintw(Win,"Startx_subwin=%i\nStarty_subwin=%i\n,winlines=%i\nwincols=%i\n",startx_subwin,starty_subwin,winlines,wincols);
		/* Shadow */
		/* SubWin */
		wbkgd(SubWin, A_NORMAL | COLOR_PAIR(10));
		wmove(SubWin,0,0);
		box(SubWin,ACS_VLINE,ACS_HLINE); //рисуем рамочку
		//box(TextWin, ACS_VLINE, ACS_HLINE);
		/* SubTitleWin */
		wbkgd(SubTitleWin, A_BOLD | COLOR_PAIR(11));
		wmove(SubTitleWin,0,(int)((subwincols-subtitle.length())/2)-1);
		mDebug("Setting subtitle: " + subtitle);
		wprintw(SubTitleWin,subtitle.c_str());
		/* BarWin */
		wmove(BarWin,0,0);
		wbkgd(BarWin,A_BOLD | COLOR_PAIR(10));	
		box(BarWin,0,0);
		wmove(BarWin,0,0);	
		/* TextWin */
		wbkgd(TextWin,COLOR_PAIR(10));
		wmove(TextWin,0,0);
		wprintw(TextWin,text.c_str());
		wmove(TextWin,0,0);
		wattron(BarWin,COLOR_PAIR(10));//установили цвет зарисовки
#ifdef DEBUGX
		getch();
#endif
		_alreadyshow=true;

		Refresh();
		return true;
	}
	bool  progressbar::Exit()
	{
		if (!initialized) return false;
#ifdef DEBUGX		
		getch();
#endif

		delwin(TitleWin);
		delwin(SubTitleWin);
		delwin(BarWin);
		delwin(SubWin);
		delwin(Shadow);
		delwin(Win);
		unreg();
		return true;
	}
	progressbar::~progressbar()
	{
		Exit();
	}
	bool progressbar::_initcurs;
