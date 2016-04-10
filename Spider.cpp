#include<string>
#include<iostream>
#include<fstream>
#include<vector>
#include<time.h>
#include<queue>
#include<hash_set>
#include<regex>
#include "winsock2.h"
#define  GET_SIZE 1024000;
#define  KEYWD "邮箱"
#define  KEYPATH "tieba.baidu.com"
#pragma comment(lib,"ws2_32.lib")
using namespace std;

queue<string> hrefUrl;
hash_set<string> visitedUrl;
hash_set<string> visitedMail;
int depth=1;
int g_ImgCnt=1;

bool Get_Host_Resource(string url,string & host,string & resource)
{
	if(url.size()>2000||url.size()==0)
		return false;
	const char * pos = strstr(url.c_str(),"http://");

	if (pos!=NULL)
		pos+=strlen("http://");
	else
		pos = url.c_str();
	if (strstr(pos,"/")==NULL)
		return false;
	char host_tmp[10240] ={0};
	char resource_tmp[10240]={0};
	sscanf(pos,"%[^/]%s",host_tmp,resource_tmp);
	host = host_tmp;
	resource = resource_tmp;
	return true;
}

bool Get_Http_Response(const string &url,char * &response,int & byteread)
{
	string host,resource;
	if (!Get_Host_Resource(url,host,resource))
	{
		cout<<"Get_Host_Resource err\n";
		return false;
	}
	else
	{
		cout<<host<<"  "<<resource<<endl;
	}
	struct hostent	* host_peer = gethostbyname(host.c_str());//获取主机对应的信息
	if (host_peer==NULL)
	{
		cout<<"get host info err\n";
		return false;
	}
	SOCKET sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);//
	if(sock<0)
	{
		cout<<"create sock err\n";
		return false;
	}
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr,host_peer->h_addr,4);
	int TimeOut = 2000;
	if(setsockopt( sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&TimeOut, sizeof(int)));
	if (connect(sock,(SOCKADDR*)&sa,sizeof(sa))!=0)
	{
			cout<<"connect err "<<url<<endl;
			closesocket(sock);
			return false;
	} 
	string send_str = "GET "+resource+" HTTP/1.1\r\nHost:"+host+"\r\nConnection:Close\r\n\r\n";
	if (send(sock,send_str.c_str(),send_str.size(),0)<0)
	{
		cout<<"send err \n";
		closesocket(sock);
		return false;
	}
	int getnum = GET_SIZE;
	char * get_buf =(char *) malloc(getnum);
	if (get_buf)
	{
		memset(get_buf,0,getnum);
	}
	else
	{
		cout<<"malloc err\n";
		closesocket(sock);
		return false;
	}
	int ret = 1;

	while (ret>0)
	{
		ret=recv(sock,get_buf+byteread,getnum-byteread,0);
		if(ret>0)
			byteread+=ret;
		else
			break;
		if (getnum-byteread<10000)
			break;
	}
	response = get_buf;
	closesocket(sock);
	return true;
}
void Get_All_Url(char * data,string url,const char * url_key)
{
	char * p = data;
	string tmp;
	char tmp_url[10240]={0};
	string host,resource;
	if(!Get_Host_Resource(url,host,resource))
		return;
	char * tag = "href=\"";
	const char * pos = strstr(p,tag);
	ofstream ofile("url.txt",ios::app);
	while(pos)
	{
		pos+=strlen(tag);
		if (strstr(pos,"\"")-pos>10000)
		{
			cout<<"url err"<<endl;
			pos+=10000;
			continue;
		}
		sscanf(pos,"%[^\"]",tmp_url);
		if(strlen(tmp_url)>10000)
		{
			cout<<"err tmp_url"<<endl;
			exit(1);
		}
		if (*tmp_url=='/')
		{

			tmp = host;
			string tmp2=tmp_url;
			tmp+=tmp2;
		}
		else
			tmp = tmp_url;
		if (url_key!=NULL)
		{
			if (tmp.find(url_key)==tmp.npos)
			{
				//cout<<"not tieba url "<<tmp<<endl;
				pos=strstr(pos,tag);
				continue;
			}
		}
		else
		if (tmp.find(KEYPATH)==tmp.npos)
		{
			//cout<<"not tieba url "<<tmp<<endl;
			pos=strstr(pos,tag);
			continue;
		}
		if (visitedUrl.find(tmp)==visitedUrl.end())
		{
			visitedUrl.insert(tmp);
			ofile<<tmp<<endl;
			hrefUrl.push(tmp);
		}
		pos=strstr(pos,tag);
	}
	ofile << endl << endl;
	ofile.close();

	return;

}

void Find_Key(string url,const char * data,const char * key)
{
	if (data==NULL||key==NULL)
	{
		return;
	}
	else
	{
		if (strstr(data,key)!=NULL)
		{
			string tmp = key;
			tmp+="-keyurl.txt";
			ofstream keyf(tmp,ios::app);
			keyf<<url<<endl;
			keyf.close();
		}
	}
	return;
}

