#include "TKplayer.h"
#include "resource.h"

HWND hwnd,hwndnew;
HANDLE hout,mutex;
ITaskbarList *pobj;
HINSTANCE hinst;
NOTIFYICONDATA nid;

vector<SONG> songlist;
list<int> indexlist,songnolist;
char curlistname[MAXLEN];
int songnum,playmode=RANDOM_MODE;
bool ishide=0;

DWORD WINAPI tkplayer(LPVOID lpParam);
DWORD WINAPI deletetab(LPVOID lpParam);
LRESULT CALLBACK wndproc(HWND hwndr,UINT message,WPARAM wParam,LPARAM lParam);
void createinvisiblewindow(WNDCLASS &wndclass);
void setnidinfo();

void init();
int readcache();
bool stdinlist(char *str,bool needclear);
int readlist(const char *listname);
void getlyric(LYRIC &lyric,const char *songname);
int newsong(const int lastplaysongno);
int play();
void drawplayer(const int vol,const int mode,const bool isquiet);
void clearline(const int linenum);
void printvolume(const int vol,const bool isquiet);
void printtime(const int position,const int length,const bool changelength);
void printlyric(const LYRIC &lyric,const int lyricno);
void writecache(const int songno,const int modeno);

void init()
{
	COORD size={42,11};
	SMALL_RECT rc={0,0,41,10};
	CONSOLE_CURSOR_INFO ConsoleCursorInfo;
	hwnd=::FindWindow("ConsoleWindowClass",NULL);
	hinst=GetModuleHandle(NULL);
	SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
	SetConsoleTitle("TKplayer");
	hout=GetStdHandle(STD_OUTPUT_HANDLE);
	setcolor(hout,WHITE);
	SetConsoleWindowInfo(hout,true,&rc);
	SetConsoleScreenBufferSize(hout,size);
	GetConsoleCursorInfo(hout,&ConsoleCursorInfo);
	ConsoleCursorInfo.bVisible=false;
	SetConsoleCursorInfo(hout,&ConsoleCursorInfo);
	srand(unsigned int(time(0)));
}

int readcache()
{
	ifstream infile;
	list<int>::iterator iter;
	char str[MAXLEN];
	int tmp=0;
	infile.open("TKcache");
	if(infile==NULL)return -1;
	infile.getline(str,MAXLEN);
	if(readlist(str)<=0)
	{
		infile.close();
		return 0;
	}
	infile.getline(str,MAXLEN);
	infile>>tmp;
	if(tmp>=0&&tmp<=2)playmode=tmp;
	for(iter=indexlist.begin();iter!=indexlist.end();iter++)
		if(!strcmp(songlist[*iter].songname,str))
		{
			songnolist.push_back(*iter);
			indexlist.erase(iter);
			infile.close();
			return 2;
		}
	infile.close();
	return 1;
}

bool stdinlist(char *str,bool needclear)
{
	CONSOLE_CURSOR_INFO ConsoleCursorInfo;
	char tmp[MAXLEN];
	int listt=-1;
	SetConsoleTitle("TKplayer");
	system("cls");
	setpos(hout,0,0);
	setcolor(hout,BLACK);
	if(needclear)
	{
		time_t waittime=clock();
		keybd_event(VK_RETURN,0,0,0);
		while(cin.getline(tmp,MAXLEN))
		{
			if((strlen(tmp)>0&&tmp[strlen(tmp)-1]=='q')||clock()-waittime>200)break;
			keybd_event(VK_RETURN,0,0,0);
		}
	}
	cout<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl;
	setpos(hout,0,0);
	setcolor(hout,WHITE);
	cout<<"请输入列表名称：（键入\".ex\"退出程序）\n";
	GetConsoleCursorInfo(hout,&ConsoleCursorInfo);
	ConsoleCursorInfo.bVisible=true;
	SetConsoleCursorInfo(hout,&ConsoleCursorInfo);
	do
	{
		do
		{
			setpos(hout,0,1);
			cin.getline(str,MAXLEN);
			if(!strnicmp(str,".ex",3))return 1;
		}
		while(str[0]=='\0');
		if(!strncmp(str,"桌面\\",5)||!strnicmp(str,"desktop\\",8))
		{
			if(!strnicmp(str,"desktop\\",8))
			{
				strcpy(tmp,str);
				strcpy(str,"桌面\\");
				strcat(str,tmp+8);
			}
			strcpy(tmp,str);
			strcpy(str,"C:\\Documents and Settings\\Administrator\\");
			strcat(str,tmp);
		}
		listt=readlist(str);
		if(listt<0)
		{
			MessageBeep(MB_ICONERROR);
			system("cls");
			setpos(hout,0,0);
			cout<<"出错！请重新输入：（键入\".exit\"退出程序）\n";
		}
//		if(usetmp)remove("~$tmp.tkpl");
	}
	while(listt<0);
	ConsoleCursorInfo.bVisible=false;
	SetConsoleCursorInfo(hout,&ConsoleCursorInfo);
	return 0;
}

