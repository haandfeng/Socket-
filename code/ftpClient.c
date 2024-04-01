#include <stdio.h>
#include"ftpClient.h"
#include"string.h"
#include <windows.h>
#include <tchar.h>
#pragma warning(disable : 4996)
char g_recvBuf[10240];       //接受消息的缓冲区///!!!!!!
char* g_fileBuf;        //存储文件内容
int g_fileSize2;         //收到包文件大小
char g_fileName[256];        //保存服务器发送过来的文件名

int main()
{
	initSocket();
	connectToHost();
	closeSocket();
	return 0;
}
//初始化socket库
bool initSocket()
{
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		err("WSAStartup");
		return FALSE;
	}
	return TRUE;

}
//关闭socket库
bool closeSocket()
{
	if (0 != WSACleanup())//WSACleanup（）中止了Windows Sockets在所有线程上的操作.
	{
		err("WSACleaup");
		return FALSE;
	}
	return TRUE;

}
//监听客户端连接
void connectToHost()
{
	//创建server socket套接字 地址，端口号
	//INVALID_SOKET SOCKET_ERROR
	SOCKET  serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		printf("socket faild :%d\n", WSAGetLastError());
		return;
	}
	//给socket绑定ip和端口号
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;//必须和创建的socket指定的一样
	serAddr.sin_port = htons(SPORT);
	serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//监听本机所有网卡
	//链接到服务器
	if (0 != connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		printf("connnect faild :%d\n", WSAGetLastError());
		return;
	}
	printf("conect success!");

	//开始处理消息,选择聊天室功能还是文件传输功能：
	char ch;
	while (true) {
		printf("chat or sendfile?\nif chat press a,if sendfile press b，else you will exit this systm!\n");
		printf("***********************************\n");
		printf("     a: Chat\n");
		printf("     b: Doadload file\n");
		printf("     c: Exit this window\n");
		printf("     d: Delete the file\n");
		printf("     e: Change the path\n");
		printf("     f: Show the files\n");
		printf("     g: Make a directory\n");
		printf("     h: Upload file\n");
		printf("***********************************\n");
		printf("Enter your choice: ");
		//如果输入信息不正确，继续输入
		do {
			ch = (char)_getch();
		} while (ch != 'a' && ch != 'b' && ch != 'c' && ch != 'd' && ch != 'e' && ch != 'f' && ch != 'g' && ch != 'h');

		system("cls"); //清除控制台显示的信息

		//进入聊天室
		if (ch == 'a')
		{
			printf("welcome to yy's chatroom!\n");
			chatroom(serfd);
			break;
		}
		//进行文件传输
		if (ch == 'b') {
			printf("welcome to download file!\n");
			//获取要发送文件名称
			downloadFileName(serfd);//先发送文件名给服务器
			while (processMsg(serfd)) {}
		}
		//退出系统
		if (ch == 'c') {
			printf("see you next time!\n");
			closesocket(serfd);
			return false;
		}
		//删除文件
		if (ch == 'd') {
			deleteFileName(serfd);
			while (processMsg(serfd)) {}
		}
		// 改变路径
		if (ch == 'e') {
			changeFilePath(serfd);
			while (processMsg(serfd)) {}
		}
		//列举
		if (ch == 'f') {
			listFilesOnServer(serfd);
		}
		//创造目录
		if (ch == 'g') {
			createDirectory(serfd);
		}
		//上传文件
		if (ch == 'h') {
			uploadFileName(serfd);
			while (processMsg(serfd)) {}
		}

		printf("\nPress Any Key To Continue:");
		_getch();
		system("cls");
	}
	return true;
}