string To_File_Name(string url)
{
	string fileName;
	fileName.resize( url.size());
	int k=0;
	for( int i=0; i<(int)url.size(); i++){
		char ch = url[i];
		if( ch!='\\'&&ch!='/'&&ch!=':'&&ch!='*'&&ch!='?'&&ch!='"'&&ch!='<'&&ch!='>'&&ch!='|')
			fileName[k++]=ch;
		else
			fileName[k++]='_';
	}
	string res = fileName.substr(0,k-1) + ".txt";
	return res;
}

void Get_Mail_Addr(const char * data,const char * savefname)
{
	string buf = data;
	const char * p = data;
	ofstream mfile;
	if (savefname!=NULL)
		mfile.open(savefname,ios::app);
	else
		mfile.open("mail.txt",ios::app);
	regex pattern("([0-9A-Za-z_.]+)@([0-9a-z]+\\.[a-z]{2,3}(\\.[a-z]{2})?)");
	cmatch results;
	while(regex_search(p,results,pattern))
	{
		string tmp =results[0];
		if (visitedMail.find(tmp)==visitedMail.end())
		{
			visitedMail.insert(tmp);
			mfile<<results[0]<<endl;
			
		}
		
		p=results[0].second;

	}
	//mfile << endl << endl;
	mfile.close();
	return;	
}
void Tieba_Get_Mail(const string & url,int & endnum)
{
	char * res = NULL;
	int bytenum = 0;
	char tmp_cout[10000]={0};
	sprintf(tmp_cout,"?pn=%d",depth);

	string tmp = tmp_cout;
	tmp=url+tmp;

	if(!Get_Http_Response(tmp,res,bytenum))
	{
		cout<<"url is err"<<url<<endl;
		return;
	}
	string sfname = To_File_Name(url);
	Get_Mail_Addr(res,sfname.c_str());
	
	Get_All_Url(res,url,url.c_str());
//	Find_Key(url,res,KEYWD);

	free(res);
	if(depth==endnum)
	{
		cout<<"邮箱地址获取完成！\n文件名："<<sfname<<endl;;
		return;
	}
	depth++;
	Tieba_Get_Mail(url,endnum);
	return;
}
void BFS(const string & url,string str)
{
	char * res = NULL;
	int bytenum = 0;
	if(!Get_Http_Response(url,res,bytenum))
	{
		cout<<"url is err"<<url<<endl;
		return;
	}
	Get_Mail_Addr(res,str.c_str());
	Get_All_Url(res,url,NULL);
	Find_Key(url,res,KEYWD);
	free(res);
}

void main(int argc,char **argv)
{
	// Get_Mail_Addr("qwe q1.com 11@qq.commmmm 22@qq.com21");
	WSADATA wsaData={0};
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 ){
		return;
	}
	cout<<"1,获取帖子邮箱\n2,爬行整个贴吧邮箱\n";

	int choose = 0;
	string surl,k;
	int endnum = 2;
	int beginnum = 1;
	cin>>choose;
	switch(choose)
	{
	case 1:
		{
			cout<<"input 百度贴吧 url 例如\"http://tieba.baidu.com/p/650127348\" 贴纸首页地址"<<endl;
			if(cin>>surl);
			else
				surl ="http://tieba.baidu.com/p/650127348";
			if(surl.find("?pn=")!=surl.npos)
			{
				surl= surl.substr(0,surl.find("?pn="));
			}
			cout<<"input begin num \n";
			if(cin>>beginnum)
				depth=beginnum;
			cout<<"input end num \n";
			cin>>endnum;
			visitedUrl.insert(surl);
			Tieba_Get_Mail(surl,endnum);
		
			break;
		}
	case 2:
		{
			cout<<"input 贴吧开始地址\n";
			string surl = "http://tieba.baidu.com/f/search/res?ie=utf-8&qw=资源";
			if(cin>>surl);
			else
				surl = "http://tieba.baidu.com/f/search/res?ie=utf-8&qw=资源";
			visitedUrl.insert(surl);
			time_t t;
			time(&t);
			struct tm*now = localtime(&t);
			string str=asctime(now);
			string str2 = To_File_Name(str);
			//	str = str.replace(" ","");
			cout <<"开始爬行获取邮箱地址\n文件名："<<str2<<endl;

			BFS(surl,str2);
			while(hrefUrl.size()>0)
			{
				string tmp = hrefUrl.front();
				BFS(tmp,str2);
				cout<<tmp<<endl;
				visitedUrl.insert(tmp);
				hrefUrl.pop();
			}
			break;
		}
	default:
		break;
	}
	
	 WSACleanup();
	system("pause");
}