int readlist(const char *listname)
{
	const char suffix[SUFFIX_NUM][7]={"mp3","wma","wav","mid","midi"};
	ifstream infile;
	vector<string> vstr;
	vector<int> vl,vr;
	string tmp;
	int len=strlen(listname),i,j,dotpos,suffixnum;
	char str[MAXLEN];
	bool getfilesucceed=0;
	if(len==0)return -1;
	if(listname[len-1]=='.')
	{
		getfilesucceed=getallfile(listname,"~$tmp.tkpl");
		if(!getfilesucceed)return -1;
		infile.open("~$tmp.tkpl");
	}
	else infile.open(listname);
	if(infile==NULL)return -1;
	songlist.clear();
	while(infile.getline(str,MAXLEN))
	{
		ifstream infilesongname;
		if(str[0]=='\0')continue;
		infilesongname.open(str);
		if(infilesongname!=NULL)
		{
			bool namesame=0,suffixmatch=0;
			infilesongname.close();
			dotpos=0;
			for(i=strlen(str)-1;i>=0;i--)
				if(str[i]=='.')
				{
					dotpos=i;
					break;
				}
			if(dotpos>0)
			{
				for(i=0;i<SUFFIX_NUM;i++)
					if(!stricmp(str+dotpos+1,suffix[i]))
					{
						suffixmatch=1;
						suffixnum=i;
						break;
					}
			}
			if(!suffixmatch)continue;
			tmp=str;
			if(vl.empty())
			{
				vstr.push_back(tmp);
				vl.push_back(-1);
				vr.push_back(-1);
			}
			else
			{
				i=0;
				while(1)
				{
					j=newstrcmp(tmp,vstr[i]);
					if(j==0)
					{
						namesame=1;
						break;
					}
					else if(j>0)
					{
						if(vr[i]!=-1)i=vr[i];
						else
						{
							vstr.push_back(tmp);
							vr[i]=vr.size();
							vr.push_back(-1);
							vl.push_back(-1);
							break;
						}
					}
					else
					{
						if(vl[i]!=-1)i=vl[i];
						else
						{
							vstr.push_back(tmp);
							vl[i]=vl.size();
							vr.push_back(-1);
							vl.push_back(-1);
							break;
						}
					}
				}
			}
			if(!namesame)songlist.push_back(SONG(str));
			songlist.back().suffix=suffixnum;
		}
	}
	infile.close();
	vstr.clear();
	vl.clear();
	vr.clear();
	if(getfilesucceed)remove("~$tmp.tkpl");
	if(songlist.empty())
	{
		dotpos=0;
		for(i=strlen(listname)-1;i>=0;i--)
			if(listname[i]=='.')
			{
				dotpos=i;
				break;
			}
		if(dotpos!=0)
		{
			for(i=0;i<SUFFIX_NUM;i++)
				if(!stricmp(listname+dotpos+1,suffix[i]))
				{
					songlist.push_back(SONG(listname));
					songlist.back().suffix=i;
					break;
				}
		}
	}
	strcpy(curlistname,listname);
	songnum=songlist.size();
	indexlist.clear();
	songnolist.clear();
	for(i=0;i<songlist.size();i++)
		indexlist.push_back(i);
	return 1;
}

void getlyric(LYRIC &lyric,const char *songname)
{
	int i,len=strlen(songname);
	char lyricfilename[MAXLEN];
	strcpy(lyricfilename,songname);
	for(i=len-1;i>=0;i--)
	{
		if(lyricfilename[i]=='.')break;
	}
	strcpy(lyricfilename+i+1,"lrc");
	lyric.getlyric(lyricfilename);
//	for(i=0;i<lyric.context.size();i++)
//		cout<<lyric.displaytime[i]<<endl<<lyric.context[i]<<endl;
}

int newsong(const int lastplaysongno)
{
	list<int>::iterator iter;
	int songno,i;
	if(indexlist.empty())
	{
		for(i=0;i<songnum;i++)
			indexlist.push_back(i);
	}
	i=rand()%indexlist.size();
	iter=indexlist.begin();
	while(i>0)
	{
		iter++;
		i--;
	}
	songno=*iter;
	if(songnum==1)return songno;
	i=0;
	while(songno==lastplaysongno&&i<=indexlist.size())
	{
		iter++;
		i++;
		if(iter==indexlist.end())iter=indexlist.begin();
		songno=*iter;
	}
	indexlist.erase(iter);
	if(songlist[songno].preference>0)songlist[songno].preference--;
	return songno;
}

