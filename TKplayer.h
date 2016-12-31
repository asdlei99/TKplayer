#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <string>
#include <vector>
#include <list>
#include <windows.h>
#include <io.h>
#include <tchar.h>
#include <shlguid.h>

#import <shell32.dll>

#pragma comment(lib,"winmm.lib")

#define WM_IAWENTRAY WM_USER+101

#define MAXLEN 1024
#define SUFFIX_NUM 5

#define OFFSET_DEFAULT 10
#define INVALID_LYRICTIME -1989432847

#define RANDOM_MODE 0
#define ORDER_MODE 1
#define CIRCLE_MODE 2

#define BLACK 0
#define WHITE 1
#define YELLOW 2
#define RED 3
#define GREY 4
#define LIGHT_GREY 5

DECLARE_INTERFACE_(ITaskbarList,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid,LPVOID* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(ActivateTab)(HWND) PURE;
	STDMETHOD(AddTab)(HWND) PURE;
	STDMETHOD(DeleteTab)(HWND) PURE;
	STDMETHOD(HrInit)(void) PURE;
};

typedef ITaskbarList *LPITaskbarList;

using namespace std;

class SONG;
class LYRIC;

class SONG
{
public:
	char longname[MAXLEN];
	char songname[256];
	char format[16];
	int preference;
	short suffix;
	SONG()
	{
		longname[0]=songname[0]=format[0]='\0';
		preference=0;
		suffix=-1;
	}
	SONG(const char *str)
	{
		int i,len=strlen(str);
		bool dot=0;
		longname[0]=songname[0]=format[0]='\0';
		preference=0;
		suffix=-1;
		strcpy(longname,str);
		for(i=len-1;i>=0;i--)
		{
			if(str[i]=='.'&&!dot)
			{
				dot=1;
				strcpy(format,str+i+1);
			}
			if(str[i]=='\\')break;
		}
		strcpy(songname,str+i+1);
	}
	void getsongshortname(char *str)
	{
		if(str==NULL)return;
		char tmp[MAXLEN];
		GetShortPathName(longname,tmp,sizeof(tmp)/sizeof(char));
		strcpy(str,tmp);
	}
};

