#pragma once
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include<stdbool.h>
#define SPORT 8888
#define err(errMsg) printf("[line:%d]%s failed code %d" ,__LINE__,errMsg, WSAGetLastError());

enum MSGTAG
{
	MSG_FILENAME = 1,
	MSG_FILESIZE = 2,
	MSG_READY_READ = 3,
	MSG_SENDFILE = 4,
	MSG_SUCCESSED = 5,
	MSG_OPENFILE_FAILD = 6,//���߿ͻ����ļ��Ҳ���
	ENDSEND = 7,
	//CHAT_REQUSET = 8,//��������
	//CHAT_START = 9,//���쿪ʼ�����߿ͻ��˿��Խ���������
	CHAT_END = 10,//�ر�������
	CHAT_ING = 11,//�������ʱ
	MSG_DIRECTORY = 12,
	MSG_SUCCESS = 13,
	MSG_FAILURE = 14,//Ŀ¼����ʧ��
	MSG_DELETEFILE = 15,
	MSG_UPLOAD = 16,//�ϴ��ļ�
	MSG_PATHNAME = 17, //�ı�·��
	MSG_UPFILENAME = 18, //�ļ��ϴ���һ��
	MSG_UPFILE = 19,  //�ļ��ϴ��ڶ���
	MSG_UP_READY_READ = 20  // �ļ��ϴ�������
};


#pragma pack(1)//���ýṹ���ֽڶ���
#define PACKET_SIZE (10240 -sizeof(int)*3)//������
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
			char chatbuf[PACKET_SIZE];//������Ϣ����
		}CHAT;
	};
};
#pragma pack()


//��ʼ��socket��
bool initSocket();
//�ر�socket��
bool closeSocket();
//�����ͻ�������
void listenToClient();
//������Ϣ
bool processMsg(SOCKET);
//��ȡ�ļ�������ļ���С
bool readFile(SOCKET, struct MsgHeader*);
//�����ļ�
bool sendFile(SOCKET, struct MsgHeader*);
//�����ļ�
void receiveFile(SOCKET clifd);
//�ڷ�����ɾ���ļ�
void deleteFileOnServer(SOCKET,const char* fileName);
//����������Ŀ¼
int createDirectory(SOCKET,const char* directoryPath);
void handleClientRequest(int clientSocket);
//���̷߳��ͺ���
unsigned __stdcall p_send(void*);
//���߳̽��ܺ���
unsigned __stdcall p_recv(void*);
// �ı��ļ�·��
bool changePath(SOCKET, struct MsgHeader*);
// �ϴ�д�ļ��汾
bool upwriteFile(SOCKET, struct MsgHeader*);