//处理消息
bool processMsg(SOCKET serfd)
{
	recv(serfd, g_recvBuf, 10240, 0);
	struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;//将接受到的信息强转为包格式
	switch (msg->msgID)
	{
	case MSG_OPENFILE_FAILD:
		printf("send file failed ,please try again\n");
		downloadFileName(serfd);
		break;
	case MSG_FILESIZE:
		readyread(serfd, msg);
		break;
	case MSG_UPFILE:
		upsendFile(serfd, msg);
		break;
	case MSG_READY_READ:
		writeFile(serfd, msg);
		break;
	case MSG_SUCCESSED:
		printf("send file success!\n");
		//进入选项窗口
		char key;
		while (true)
		{
			printf("if you want to continue to download,print 1,if you want to chat,print 2,else press another key:\n ");
			printf("***********************************\n");
			printf("     1: Chat\n");
			printf("     2: Doadload file\n");
			printf("     3: Exit this window\n");
			printf("     4: Delete the file\n");
			printf("     5: Change the path\n");
			printf("     6: Show the files\n");
			printf("     7: Make a directory\n");
			printf("     8: Upload file\n");
			printf("***********************************\n");
			printf("Enter your choice (1 or 2 or 3 or others): ");
			do {
				key = (char)_getch();
			} while (key != '1' && key != '2' && key != '3' && key != '4' && key != '5' && key != '6' && key != '7' && key != '8');

			system("cls"); //清除控制台显示的信息
			if (key == '1') {
				downloadFileName(serfd);
				while (processMsg(serfd)) {}
			}
			if (key == '2') {
				struct MsgHeader Chat;
				Chat.msgID = CHAT_ING;
				printf("welcome to yy's chatroom!\n");
				chatroom(serfd);
				break;
			}

			if (key == '3') {
				struct MsgHeader file;
				file.msgID = ENDSEND;
				if (SOCKET_ERROR == send(serfd, (char*)&file, sizeof(struct MsgHeader), 0))
				{
					err("send");
					return;
				}
				printf("关闭系统，欢迎下次再见~\n");
				closesocket(serfd);
				return false;
			}
			if (key == '4') {
				deleteFileName(serfd);
				while (processMsg(serfd)) {}
			}
			// 路径
			if (key == '5') {
				changeFilePath(serfd);
				while (processMsg(serfd)) {}
			}
			//列举
			if (key == '6') {
				listFilesOnServer(serfd);
			}
			//创造目录
			if (key == '7') {
				createDirectory(serfd);
			}
			//上传文件
			if (key == '8') {
				uploadFileName(serfd);
			}
		}
		break;
	}
	return TRUE;
}

//struct MsgHeader {
//	int msgID;
//	struct {
//		char fileName[1024];
//	} fileInfo;
//};

void error1(const char* msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}



void deleteFileName(SOCKET serfd) {
	char fileName[1024];
	printf("请输入要删除的文件名：");
	gets_s(fileName, 1024);

	struct MsgHeader file;
	file.msgID = MSG_DELETEFILE;
	strcpy(file.fileInfo.fileName, fileName);

	if (SOCKET_ERROR == send(serfd, (char*)&file, sizeof(struct MsgHeader), 0)) {
		error1("send");
		return;
	}

	// 接收服务器的回应或处理结果，根据实际情况进行处理

	// 在服务器当前文件夹下删除文件
	//deleteFileOnServer(fileName);
}


// 客户端接收服务器回复的函数
void handleServerResponse(SOCKET serverSocket) {
	struct MsgHeader response;
	if (recv(serverSocket, (char*)&response, sizeof(struct MsgHeader), 0) == SOCKET_ERROR) {
		perror("recv");
		return;
	}

	if (response.msgID == MSG_SUCCESS) {
		printf("目录创建成功\n");
	}
	else if (response.msgID == MSG_FAILURE) {
		printf("目录创建失败\n");
	}
}
//客户端发送创建目录请求的函数
void createDirectory(SOCKET clientSocket) {
	char directoryPath[1024];
	printf("请输入要创建的目录路径：");
	fgets(directoryPath, sizeof(directoryPath), stdin);

	// 移除输入字符串中的换行符
	size_t len = strlen(directoryPath);
	if (len > 0 && directoryPath[len - 1] == '\n') {
		directoryPath[len - 1] = '\0';
	}

	struct MsgHeader directory;
	directory.msgID = MSG_DIRECTORY;
	strcpy(directory.dirInfo.dirPath, directoryPath);

	if (send(clientSocket, (char*)&directory, sizeof(struct MsgHeader), 0) == SOCKET_ERROR) {
		perror("send");
		return;
	}

	// 等待服务器回复
	/*handleServerResponse(clientSocket);*/
}
// 改变路径
void changeFilePath(SOCKET serfd) {
	char pathName[1024];
	printf("请输入要切换的路径：");
	gets_s(pathName, 1024);
	//strncpy(uploadpath, pathname, sizeof(uploadpath) - 1);
	//uploadpath[sizeof(uploadpath) - 1] = '\0';  // ensure null-terminatio
	//"C:\srp\main.py"
	struct MsgHeader path;
	path.msgID = MSG_PATHNAME;
	strcpy(path.fileInfo.filePath, pathName);

	path.fileInfo.pathSize = strlen(pathName);
	if (SOCKET_ERROR == send(serfd, (char*)&path, sizeof(struct MsgHeader), 0))
	{
		err("send");
		return;
	}
}


