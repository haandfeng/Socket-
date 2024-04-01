#pragma once
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include<stdbool.h>
#define SPORT 8888
#define err(errMsg) printf("[line:%d]%s failed code %d" ,__LINE__,errMsg, WSAGetLastError());

//包标志位
enum MSGTAG
{
	MSG_FILENAME = 1,
	MSG_FILESIZE = 2,
	MSG_READY_READ = 3,
	MSG_SENDFILE = 4,
	MSG_SUCCESSED = 5,
	MSG_OPENFILE_FAILD = 6,
	ENDSEND = 7,
	//CHAT_REQUSET = 8,
	//CHAT_START = 9,
	CHAT_END = 10,
	CHAT_ING = 11,
	MSG_DIRECTORY = 12,
	MSG_SUCCESS = 13,
	MSG_FAILURE = 14,//目录创造失败
	MSG_DELETEFILE = 15,
	MSG_UPLOAD = 16,//上传文件
	MSG_PATHNAME = 17, //文件路径
	MSG_UPFILENAME = 18, //文件上传第一步
	MSG_UPFILE = 19,  //文件上传第二步
	MSG_UP_READY_READ=20  // 文件上传第三步
};

//包结构
#pragma pack(1)
#define PACKET_SIZE (102400-sizeof(int)*3)//!!!!

struct MsgHeader
{
	enum MSGTAG msgID;//当前消息标记
	union MyUnion
	{
		struct
		{
			char fileName[256];
			int fileSize;
			char filePath[1024];
			int pathSize;
		}fileInfo; //260
		// 目录信息
		struct {
			char dirPath[1024];
		} dirInfo;
		struct
		{
			int nStart;
			int nsize;//该包的数据大小
			char buf[PACKET_SIZE];
		}packet;
		struct
		{
			char chatbuf[PACKET_SIZE];
		}CHAT;
	};
};
#pragma pack()


//初始化socket库
bool initSocket();
//关闭socket库
bool closeSocket();
//监听客户端连接
void connectToHost();
//处理消息
bool processMsg(SOCKET);
//删除文件
void deleteFileName(SOCKET serfd);
//服务器帮助删除
//void deleteFileOnServer(const char* fileName);
void error1(const char* msg);
//获取文件名
void downloadFileName(SOCKET serfd);
//上传文件
void uploadFileName(SOCKET serfd);
//根据返回的文件大小分配内存空间
void readyread(SOCKET serfd, struct MsgHeader*);
//写文件
bool writeFile(SOCKET serfd, struct MsgHeader*);
//聊天室
bool chatroom(SOCKET);

//列举
void listFilesOnServer(SOCKET serfd);
//创建目录
void createDirectory(SOCKET clientSocket);
void handleServerResponse(SOCKET serverSocket);

//多线程发送函数
unsigned __stdcall p_send(void*);
//多线程接受函数
unsigned __stdcall p_recv(void*);
//改变路径
void changeFilePath(SOCKET);
//上传文件sever sendfile复制版
bool upsendFile(SOCKET , struct MsgHeader* );