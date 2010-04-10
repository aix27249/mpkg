#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H


#include <iostream>
#include <ncursesw/ncurses.h>
#include <vector>

#define DEF_NAME (int)766456
#define DEF_TYTLE
//#define DEBUGX


using namespace std;

class progressbar{

	/********************************************************************************
	 *										*
	 *	Дальше идут ф-ии интерфейса, которые будут потом переведены		*
	 *	в class interfase_installer;						*
	 *										*
	 *******************************************************************************/
public:
/*Constructors: default, copy, Titles, countet & Titles*/
	progressbar( void );
	progressbar( progressbar&);
	progressbar( char*);
	progressbar( string Title, string SubTitle="", string FrontText="", string FrontBar="");
	progressbar( int PackedCounter, string Title="", string SubTitle="", string FrontText="", string FrontBar="");
	~progressbar();
/*Function settings any texts            	 */	
	int SetTitle(string);
	int SetSubtitle(string);
	int SetText(string);
/*Function managements progress bar		 */
	int IncreaseProgress();
	int SetProgress(int);
	int SetProgressMax(int);

/*Function managements condishen screen  	 */
	bool Init();
	bool initialized;
	bool Hide();
	bool Show(bool force = false);
	void Refresh();
	bool Exit();
/****************************************************************************************************************************************************************
 *																				*
 *						<Краткое описание функций интерфейса класса "progressbar">	состояние на 20/02/08				*
 *																				*
 *	progressbar( void/string );	-конструктор ставит по умолчанию число элементов равным 100, если есть стринг то - это отображается в л.в. углу экрана	*
 *	int SetTitle(string);		-Установить титл экрана													*
 *	int SetSubtitle(string);	-Установить текст в заголовке во внутренем экране									*
 *	int SetText(string);   		-Установить текст выше прогрессбара 											*
 *	int SetProgress(int);		-установить кол-во пройденых этапов из максимального 									*
 *	int SetProgressMax(int);	-установить общее кол-во этапов												*
 *	bool Init();			-обязательная ф-ия в начале работы											*
 *	bool Hide();			-временная остановка графического отображения										*
 *	bool Show();			-начало работы с цветным экраном или возбновление работы после ф-ии Hide()						*
 *	bool Exit();			-завершающая ф-ия													*
 *																				*
 *																				*
 * **************************************************************************************************************************************************************/
protected:
	long long int progressMax; //всего виртуальных шагов
	long long int currentProgress;  //текущий виртуальный прогресс
	
	int progressBarLength;               //физическая длина прогрессбара
	int progressBarPosition;           // текущий физический прогресс
	/*
	long long int max_counter; //всего всего пакетов грубо говоря
	long long int set_counter;  //прошло елементов от макс коунтер	
	int long_bar;               //общая число елемнтов прогресса бара
	int position_bar;           // отрисовано число элементов бара из long_bar

	 */
	int count_position;

	vector<string> list_bar;    //список имен устанавливаемых
	string frontbar; // tyt_progress; 	    //текст перед прогресс баром
	string fronttext;            //текст перед именем пакета 	
	string title;	    //текст в левом верхнем углу экрана
	string subtitle;	    //текст в центральном окне посередине
	string text;

	bool _list_bar;             //Флаг загрузки списка имен
	static bool _initcurs;
	bool _alreadyshow;
	WINDOW* Win;
	WINDOW* TitleWin;
	WINDOW* SubWin;
	WINDOW* TextWin;
	WINDOW* BarWin;
	WINDOW* Shadow;
	WINDOW* SubTitleWin;
	/************************
	 *	Размеры окна	*
	 * **********************/
	int winlines;
	int wincols;

	int subwinlines;
	int subwincols;
	int text_lines;

	int startx_subwin;
	int starty_subwin;
	int long_subwin;
	int hight_subwin;

	//int long_bar;
	int height_bar;
	int startx_bar;//относительно subwin
	int starty_bar;//относительно subwin
	int startx_text;
	int starty_text;

void init_curses();

/*ф-ии для внутренне реализации*/
//private:
public:
/************************************************************************
 *									*
 *	template<class T>int set_fronttext(T name);			*
 *									*
 *	Установка текста перед строкой с именем устанавливаемого пакета;*
 *	Работает (пока) с char* и string и shot int когда вызываетсо по *
 *	умолчанию (макрос определен в библиотеке как DEF_NAME).		*
 *	в случаи ошибки (например слишком длинная строка) возвращает -1.*
 *									*
 * ********************************************************************/
int set_fronttext(string name);
int set_fronttext(char* name);
int set_fronttext (int def);
/************************************************************************
*									*
*	template<class T> int set_frontbar(T);			*
*									*
*	Устанавливает текст непосредственно перед прогресс баром	*
*	Определен макрос DEF_NAME - to default	- "Ход установки: "	*
************************************************************************/
int set_frontbar(string);
int set_frontbar(char *);/******Внутренняя ф-ия******/
int set_frontbar(int); //default
/********************************
*внутреняя ф-ия для установки титла (в левом верхнем углу)
*
********************************/
int set_title(char*);
int set_title(string);
//int set_title(int);
/******************************
 *Внутреняя ф-ия для установки subtitle;
 *
 ******************************/
int set_subtitle(string);
int set_subtitle(char*);
/***Внутреняя ф-ия***/
int bar_strlen(char** menu);
/******************************************************
 *
 *	void reg_packed(string title, int count);
 *	void reg_packed(string title, vector<string> list_packed);
 *	void reg_packed(tring title, char* list_packed);
 *
 *	Перезагружаемая стартовая функция поргресс бара служит для инициализации базового окна; 
 *	
 *	string title - заголовок, выводится в левый верхний угол экрана;
 *	| int count - кол-во элементов для прогресс бара (при использовании этой реализации функции прийдется каждый раз отправлять имя пакета)
 *	| vector<string> list_packed - список (вектор) имен, устранавливаемых пакетов.
 *	| char* list_packed  - список имен, устанавливаемый библиотек
 *
 *
 * ****************************************************/
void reg_packed(string title, int count);
void reg_paced(string title, vector<string> list_packed);
void reg_packed(string title, char** list_packed);
/************************************************************************************************************
 * 		void unreg();
 *
 *          	Обязательная функия в конце использования библиотеки !!! (можно потом добавить в деструктор)
 *
 ************************************************************************************************************/
void unreg()
{endwin();}
/**************************************************
 *
 *	void puch_bar(char**); / void wpush_bar(char**);
 *
 *	Функция для постороения списка имен, устанавливаемых пакетов, не изменяет / изменяет кол-во пакетов (необязательно)
 *
 * ***********************************************/

void push_bar(char** list_pucked);
void wpush_bar(char** list_pucked);
int AddPossBar(int);
int AddVirtualBar(int);

};
#endif