int play()
{
	const int volumeval[6]={0,200,400,600,800,1000};
	const char suffix[SUFFIX_NUM][7]={"mp3","wma","wav","mid","midi"};
	LYRIC lyric;
	list<int>::iterator prevsong,cursong,nextsong;
	static int curvolume=4;
	int playsongno,errno,usetmp,i,position,songlength1,songlength2,ret=-1,realsuffix;
	int lyricno,lastlyricno,lyricoffset,tmpmode,lastposition,realpos,lastseekpos;
	bool ispause=0,tonext=1,isquiet=0,usesl2;
	char cmd[MAXLEN],response[MAXLEN],name1[MAXLEN],name2[MAXLEN],songshortname[MAXLEN],tmpstr[MAXLEN];
	drawplayer(curvolume,playmode,isquiet);
	if(playmode==RANDOM_MODE)
	{
		if(songnolist.empty())songnolist.push_back(newsong(-1));
		prevsong=NULL;
		cursong=songnolist.begin();
		nextsong=cursong;
		nextsong++;
		playsongno=*cursong;
	}
	else if(!songnolist.empty())
	{
		playsongno=*songnolist.begin();
		songnolist.clear();
	}
	else playsongno=0;
	while(1)
	{
		ispause=0;
		tonext=1;
		usetmp=-1;
		lyricno=lastlyricno=-1;
		lyricoffset=0;
		lyric.findlyric=0;
		realpos=lastseekpos=lastposition=0;
		tmpmode=playmode;
		realsuffix=songlist[playsongno].suffix;
		songlist[playsongno].getsongshortname(songshortname);
		sprintf(cmd,"open %s Alias music%d",songshortname,playsongno);
		errno=mciSendString(cmd,NULL,0,0);
		getlimitstr(songlist[playsongno].songname,tmpstr,42,1);
		setpos(hout,0,0);
		setcolor(hout,BLACK);
		cout<<"                                          ";
		setpos(hout,0,0);
		setcolor(hout,WHITE);
		cout<<tmpstr;
		setpos(hout,0,1);
//		cout<<"正在播放： "<<songlist[playsongno].songname<<endl<<songlist[playsongno].preference<<endl;
		if(errno==277)
		{
			sprintf(cmd,"close music%d",playsongno);
			errno=mciSendString(cmd,NULL,0,0);
			CopyFile(songlist[playsongno].longname,"~$tmp.mp3",FALSE);
			sprintf(cmd,"open ~$tmp.mp3 Alias music%d",playsongno);
			errno=mciSendString(cmd,NULL,0,0);
			if(errno==0)usetmp=0;
			else
			{
				for(i=1;i<SUFFIX_NUM-1;i++)
				{
					sprintf(cmd,"close music%d",playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					sprintf(name1,"~$tmp.%s",suffix[i-1]);
					sprintf(name2,"~$tmp.%s",suffix[i]);
					remove(name2);
					rename(name1,name2);
					sprintf(cmd,"open ~$tmp.%s Alias music%d",suffix[i],playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					usetmp=i;
					if((!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('A')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_LEFT)<0))
					{
						setcolor(hout,RED);
						setpos(hout,0,5);
						cout<<"[A][Ctrl+←]";
						while((hwnd==GetForegroundWindow()&&GetKeyState('A')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_LEFT)<0)){};
						setcolor(hout,YELLOW);
						setpos(hout,0,5);
						cout<<"[A][Ctrl+←]";
						tonext=0;
						break;
					}
					if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('Q')<0)
					{
						setcolor(hout,RED);
						setpos(hout,18,7);
						cout<<"[Q]";
						while(hwnd==GetForegroundWindow()&&GetKeyState('Q')<0){};
						setcolor(hout,YELLOW);
						setpos(hout,18,7);
						cout<<"[Q]";
						ret=1;
					}
					if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('F')<0)
					{
						setcolor(hout,RED);
						setpos(hout,0,7);
						cout<<"[F]";
						while(hwnd==GetForegroundWindow()&&GetKeyState('F')<0){};
						setcolor(hout,YELLOW);
						setpos(hout,0,7);
						cout<<"[F]";
						ret=2;
					}
					if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('T')<0)
					{
						setcolor(hout,RED);
						setpos(hout,9,7);
						cout<<"[T]";
						while(hwnd==GetForegroundWindow()&&GetKeyState('T')<0){};
						setcolor(hout,YELLOW);
						setpos(hout,9,7);
						cout<<"[T]";
						ShowWindow(hwnd,SW_HIDE);
						WaitForSingleObject(mutex,INFINITE);
						ishide=1;
						ReleaseMutex(mutex);
					}
					if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState(VK_ESCAPE)<0)
					{
						setcolor(hout,RED);
						setpos(hout,31,7);
						cout<<"[ESC]";
						while(hwnd==GetForegroundWindow()&&GetKeyState(VK_ESCAPE)<0){};
						setcolor(hout,YELLOW);
						setpos(hout,31,7);
						cout<<"[ESC]";
						setpos(hout,0,10);
						ret=0;
					}
					if(errno==0)
					{
						realsuffix=i;
						break;
					}
				}
			}
		}
		sprintf(cmd,"play music%d",playsongno);
		errno=mciSendString(cmd,NULL,0,0);
		if(errno!=0)
		{
			sprintf(cmd,"stop music%d",playsongno);
			mciSendString(cmd,NULL,0,0);
			sprintf(cmd,"close music%d",playsongno);
			mciSendString(cmd,NULL,0,0);
			MessageBeep(MB_ICONERROR);
			setpos(hout,0,1);
			setcolor(hout,WHITE);
			cout<<"无法播放  ";
			setpos(hout,0,9);
			cout<<"音频格式与扩展名不匹配或文件不存在或已损坏";
			for(i=0;i<8;i++)
			{
				Sleep(30);
				if((!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('A')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_LEFT)<0))
				{
					setcolor(hout,RED);
					setpos(hout,0,5);
					cout<<"[A][Ctrl+←]";
					while((hwnd==GetForegroundWindow()&&GetKeyState('A')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_LEFT)<0)){};
					setcolor(hout,YELLOW);
					setpos(hout,0,5);
					cout<<"[A][Ctrl+←]";
					tonext=0;
				}
				if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('Q')<0)
				{
					setcolor(hout,RED);
					setpos(hout,18,7);
					cout<<"[Q]";
					while(hwnd==GetForegroundWindow()&&GetKeyState('Q')<0){};
					setcolor(hout,YELLOW);
					setpos(hout,18,7);
					cout<<"[Q]";
					ret=1;
				}
				if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('F')<0)
				{
					setcolor(hout,RED);
					setpos(hout,0,7);
					cout<<"[F]";
					while(hwnd==GetForegroundWindow()&&GetKeyState('F')<0){};
					setcolor(hout,YELLOW);
					setpos(hout,0,7);
					cout<<"[F]";
					ret=2;
				}
				if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('T')<0)
				{
					setcolor(hout,RED);
					setpos(hout,9,7);
					cout<<"[T]";
					while(hwnd==GetForegroundWindow()&&GetKeyState('T')<0){};
					setcolor(hout,YELLOW);
					setpos(hout,9,7);
					cout<<"[T]";
					ShowWindow(hwnd,SW_HIDE);
					WaitForSingleObject(mutex,INFINITE);
					ishide=1;
					ReleaseMutex(mutex);
				}
				if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState(VK_ESCAPE)<0)
				{
					setcolor(hout,RED);
					setpos(hout,31,7);
					cout<<"[ESC]";
					while(hwnd==GetForegroundWindow()&&GetKeyState(VK_ESCAPE)<0){};
					setcolor(hout,YELLOW);
					setpos(hout,31,7);
					cout<<"[ESC]";
					setpos(hout,0,10);
					ret=0;
				}
			}
		}
		else
		{
			getlyric(lyric,songlist[playsongno].songname);
			for(i=strlen(songlist[playsongno].songname)-1;i>=0;i--)
				if(songlist[playsongno].songname[i]=='.')break;
			tmpstr[i]='\0';
			for(i--;i>=0;i--)
				tmpstr[i]=songlist[playsongno].songname[i];
			strcat(tmpstr," - TKplayer");
			SetConsoleTitle(tmpstr);
			_tcscpy(nid.szTip,TEXT(tmpstr));
			Shell_NotifyIcon(NIM_MODIFY,&nid);
			if(!lyric.findlyric||lyric.context.empty())
			{
				setcolor(hout,WHITE);
				setpos(hout,13,9);
				cout<<"未找到匹配的歌词";
			}
			setpos(hout,0,1);
			cout<<"正在播放中";
			setpos(hout,12,3);
			setcolor(hout,YELLOW);
			cout<<"暂停  ";
			if(realsuffix<2)
			{
				sprintf(cmd,"status music%d length",playsongno);
				songlength2=0;
				mciSendString(cmd,response,MAXLEN,0);
				songlength2=atoi(response);
				if(songlist[playsongno].suffix<=12)songlength1=getlengthfromshell(songlist[playsongno].longname);
				else songlength1=-1;
				if(songlength1==-1||(songlength2-songlength1<=3000&&songlength1-songlength2>=3000))
				{
					usesl2=1;
					printtime(0,songlength2,1);
				}
				else
				{
					usesl2=0;
					printtime(0,songlength1,1);
				}
			}
			else
			{
				setcolor(hout,BLACK);
				setpos(hout,25,1);
				cout<<"                 ";
			}
			if(realsuffix>=2)
			{
				printvolume(5,0);
			}
			else if(isquiet)
			{
				sprintf(cmd,"setaudio music%d volume to 0",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				printvolume(0,1);
//				cout<<"vol:"<<0<<endl;
			}
			else
			{
				if(curvolume!=5)
				{
					sprintf(cmd,"setaudio music%d volume to %d",playsongno,volumeval[curvolume]);
					errno=mciSendString(cmd,NULL,0,0);
				}
//				cout<<"vol:"<<curvolume<<endl;
				printvolume(curvolume,0);
			}
			writecache(playsongno,playmode);
		}
		while(tonext&&errno==0)
		{
			Sleep(20);
			sprintf(cmd,"status music%d mode",playsongno);
			errno=mciSendString(cmd,response,MAXLEN,0);
			if(!strncmp(response,"stopped",7))
			{
				sprintf(cmd,"close music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				break;
			}
			if(realsuffix<2)
			{
				sprintf(cmd,"status music%d position",playsongno);
				errno=mciSendString(cmd,response,MAXLEN,0);
				position=atoi(response);
				if(usesl2||songlength2==0)realpos=position;
				else realpos=(lastseekpos+0.0)*((songlength1+0.0)/(songlength2+0.0))+position-lastseekpos;
				if(realpos/1000!=lastposition/1000)printtime(realpos,songlength2,0);
				lastposition=realpos;
			}
			if(lyric.findlyric&&!lyric.context.empty()&&realsuffix<2)
			{
				lastlyricno=lyricno;
				lyricno=lyric.getlyricno(realpos,OFFSET_DEFAULT+lyricoffset);
				if(lyricno!=-2&&lyricno!=lastlyricno)printlyric(lyric,lyricno);
//				if(lyricno!=-1&&lyricno!=lastlyricno)cout<<lyric.displaytime[lyricno]<<" "<<lyric.context[lyricno]<<endl;
			}
			if((!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('S')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_DOWN)<0)) // 暂停/继续
			{
				setcolor(hout,RED);
				setpos(hout,0,3);
				cout<<"[S][Ctrl+↓]";
				while((hwnd==GetForegroundWindow()&&GetKeyState('S')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_DOWN)<0)){};
				setcolor(hout,YELLOW);
				setpos(hout,0,3);
				cout<<"[S][Ctrl+↓]";
				if(ispause)
				{
					sprintf(cmd,"resume music%d",playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					ispause=0;
					setpos(hout,0,1);
					setcolor(hout,WHITE);
					cout<<"正在播放中";
					setpos(hout,12,3);
					setcolor(hout,YELLOW);
					cout<<"暂停  ";
				}
				else
				{
					sprintf(cmd,"pause music%d",playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					ispause=1;
					setpos(hout,0,1);
					setcolor(hout,WHITE);
					cout<<"已暂停    ";
					setpos(hout,12,3);
					setcolor(hout,YELLOW);
					cout<<"继续  ";
				}
			}
			if((!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('D')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_RIGHT)<0)) // 下一首
			{
				setcolor(hout,RED);
				setpos(hout,0,4);
				cout<<"[D][Ctrl+→]";
				while((hwnd==GetForegroundWindow()&&GetKeyState('D')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_RIGHT)<0)){};
				setcolor(hout,YELLOW);
				setpos(hout,0,4);
				cout<<"[D][Ctrl+→]";
				sprintf(cmd,"stop music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				sprintf(cmd,"close music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				break;
			}
			if((!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('A')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_LEFT)<0)) // 上一首
			{
				setcolor(hout,RED);
				setpos(hout,0,5);
				cout<<"[A][Ctrl+←]";
				while((hwnd==GetForegroundWindow()&&GetKeyState('A')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_LEFT)<0)){};
				setcolor(hout,YELLOW);
				setpos(hout,0,5);
				cout<<"[A][Ctrl+←]";
				sprintf(cmd,"stop music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				sprintf(cmd,"close music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				tonext=0;
				break;
			}
			if((!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('R')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_UP)<0)) // 喜欢
			{
				setcolor(hout,RED);
				setpos(hout,0,6);
				cout<<"[R][Ctrl+↑]";
				while((hwnd==GetForegroundWindow()&&GetKeyState('R')<0)||(GetKeyState(VK_CONTROL)<0&&GetKeyState(VK_UP)<0)){};
				setcolor(hout,YELLOW);
				setpos(hout,0,6);
				cout<<"[R][Ctrl+↑]";
				if(songlist[playsongno].preference<indexlist.size()/3&&songlist[playsongno].preference<20)
				{
					songlist[playsongno].preference++;
					indexlist.push_back(playsongno);
				}
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&realsuffix<2&&GetKeyState('C')<0) // 快进
			{
				setcolor(hout,RED);
				setpos(hout,18,3);
				cout<<"[C]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('C')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,18,3);
				cout<<"[C]";
				sprintf(cmd,"status music%d position",playsongno);
				errno=mciSendString(cmd,response,MAXLEN,0);
				position=atoi(response);
				if(usesl2||songlength2==0)realpos=position;
				else realpos=(lastseekpos+0.0)*((songlength1+0.0)/(songlength2+0.0))+position-lastseekpos;
				if(position>=0)
				{
					if(usesl2||songlength2==0)sprintf(cmd,"seek music%d to %d",playsongno,position+5000);
					else sprintf(cmd,"seek music%d to %d",playsongno,int((realpos+5000.0)*((songlength2+0.0)/(songlength1+0.0))));
					errno=mciSendString(cmd,NULL,0,0);
					sprintf(cmd,"status music%d position",playsongno);
					errno=mciSendString(cmd,response,MAXLEN,0);
					lastseekpos=atoi(response);
					sprintf(cmd,"play music%d",playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					if(usesl2||songlength2==0)realpos=lastseekpos;
					else realpos=(lastseekpos+0.0)*((songlength1+0.0)/(songlength2+0.0));
				}
				if(ispause)
				{
					ispause=0;
					setpos(hout,0,1);
					setcolor(hout,WHITE);
					cout<<"正在播放中";
					setpos(hout,12,3);
					setcolor(hout,YELLOW);
					cout<<"暂停  ";
				}
				if(realpos/1000!=lastposition/1000)printtime(realpos,songlength2,0);
				lastposition=realpos;
//				cout<<position<<endl;
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&realsuffix<2&&GetKeyState('Z')<0) // 快退
			{
				setcolor(hout,RED);
				setpos(hout,18,4);
				cout<<"[Z]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('Z')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,18,4);
				cout<<"[Z]";
				sprintf(cmd,"status music%d position",playsongno);
				errno=mciSendString(cmd,response,MAXLEN,0);
				position=atoi(response);
				if(usesl2||songlength2==0)realpos=position;
				else realpos=(lastseekpos+0.0)*((songlength1+0.0)/(songlength2+0.0))+position-lastseekpos;
				if(position>0)
				{
					if(realpos>5000)
					{
						if(usesl2||songlength2==0)sprintf(cmd,"seek music%d to %d",playsongno,position-5000);
						else sprintf(cmd,"seek music%d to %d",playsongno,int((realpos-5000.0)*((songlength2+0.0)/(songlength1+0.0))));
					}
					else sprintf(cmd,"seek music%d to 0",playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					sprintf(cmd,"status music%d position",playsongno);
					errno=mciSendString(cmd,response,MAXLEN,0);
					lastseekpos=atoi(response);
					sprintf(cmd,"play music%d",playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					if(usesl2||songlength2==0)realpos=lastseekpos;
					else realpos=(lastseekpos+0.0)*((songlength1+0.0)/(songlength2+0.0));
				}
				if(ispause)
				{
					ispause=0;
					setpos(hout,0,1);
					setcolor(hout,WHITE);
					cout<<"正在播放中";
					setpos(hout,12,3);
					setcolor(hout,YELLOW);
					cout<<"暂停  ";
				}
				if(realpos/1000!=lastposition/1000)printtime(realpos,songlength2,0);
				lastposition=realpos;
//				cout<<position<<endl;
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&realsuffix<2&&GetKeyState('W')<0) // 大声
			{
				setcolor(hout,RED);
				setpos(hout,31,3);
				cout<<"[W]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('W')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,31,3);
				cout<<"[W]";
				if(isquiet)
				{
					isquiet=0;
					curvolume=1;
					setcolor(hout,YELLOW);
					setpos(hout,34,5);
					cout<<"静音    ";
				}
				else curvolume++;
				if(curvolume>5)curvolume=5;
				sprintf(cmd,"setaudio music%d volume to %d",playsongno,volumeval[curvolume]);
				errno=mciSendString(cmd,NULL,0,0);
				printvolume(curvolume,0);
//				cout<<curvolume<<endl;
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&realsuffix<2&&GetKeyState('X')<0) // 小声
			{
				setcolor(hout,RED);
				setpos(hout,31,4);
				cout<<"[X]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('X')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,31,4);
				cout<<"[X]";
				if(!isquiet)
				{
					curvolume--;
					if(curvolume<0)curvolume=0;
					sprintf(cmd,"setaudio music%d volume to %d",playsongno,volumeval[curvolume]);
					errno=mciSendString(cmd,NULL,0,0);
					printvolume(curvolume,0);
//					cout<<curvolume<<endl;
				}
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&realsuffix<2&&GetKeyState('E')<0) // 静音
			{
				setcolor(hout,RED);
				setpos(hout,31,5);
				cout<<"[E]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('E')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,31,5);
				cout<<"[E]";
				if(isquiet)
				{
					isquiet=0;
					sprintf(cmd,"setaudio music%d volume to %d",playsongno,volumeval[curvolume]);
					errno=mciSendString(cmd,NULL,0,0);
					setcolor(hout,YELLOW);
					setpos(hout,34,5);
					cout<<"静音    ";
					printvolume(curvolume,0);
//					cout<<curvolume<<endl;
				}
				else
				{
					isquiet=1;
					sprintf(cmd,"setaudio music%d volume to 0",playsongno);
					errno=mciSendString(cmd,NULL,0,0);
					setcolor(hout,YELLOW);
					setpos(hout,34,5);
					cout<<"恢复声音";
					printvolume(0,1);
//					cout<<0<<endl;
				}
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&realsuffix<2&&GetKeyState('V')<0) // 歌词延后（歌词比歌慢）
			{
				setcolor(hout,RED);
				setpos(hout,18,5);
				cout<<"[V]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('V')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,18,5);
				cout<<"[V]";
				if(lyric.findlyric&&!lyric.context.empty())
				{
					lyricoffset+=250;
					lastlyricno=lyricno;
					lyricno=lyric.getlyricno(realpos,OFFSET_DEFAULT+lyricoffset);
					if(lyricno!=-2&&lyricno!=lastlyricno)printlyric(lyric,lyricno);
//					if(lyricno!=-1&&lyricno!=lastlyricno)cout<<lyric.displaytime[lyricno]<<" "<<lyric.context[lyricno]<<endl;
				}
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&realsuffix<2&&GetKeyState('B')<0) // 歌词提前（歌词比歌快）
			{
				setcolor(hout,RED);
				setpos(hout,18,6);
				cout<<"[B]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('B')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,18,6);
				cout<<"[B]";
				if(lyric.findlyric&&!lyric.context.empty())
				{
					lyricoffset-=250;
					lastlyricno=lyricno;
					lyricno=lyric.getlyricno(realpos,OFFSET_DEFAULT+lyricoffset);
					if(lyricno!=-2&&lyricno!=lastlyricno)printlyric(lyric,lyricno);
//					if(lyricno!=-1&&lyricno!=lastlyricno)cout<<lyric.displaytime[lyricno]<<" "<<lyric.context[lyricno]<<endl;
				}
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('G')<0) // 随机/顺序
			{
				setcolor(hout,RED);
				setpos(hout,31,6);
				cout<<"[G]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('G')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,31,6);
				cout<<"[G]";
				setcolor(hout,WHITE);
				setpos(hout,6,2);
				if(tmpmode==RANDOM_MODE)cout<<"顺序播放";
				else if(tmpmode==ORDER_MODE)cout<<"单曲循环";
				else cout<<"随机播放";
				setcolor(hout,YELLOW);
				setpos(hout,34,6);
				if(tmpmode==RANDOM_MODE)cout<<"单曲循环";
				else if(tmpmode==ORDER_MODE)cout<<"随机播放";
				else cout<<"顺序播放";
				tmpmode=(tmpmode+1)%3;
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('Q')<0) // 返回主菜单
			{
				setcolor(hout,RED);
				setpos(hout,18,7);
				cout<<"[Q]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('Q')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,18,7);
				cout<<"[Q]";
				sprintf(cmd,"stop music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				sprintf(cmd,"close music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				ret=1;
				break;
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('F')<0) // 刷新列表
			{
				setcolor(hout,RED);
				setpos(hout,0,7);
				cout<<"[F]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('F')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,0,7);
				cout<<"[F]";
				ret=2;
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState('T')<0) // 隐藏
			{
				setcolor(hout,RED);
				setpos(hout,9,7);
				cout<<"[T]";
				while(hwnd==GetForegroundWindow()&&GetKeyState('T')<0){};
				setcolor(hout,YELLOW);
				setpos(hout,9,7);
				cout<<"[T]";
				ShowWindow(hwnd,SW_HIDE);
				WaitForSingleObject(mutex,INFINITE);
				ishide=1;
				ReleaseMutex(mutex);
			}
			if(!ishide&&hwnd==GetForegroundWindow()&&GetKeyState(VK_ESCAPE)<0) // 退出
			{
				setcolor(hout,RED);
				setpos(hout,31,7);
				cout<<"[ESC]";
				while(hwnd==GetForegroundWindow()&&GetKeyState(VK_ESCAPE)<0){};
				setcolor(hout,YELLOW);
				setpos(hout,31,7);
				cout<<"[ESC]";
				setpos(hout,0,10);
				sprintf(cmd,"stop music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				sprintf(cmd,"close music%d",playsongno);
				errno=mciSendString(cmd,NULL,0,0);
				ret=0;
				break;
			}
		} // while end
		if(usetmp!=0)
		{
			sprintf(name1,"~$tmp.%s",suffix[usetmp]);
			remove(name1);
		}
		if(lyric.findlyric&&!lyric.context.empty()&&lyricoffset!=0)
		{
			lyric.save(lyricoffset);
		}
		if(ret!=-1)break;
		if(tmpmode!=playmode)
		{
			writecache(playsongno,tmpmode);
			if(playmode==RANDOM_MODE)
			{
				playmode=tmpmode;
				songnolist.clear();
				if(tmpmode!=CIRCLE_MODE)
				{
					if(tonext)playsongno=(playsongno+1)%songnum;
					else
					{
						playsongno--;
						if(playsongno<0)playsongno=songnum-1;
					}
				}
			} // 随机->顺序 或 随机->单曲
			else if(tmpmode==RANDOM_MODE)
			{
				playmode=RANDOM_MODE;
				if(!songnolist.empty())songnolist.clear();
				songnolist.push_back(newsong(playsongno));
				prevsong=NULL;
				cursong=songnolist.begin();
				nextsong=cursong;
				nextsong++;
				playsongno=*cursong;
			} // 顺序->随机 或 单曲->随机
			else if(tmpmode==CIRCLE_MODE)
			{
				playmode=CIRCLE_MODE;
			} // 顺序->单曲
			else
			{
				playmode=ORDER_MODE;
				if(tonext)playsongno=(playsongno+1)%songnum;
				else
				{
					playsongno--;
					if(playsongno<0)playsongno=songnum-1;
				}
			} // 单曲->顺序
		}
		else if(playmode==RANDOM_MODE)
		{
			if(tonext)
			{
				if(nextsong==songnolist.end())
				{
					prevsong=cursong;
					cursong=nextsong;
					songnolist.push_back(newsong(playsongno));
					if(songnolist.size()>1000)songnolist.pop_front();
					cursong--;
				}
				else
				{
					prevsong=cursong;
					cursong=nextsong;
					nextsong++;
				}
			}
			else
			{
				if(prevsong==NULL)
				{
					nextsong=cursong;
					songnolist.push_front(newsong(playsongno));
					if(songnolist.size()>1000)songnolist.pop_back();
					cursong=songnolist.begin();
				}
				else
				{
					nextsong=cursong;
					cursong=prevsong;
					if(prevsong==songnolist.begin())prevsong=NULL;
					else prevsong--;
				}
			}
			playsongno=*cursong;
		} // 一直随机
		else if(playmode==ORDER_MODE)
		{
			if(tonext)playsongno=(playsongno+1)%songnum;
			else
			{
				playsongno--;
				if(playsongno<0)playsongno=songnum-1;
			}
		} // 一直顺序
		setpos(hout,25,1);
		setcolor(hout,BLACK);
		cout<<"                 ";
		clearline(8);
		clearline(9);
		clearline(10);
	}
	return ret;
}

void drawplayer(const int vol,const int mode,const bool isquiet)
{
	const char modestr[3][12]={"随机播放","顺序播放","单曲循环"};
	int i;
	system("cls");
	setpos(hout,0,2);
	setcolor(hout,WHITE);
	cout<<"模式："<<modestr[mode%3];
	setpos(hout,31,2);
	cout<<"音量：";
	if(vol==0)cout<<" 静音";
	else
	{
		for(i=0;i<vol;i++)
			cout<<"|";
		setcolor(hout,GREY);
		for(i=0;i<5-vol;i++)
			cout<<".";
	}
	setpos(hout,0,3);
	setcolor(hout,YELLOW);
	cout<<"[S][Ctrl+↓]暂停  [C]快进      [W]提高音量";
	setpos(hout,0,4);
	cout<<"[D][Ctrl+→]下首  [Z]快退      [X]降低音量";
	setpos(hout,0,5);
	cout<<"[A][Ctrl+←]上首  [V]歌词延后  [E]静音    ";
	setpos(hout,0,6);
	cout<<"[R][Ctrl+↑]\"赞\"  [B]歌词提前  [G]"<<modestr[(mode+1)%3];
	setpos(hout,0,7);
	cout<<"[F]刷新  [T]隐藏  [Q]回主界面  [ESC]退出  ";
	setcolor(hout,WHITE);
}

void clearline(const int linenum)
{
	setcolor(hout,BLACK);
	setpos(hout,0,linenum);
	if(linenum<10)cout<<"                                          ";
	else cout<<"                                         ";
}

void printvolume(const int vol,const bool isquiet)
{
	int i;
	setcolor(hout,WHITE);
	setpos(hout,31,2);
	cout<<"音量：";
	if(isquiet)cout<<" 静音";
	else
	{
		for(i=0;i<vol;i++)
			cout<<"|";
		setcolor(hout,GREY);
		for(i=0;i<5-vol;i++)
			cout<<".";
	}
}

void printtime(const int position,const int length,const bool changelength)
{
	int posix,lenx;
	if(length<=0)
	{
		lenx=-1;
		if(position>=6000000)posix=36;
		else posix=37;
	}
	else if(length>=6000000)
	{
		lenx=36;
		if(position>=6000000)posix=27;
		else posix=28;
	}
	else
	{
		lenx=37;
		if(position>=6000000)posix=28;
		else posix=29;
	}
	if(changelength)
	{
		setpos(hout,25,1);
		setcolor(hout,BLACK);
		cout<<"                 ";
		if(lenx!=-1)
		{
			setcolor(hout,WHITE);
			setpos(hout,lenx-2,1);
			cout<<"/";
			setpos(hout,lenx,1);
			cout<<setfill('0')<<setw(2)<<length/60000<<":"<<setfill('0')<<setw(2)<<length/1000%60;
		}
		setcolor(hout,WHITE);
		setpos(hout,posix,1);
		cout<<setfill('0')<<setw(2)<<position/60000<<":"<<setfill('0')<<setw(2)<<position/1000%60;
	}
	else
	{
		setcolor(hout,WHITE);
		setpos(hout,posix,1);
		cout<<setfill('0')<<setw(2)<<position/60000<<":"<<setfill('0')<<setw(2)<<position/1000%60;
	}
}

void printlyric(const LYRIC &lyric,const int lyricno)
{
	char tmpstr[3][MAXLEN]={'\0','\0','\0'},printstr[3][MAXLEN]={"\0","\0","\0"};
	bool bright[3]={0,1,0};
	int i,len,divpos1=-1,divpos2=-1;
	clearline(8);
	clearline(9);
	clearline(10);
	if(lyricno-1<lyric.context.size()&&lyricno-1>=0)strcpy(tmpstr[0],lyric.context[lyricno-1].c_str());
	if(lyricno<lyric.context.size()&&lyricno>=0)strcpy(tmpstr[1],lyric.context[lyricno].c_str());
	if(lyricno+1<lyric.context.size()&&lyricno+1>=0)strcpy(tmpstr[2],lyric.context[lyricno+1].c_str());
	if(tmpstr[0]=='\0'&&tmpstr[1]=='\0'&&tmpstr[2]=='\0')return;
	len=strlen(tmpstr[1]);
	if(len<=42)
	{
		getlimitstr(tmpstr[0],printstr[0],42,0);
		strcpy(printstr[1],tmpstr[1]);
		getlimitstr(tmpstr[2],printstr[2],41,1);
	}
	else if(len<=80)
	{
		getlimitstr(tmpstr[2],printstr[2],41,1);
		for(i=0;i<len&&i<45; )
		{
			if(i<=42&&len-i<=42)
			{
				if(len/2-i>=-1&&len/2-i<=1&&divpos1==-1)divpos1=i;
				if(tmpstr[1][i]==' ')
				{
					if(divpos2==-1)divpos2=i;
					else if((len/2-i)*(len/2-i)<(len/2-divpos2)*(len/2-divpos2))divpos2=i;
				}
			}
			if(tmpstr[1][i]<0)i+=2;
			else i++;
		}
		if(divpos2!=-1)i=divpos2;
		else i=divpos1;
		strcpy(printstr[1],(tmpstr[1])+i);
		tmpstr[1][i]='\0';
		strcpy(printstr[0],tmpstr[1]);
		bright[0]=1;
	}
	else
	{
		getlimitstr(tmpstr[1],printstr[0],120,1);
		len=strlen(printstr[0]);
		for(i=0;i<len&&i<45; )
		{
			if(i<=42&&len-i<=80)
			{
				if(len/3-i>=-1&&len/3-i<=1&&divpos1==-1)divpos1=i;
				if(tmpstr[1][i]==' ')
				{
					if(divpos2==-1)divpos2=i;
					else if((len/3-i)*(len/3-i)<(len/3-divpos2)*(len/3-divpos2))divpos2=i;
				}
			}
			if(tmpstr[1][i]<0)i+=2;
			else i++;
		}
		if(divpos2!=-1)i=divpos2;
		else i=divpos1;
		strcpy(tmpstr[1],(printstr[0])+i);
		printstr[0][i]='\0';
		len=strlen(tmpstr[1]);
		divpos1=divpos2=-1;
		for(i=0;i<len&&i<45; )
		{
			if(i<=42&&len-i<=42)
			{
				if(len/2-i>=-1&&len/2-i<=1&&divpos1==-1)divpos1=i;
				if(tmpstr[1][i]==' ')
				{
					if(divpos2==-1)divpos2=i;
					else if((len/2-i)*(len/2-i)<(len/2-divpos2)*(len/2-divpos2))divpos2=i;
				}
			}
			if(tmpstr[1][i]<0)i+=2;
			else i++;
		}
		strcpy(printstr[2],(tmpstr[1])+i);
		tmpstr[1][i]='\0';
		strcpy(printstr[1],tmpstr[1]);
		bright[0]=bright[2]=1;
	}
	for(i=0;i<3;i++)
	{
		setpos(hout,(42-strlen(printstr[i]))>>1,i+8);
		if(bright[i])setcolor(hout,WHITE);
		else setcolor(hout,LIGHT_GREY);
		cout<<printstr[i];
	}
}

void writecache(const int songno,const int modeno)
{
	ofstream outfile;
	outfile.open("TKcache");
	outfile<<curlistname<<endl<<songlist[songno].songname<<endl<<modeno<<endl;
	outfile.close();
}

DWORD WINAPI tkplayer(LPVOID lpParam)
{
	int cachet=0,playt;
	char str[MAXLEN];
	bool ret=0;
	cachet=readcache(); // 返回值: -1 冇cache 0 有cache但list唔啱 1 有cache有list但1stsong唔啱 2都冇问题
	if(cachet<=0)ret=stdinlist(str,0);
	while(!ret)
	{
		playt=play(); // 返回值: 0 退出程序 1 重新输入列表 2 刷新
		if(playt==0)break;
		else if(playt==1)
		{
			ret=stdinlist(str,1);
			if(ret)break;
		}
		else if(playt==2)readlist(curlistname);
	}
	Shell_NotifyIcon(NIM_DELETE,&nid);
	exit(0);
}

DWORD WINAPI deletetab(LPVOID lpParam)
{
	while(1)
	{
		Sleep(10);
		if(!ishide)pobj->DeleteTab(hwnd);
	}
	return 0;
}

LRESULT CALLBACK wndproc(HWND hwndr,UINT message,WPARAM wParam,LPARAM lParam)
{
	if(message==WM_IAWENTRAY&&wParam==IDI_ICON1)
	{
		if(lParam==WM_LBUTTONDOWN)
		{
			ShowWindow(hwnd,SW_SHOWNORMAL);
			SetForegroundWindow(hwnd);
			WaitForSingleObject(mutex,INFINITE);
			ishide=0;
			ReleaseMutex(mutex);
			return TRUE;
		}
		return FALSE;
	}
	return DefWindowProc(hwndr,message,wParam,lParam) ;
}

void createinvisiblewindow(WNDCLASS &wndclass)
{
	TCHAR szappname[]=TEXT("tempwnd");
	wndclass.style=CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc=wndproc;
	wndclass.cbClsExtra=wndclass.cbWndExtra=0;
	wndclass.hInstance=hinst;
	wndclass.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wndclass.hCursor=LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName=NULL;
	wndclass.lpszClassName=szappname;
	if(!RegisterClass(&wndclass))
	{
		cerr<<"窗口初始化失败！无法运行程序！";
		exit(0);
	}
	hwndnew=CreateWindow(szappname,TEXT("tempwnd"),WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,hinst,NULL);
	ShowWindow(hwndnew,SW_HIDE);
	UpdateWindow(hwndnew);
}

void setnidinfo()
{
	nid.cbSize=sizeof(NOTIFYICONDATA);
	nid.hWnd=hwndnew;
	nid.uID=IDI_ICON1;
	nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
	nid.uCallbackMessage=WM_IAWENTRAY;
	nid.hIcon=LoadIcon(hinst,MAKEINTRESOURCE(IDI_ICON1));
	_tcscpy(nid.szTip,TEXT("TKplayer"));
}

int main()
{
	MSG msg;
	WNDCLASS wndclass;
	HANDLE delthread,podthread;
	init();
	CoInitialize(0);
	CoCreateInstance(CLSID_TaskbarList,0,CLSCTX_INPROC_SERVER,IID_ITaskbarList,(void **)&pobj);
	mutex=CreateMutex(NULL,false,NULL);
	delthread=CreateThread(NULL,0,deletetab,NULL,0,NULL);
	createinvisiblewindow(wndclass);
	setnidinfo();
	Shell_NotifyIcon(NIM_ADD,&nid);
	podthread=CreateThread(NULL,0,tkplayer,NULL,0,NULL);
	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	CloseHandle(mutex);
	CloseHandle(podthread);
	CloseHandle(delthread);
	return 0;
}