class LYRIC
{
	int gettime(const string str)
	{
		int i,len=str.length(),colonpos=-1,dotpos=-1,minute=0,second=0,misecond=0;
		bool ispositive=1;
		if(str[0]!='['||str[len-1]!=']')return INVALID_LYRICTIME;
		for(i=1;i<len-1;i++)
		{
			if(str[i]!='.'&&str[i]!=':'&&str[i]!='-'&&!(str[i]>='0'&&str[i]<='9'))return INVALID_LYRICTIME;
			if(str[i]=='-')ispositive=0;
			if(str[i]==':')
			{
				if(colonpos!=-1)return INVALID_LYRICTIME;
				else colonpos=i;
			}
			else if(str[i]=='.')
			{
				if(dotpos!=-1)return INVALID_LYRICTIME;
				else dotpos=i;
			}
		}
		if(colonpos==-1)return INVALID_LYRICTIME;
		for(i=1;i<colonpos;i++)
			if(str[i]!='-')minute=minute*10+str[i]-'0';
		if(dotpos==-1)
		{
			for(i=colonpos+1;i<len-1;i++)
				if(str[i]!='-')second=second*10+str[i]-'0';
		}
		else
		{
			for(i=colonpos+1;i<dotpos;i++)
				if(str[i]!='-')second=second*10+str[i]-'0';
			if(str[dotpos+1]!=']'&&str[dotpos+1]!='\0')misecond+=(str[dotpos+1]-'0')*100;
			else return INVALID_LYRICTIME;
			if(str[dotpos+2]!=']'&&str[dotpos+1]!='\0')misecond+=(str[dotpos+2]-'0')*10;
			if(str[dotpos+3]!=']'&&str[dotpos+1]!='\0')misecond+=(str[dotpos+3]-'0');
		}
		return (int(ispositive)*2-1)*(minute*60000+second*1000+misecond);
	}
public:
	vector<int> displaytime;
	vector<string> context;
	char lyricfilename[256];
	bool findlyric;
	LYRIC()
	{
		findlyric=0;
	}
	void getlyric(const char *lyricfile)
	{
		ifstream infile;
		string st,con;
		char str[MAXLEN]="Lyrics\\";
		int i,j,k,size,lastrightbrac,rettime;
		findlyric=0;
		context.clear();
		displaytime.clear();
		if(lyricfile==NULL||strlen(lyricfile)==0)return;
		strcat(str,lyricfile);
		infile.open(str);
		if(infile==NULL)return;
		findlyric=1;
		strcpy(lyricfilename,lyricfile);
		while(infile.getline(str,MAXLEN))
		{
			if(strlen(str)==0)continue;
			st=str;
			lastrightbrac=-1;
			for(i=0;i<strlen(str);i++)
				if(str[i]==']')lastrightbrac=i;
			if(lastrightbrac!=-1)
			{
				con=st.substr(lastrightbrac+1,strlen(str)-lastrightbrac);
				j=k=0;
				for(i=0;i<=lastrightbrac;i++)
				{
					if(str[i]=='[')j=i;
					if(str[i]==']')
					{
						k=i;
						rettime=gettime(st.substr(j,k-j+1));
						if(rettime!=INVALID_LYRICTIME)
						{
							displaytime.push_back(rettime);
							context.push_back(con);
						}
					}
				}
			}
		}
		infile.close();
		size=context.size();
		for(i=1;i<size;i++)
			for(j=i;j>0;j--)
				if(displaytime[j]<displaytime[j-1])
				{
					swap(displaytime[j],displaytime[j-1]);
					swap(context[j],context[j-1]);
				}
				else break;
	}
	void save(const int offset)
	{
		ifstream infile;
		ofstream outfile;
		string st;
		char str[MAXLEN]="Lyrics\\",tempfile[32]="Lyrics\\~$tmp.lrc",inputstr[MAXLEN];
		int i,j,k,rettime;
		bool ispositive;
		if(!findlyric)return;
		strcat(str,lyricfilename);
		infile.open(str);
		if(infile==NULL)return;
		outfile.open(tempfile);
		while(infile.getline(inputstr,MAXLEN))
		{
			if(strlen(inputstr)==0)
			{
				outfile<<endl;
				continue;
			}
			st=inputstr;
			j=k=-1;
			for(i=0;i<strlen(inputstr);i++)
			{
				if(inputstr[i]=='[')
				{
					j=i;
				}
				else if(inputstr[i]==']')
				{
					k=i;
					if(j!=-1)
					{
						rettime=gettime(st.substr(j,k-j+1));
						if(rettime==INVALID_LYRICTIME)outfile<<st.substr(j,k-j+1);
						else
						{
							rettime+=offset;
							ispositive=rettime>=0;
							if(rettime<0)rettime=-rettime;
							int minute=rettime/60000,second=rettime/1000%60,misecond=rettime/10%100;
							if(ispositive)
							{
								outfile<<"["<<setfill('0')<<setw(2)<<minute<<":"<<setfill('0')<<setw(2)<<second<<"."<<setfill('0')<<setw(2)<<misecond<<"]";
							}
							else
							{
								outfile<<"[";
								if(minute>0)outfile<<"-"<<minute<<":";
								else outfile<<"00:";
								outfile<<"-"<<second<<"."<<setfill('0')<<setw(2)<<misecond<<"]";
							}
						}
					}
				}
			}
			outfile<<st.substr(k+1,strlen(inputstr)-k)<<endl;
		}
		infile.close();
		outfile.close();
		remove(str);
		rename(tempfile,str);
	}
	int getlyricno(const int curtime,const int offset)
	{
		int left,right,mid;
		if(displaytime.empty())return -2;
		else if(curtime-offset<displaytime[0])return -1;
		else if(curtime-offset>=displaytime.back())return displaytime.size()-1;
		left=0;
		right=displaytime.size()-2;
		mid=(left+right)>>1;
		while(left<right)
		{
			if(curtime-offset>=displaytime[mid]&&curtime-offset<displaytime[mid+1])return mid;
			else if(curtime-offset<displaytime[mid])right=mid-1;
			else left=mid+1;
			mid=(left+right)>>1;
		}
		return left;
	}
};

int newstrcmp(const string &str1,const string &str2);
void setcolor(const HANDLE hout,const int color);
void getlimitstr(const char *oldstr,char *newstr,const int remainlen,const bool truncback);
int getlengthfromshell(const char *filename);
bool getallfile(const char *path,const char *tmpfilename);

int newstrcmp(const string &str1,const string &str2)
{
	int l1=str1.length(),l2=str2.length(),i;
	if(l1!=l2)return l1-l2;
	for(i=l1-1;i>=0;i--)
	{
		char c1=str1[i],c2=str2[i];
		if(c1>='A'&&c1<='Z')c1+='a'-'A';
		if(c2>='A'&&c2<='Z')c2+='a'-'A';
		if(c1!=c2)return c1-c2;
	}
	return 0;
}

