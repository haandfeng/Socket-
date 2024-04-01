#pragma once
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include<stdbool.h>
#define SPORT 8888
#define err(errMsg) printf("[line:%d]%s failed code %d" ,__LINE__,errMsg, WSAGetLastError());

//����־λ
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
	MSG_FAILURE = 14,//Ŀ¼����ʧ��
	MSG_DELETEFILE = 15,
	MSG_UPLOAD = 16,//�ϴ��ļ�
	MSG_PATHNAME = 17, //�ļ�·��
	MSG_UPFILENAME = 18, //�ļ��ϴ���һ��
	MSG_UPFILE = 19,  //�ļ��ϴ��ڶ���
	MSG_UP_READY_READ=20  // �ļ��ϴ�������
};

//���ṹ
#pragma pack(1)
#define PACKET_SIZE (102400-sizeof(int)*3)//!!!!

struct MsgHeader
{
	enum MSGTAG msgID;//��ǰ��Ϣ���
	union MyUnion
	{
		struct
		{
			char fileName[256];
			int fileSize;
			char filePath[1024];
			int pathSize;
		}fileInfo; //260
		// Ŀ¼��Ϣ
		struct {
			char dirPath[1024];
		} dirInfo;
		struct
		{
			int nStart;
			int nsize;//�ð������ݴ�С
			char buf[PACKET_SIZE];
		}packet;
		struct
		{
			char chatbuf[PACKET_SIZE];
		}CHAT;
	};
};
#pragma pack()


//��ʼ��socket��
bool initSocket();
//�ر�socket��
bool closeSocket();
//�����ͻ�������
void connectToHost();
//������Ϣ
bool processMsg(SOCKET);
//ɾ���ļ�
void deleteFileName(SOCKET serfd);
//����������ɾ��
//void deleteFileOnServer(const char* fileName);
void error1(const char* msg);
//��ȡ�ļ���
void downloadFileName(SOCKET serfd);
//�ϴ��ļ�
void uploadFileName(SOCKET serfd);
//���ݷ��ص��ļ���С�����ڴ�ռ�
void readyread(SOCKET serfd, struct MsgHeader*);
//д�ļ�
bool writeFile(SOCKET serfd, struct MsgHeader*);
//������
bool chatroom(SOCKET);

//�о�
void listFilesOnServer(SOCKET serfd);
//����Ŀ¼
void createDirectory(SOCKET clientSocket);
void handleServerResponse(SOCKET serverSocket);

//���̷߳��ͺ���
unsigned __stdcall p_send(void*);
//���߳̽��ܺ���
unsigned __stdcall p_recv(void*);
//�ı�·��
void changeFilePath(SOCKET);
//�ϴ��ļ�sever sendfile���ư�
bool upsendFile(SOCKET , struct MsgHeader* );