//下载文件名	
void downloadFileName(SOCKET serfd)
{
	char fileName[1024];
	printf("请输入要下载的文件名：");
	gets_s(fileName, 1024);
	struct MsgHeader file;
	file.msgID = MSG_FILENAME;
	strcpy(file.fileInfo.fileName, fileName);
	if (SOCKET_ERROR == send(serfd, (char*)&file, sizeof(struct MsgHeader), 0))
	{
		err("send");
		return;
	}
}

//上传文件	
void uploadFileName(SOCKET serfd)
{
	char fileName[1024];
	printf("请输入要上传的文件名：");
	gets_s(fileName, 1024);
	struct MsgHeader file;
	file.msgID = MSG_UPFILENAME;
	strcpy(file.fileInfo.fileName, fileName);
	FILE* pread = fopen(fileName, "rb");
	//找不到就告诉客户端发送失败
	if (pread == NULL)
	{
		printf("找不到[%s]文件..\n", fileName);

	}
	//找到文件后，获取文件大小
	fseek(pread, 0, SEEK_END);
	g_fileSize2 = ftell(pread);//函数 ftell 用于得到文件位置指针当前位置相对于文件首的偏移字节数
	//printf("%d", g_fileSize2);
	fseek(pread, 0, SEEK_SET);//读指针移到文件开始地方，准备开始读
	//把文件大小发给客户端
	file.fileInfo.fileSize = g_fileSize2;
	char tfname[200] = { 0 }, text[100];
	_splitpath(fileName, NULL, NULL, tfname, text);//获取文件的名字和属性
	strcat(tfname, text);
	//把文件名发给客户端
	strcpy(file.fileInfo.fileName, tfname);

	//读取文件内容
	//分配内存空间
	g_fileBuf = calloc(g_fileSize2 + 1, sizeof(char));
	if (g_fileBuf == NULL)
	{
		printf("内存不足，请重试\n");
		return false;
	}
	fread(g_fileBuf, sizeof(char), g_fileSize2, pread);
	g_fileBuf[g_fileSize2] = '\0';
	fclose(pread);


	send(serfd, (char*)&file, sizeof(struct MsgHeader), 0);


}
// 上传文件2
bool upsendFile(SOCKET clifd, struct MsgHeader* pmsg)
{
	//告诉客户端准备接受文件了
	//如果文件的长度大于每个数据包能传送的大小(packet_size)那么就分快
	//printf("chenggong");
	struct MsgHeader msg;
	msg.msgID = MSG_UP_READY_READ;

	for (size_t i = 0; i < g_fileSize2; i += PACKET_SIZE)
	{
		//printf("upsendFile");
		msg.packet.nStart = i;
		//判断是否为最后一个包，获取最后一个包的长度
		if (i + PACKET_SIZE + 1 > g_fileSize2)
		{
			//printf("upsendFile1");
			msg.packet.nsize = g_fileSize2 - i;
		}
		else
		{
			
			msg.packet.nsize = PACKET_SIZE;
		}
		memcpy(msg.packet.buf, g_fileBuf + msg.packet.nStart, msg.packet.nsize);
		//printf("upsendFile2");
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("upsendFile3");
			printf("文件发送失败%d\n", WSAGetLastError());
			return false;
		}
	}

	return TRUE;
}
//准备读
void readyread(SOCKET serfd, struct MsgHeader* pmsg)//该函数用来分配内存
{
	strcpy(g_fileName, pmsg->fileInfo.fileName);
	//准备内存 pmsg->fileInfo.fileSize
	g_fileSize2 = pmsg->fileInfo.fileSize;
	//申请空间
	g_fileBuf = calloc(g_fileSize2 + 1, sizeof(char));
	if (g_fileBuf == NULL)//内存申请失败
	{
		printf("内存不足，请重试\n");
	}
	else
	{
		struct MsgHeader msg;
		msg.msgID = MSG_SENDFILE;
		if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			err("send");
			return;
		}
	}
}
//写文件
bool writeFile(SOCKET serfd, struct MsgHeader* pmsg)
{
	//如果g_fileBUf为空，则分配内存失败
	if (g_fileBuf == NULL)
	{
		return FALSE;
	}
	//将指针移到开始读的位置
	int nStart = pmsg->packet.nStart;
	//获取包长度
	int nsize = pmsg->packet.nsize;
	//拼接
	memcpy(g_fileBuf + nStart, pmsg->packet.buf, nsize);
	//判断文件是否完整，服务器是否发完了数据，接受到最后一个包时开始写入
	//printf(g_fileSize);
	if (nStart + nsize >= g_fileSize2)
	{
		FILE* pwrite = fopen(g_fileName, "wb");
		if (pwrite == NULL)
		{
			printf("write file error..\n");
			return false;
		}
		fwrite(g_fileBuf, sizeof(char), g_fileSize2, pwrite);
		//写完释放缓存，关闭读指针
		fclose(pwrite);
		free(g_fileBuf);
		g_fileBuf = NULL;
		//发送写入成功消息
		struct MsgHeader msg;
		msg.msgID = MSG_SUCCESSED;
		if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			err("send");
			return;
		}
	}
	return true;
}
//聊天室
bool chatroom(SOCKET serfd)
{
	HANDLE hThread[2];//声明两个线程
	unsigned threadID[2];
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0, p_send, &serfd, 0, &threadID[0]);//多线程p_send;
	hThread[1] = (HANDLE)_beginthreadex(NULL, 0, p_recv, &serfd, 0, &threadID[1]);//多线程p_recv;
	WaitForSingleObject(hThread[0], INFINITE);//等待线程结束
	WaitForSingleObject(hThread[1], INFINITE);
	CloseHandle(hThread[0]);//关闭线程
	CloseHandle(hThread[1]);

}
//多线程发送函数
unsigned __stdcall p_send(void* pfd) {
	SOCKET fd;
	fd = *((SOCKET*)pfd);
	struct MsgHeader Chat3;//用于发送消息
	int flag = 0;//用于标记是否退出
	while (flag == 0) {
		printf("send>");
		gets_s(Chat3.CHAT.chatbuf, PACKET_SIZE);
		//如果输入的内容是close，就关闭
		if (strlen(Chat3.CHAT.chatbuf) > 200)
		{
			printf("the buffer is too big!\n");
			memset(&Chat3, 0, sizeof(Chat3));
			continue;
		}
		if (strcmp(Chat3.CHAT.chatbuf, "close") == 0)
		{
			Chat3.msgID = CHAT_END;
			send(fd, (char*)&Chat3, sizeof(struct MsgHeader), 0);
			memset(&Chat3, 0, sizeof(Chat3.CHAT.chatbuf));
			printf("exit chatroom,bye~bye~\n");
			flag = 1;//标志结束
			closesocket(fd);
			return false;
		}
		//如果输入sendfile，就发送文件
		if (strcmp(Chat3.CHAT.chatbuf, "sendfile") == 0)
		{
			downloadFileName(fd);
		}
		Chat3.msgID = CHAT_ING;
		if (SOCKET_ERROR == send(fd, (char*)&Chat3, sizeof(struct MsgHeader), 0))
		{
			printf("server had closed!\n");
			return false;
		}
		memset(&Chat3, 0, sizeof(Chat3.CHAT.chatbuf));
	}
}
//对线程接受函数
unsigned __stdcall p_recv(void* pfd) {
	int flag = 0;//用于标记退出,为0不退出，为1退出
	while (flag == 0) {
		SOCKET fd;
		fd = *((SOCKET*)pfd);
		int nRes = recv(fd, g_recvBuf, 10240, 0);
		if (0 < nRes) {
			struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
			//因为在聊天过程中涉及到传送信息功能，所以要对信息进行判断，判断是否为文件还是聊天信息
			switch (msg->msgID)
			{
			case MSG_OPENFILE_FAILD:
				printf("send file failed ,please try again\n");
				downloadFileName(fd);
				break;
			case MSG_FILESIZE:
				readyread(fd, msg);
				break;
			case MSG_READY_READ:
				writeFile(fd, msg);
				break;
			case MSG_SUCCESSED:
				printf("send file success!\n");
				//进入选项窗口
				break;
			case CHAT_ING:
				printf("recv:%s\n", msg->CHAT.chatbuf);
				memset(msg, 0, sizeof(msg));//清空缓存防止溢出
				break;
			case CHAT_END:
				printf("the client had exit this system!\n");
				closeSocket();
				flag = 1;//标志位1，说明退出，线程结束
				break;
			}
		}
	}
}

#define MSG_FILELIST 2

void listFilesOnServer(SOCKET serfd) {
	// 获取用户输入的路径
	TCHAR inputPath[MAX_PATH];
	_tprintf(_T("请输入要列出文件的路径："));
	_tscanf_s(_T("%s"), inputPath, MAX_PATH);

	// 构造搜索路径
	TCHAR searchPath[MAX_PATH];
	_tcscpy_s(searchPath, MAX_PATH, inputPath);
	_tcscat_s(searchPath, MAX_PATH, _T("\\*"));

	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(searchPath, &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		error1("FindFirstFile");
		return;
	}

	_tprintf(_T("%s目录下的所有文件：\n"), inputPath);

	// 遍历目录下的所有文件
	do {
		// 输出文件名到控制台
		_tprintf(_T("%s\n"), findFileData.cFileName);

	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