void setcolor(const HANDLE hout,const int color)
{
	if(color==BLACK)SetConsoleTextAttribute(hout,0);
	else if(color==WHITE)SetConsoleTextAttribute(hout,FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);
	else if(color==YELLOW)SetConsoleTextAttribute(hout,FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED);
	else if(color==RED)SetConsoleTextAttribute(hout,FOREGROUND_INTENSITY|FOREGROUND_RED);
	else if(color==GREY)SetConsoleTextAttribute(hout,FOREGROUND_INTENSITY);
	else if(color==LIGHT_GREY)SetConsoleTextAttribute(hout,FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);
}

void setpos(const HANDLE hout,const int x,const int y)
{
	static COORD coord;
	coord.X=x;
	coord.Y=y;
	SetConsoleCursorPosition(hout,coord);
}

void getlimitstr(const char *oldstr,char *newstr,const int remainlen,const bool truncback)
{
	int i,len=strlen(oldstr);
	strcpy(newstr,oldstr);
	if(strlen(oldstr)<=remainlen)return;
	if(truncback)
	{
		for(i=0;i<len; )
		{
			if(remainlen-i<=3||(remainlen-i==4&&newstr[i]<0))
			{
				newstr[i]=newstr[i+1]=newstr[i+2]='.';
				newstr[i+3]='\0';
				return;
			}
			if(newstr[i]<0)i+=2;
			else i++;
		}
	}
	else
	{
		for(i=0;i<len; )
		{
			if(len-i+3<=remainlen)
			{
				char tmp[MAXLEN]="...";
				strcat(tmp,newstr+i);
				strcpy(newstr,tmp);
				return;
			}
			if(newstr[i]<0)i+=2;
			else i++;
		}
	}
}

int getlengthfromshell(const char *filename)
{
	int i,pos=-1,hour,minute,second;
	char path[MAXLEN],file[MAXLEN],c;
	stringstream ss;
	if(filename==NULL||filename[0]=='\0')return -1;
	for(i=strlen(filename)-1;i>=0;i--)
		if(filename[i]=='\\')
		{
			pos=i;
			break;
		}
	if(pos==-1)return -1;
	for(i=0;i<=pos;i++)
		path[i]=filename[i];
	path[pos+1]='\0';
	strcpy(file,filename+pos+1);
	Shell32::IShellDispatchPtr ptrshell;
    ptrshell.CreateInstance(__uuidof(Shell32::Shell));
    _variant_t var((short)Shell32::ssfRECENT);
	Shell32::FolderPtr ptrfolder=ptrshell->NameSpace(path);
	if(ptrfolder==NULL)return -1;
	Shell32::FolderItemPtr ptritem=ptrfolder->ParseName(file);
	if(ptritem==NULL)return -1;
	string strval=ptrfolder->GetDetailsOf(_variant_t((IDispatch *)ptritem),21);
	if(strval.length()==0)return -1;
	ss<<strval;
	ss>>hour>>c>>minute>>c>>second;
	return (hour*3600+minute*60+second+1)*1000;
}

bool getallfile(const char *path,const char *tmpfilename)
{
	const char suffix[SUFFIX_NUM][7]={"mp3","wma","wav","mid","midi"};
	_finddata_t fileinfo;
	ofstream outfile;
	char dir[MAXLEN],dir2[MAXLEN];
	long handle;
	int i,j;
	bool existvalidfile=0;
	if(path==NULL||tmpfilename==NULL||path[0]=='\0'||tmpfilename[0]=='\0')return 0;
	for(i=strlen(path)-1;i>=0;i--)
		if(path[i]=='\\')break;
	if(i<0)return 0;
	strcpy(dir,path);
	dir[i+1]='\0';
	strcpy(dir2,dir);
	dir[i+1]='*';
	dir[i+2]='\0';
	handle=_findfirst(dir,&fileinfo);
	if(handle==-1)return 0;
	do
	{
		if(!(fileinfo.attrib&_A_SUBDIR))
		{
			for(i=strlen(fileinfo.name);i>=0;i--)
				if(fileinfo.name[i]=='.')break;
			if(i>0)
			{
				for(j=0;j<SUFFIX_NUM;j++)
					if(!stricmp(suffix[j],fileinfo.name+i+1))break;
				if(j!=SUFFIX_NUM)
				{
					if(!existvalidfile)
					{
						outfile.open(tmpfilename);
						existvalidfile=1;
					}
					outfile<<dir2<<fileinfo.name<<endl;
				}
			}
		}
	}
	while(_findnext(handle,&fileinfo)==0);
	_findclose(handle);
	if(existvalidfile)outfile.close();
	return existvalidfile;
}
