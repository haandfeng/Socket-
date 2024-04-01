#include <stdio.h>
#include"ftpServer.h"
char g_recvBuf[10240];// 变量自动初始化为零
int g_fileSize;//获取文件大小
char* g_fileBuf;
char uploadPath[1024] = ""; // 文件上传路径
int uploadPathSize = 0;
#pragma warning(disable : 4996)
char g_fileName[256];        //保存客户端发送过来的文件名
int main()
{
	initSocket();
	listenToClient();
	closeSocket();
	return 0;
}

/*

客户端请求下载文件->把文件名发送给服务器
服务器接受客户端发送的文件名->根据文件名，找到文件，把文件大小发给客户端
客户端接受到大小，开始准备开辟内存，告诉服务器准备发送
服务器接受开始发送指令->开始发送数据
开始接受，存起来->接受完成，告诉服务器接受完成
关闭连接
*/


//初始化socket库
bool initSocket()
{
	WSADATA wsadata;
	//makeword：请求的协议版本
	//只能在一次成功的WSAStartup()调用之后才能调用进一步的Windows Sockets API函数。
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		printf("WSAStartup faild :%d\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;

}
//关闭socket库
bool closeSocket()
{
	if (0 != WSACleanup())
	{
		printf("WSACleaup faild:%d\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}
//监听客户端连接
void listenToClient()
{
	printf("listen to client....\n");
	//创建server socket套接字 地址，端口号
	//INVALID_SOKET SOCKET_ERROR
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		err("socket");
		return;
	}
	//给socket绑定ip和端口号
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;//必须和创建的socket指定的一样
	serAddr.sin_port = htons(SPORT);
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY;//监听本机所有网卡
	//绑定套接字
	if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		err("bind");
		return;
	}
	//监听客户端连接
	if (0 != listen(serfd, 10))
	{
		err("listen");
		return;
	}
	//有客户端连接，就接受
	struct sockaddr_in cliAddr;
	int len = sizeof(cliAddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*)&cliAddr, &len);
	if (INVALID_SOCKET == clifd)
	{
		err("accept");
		return;
	}
	printf("new client connect success....\n");
	//开始处理消息
	while (processMsg(clifd)) {}
}
//处理消息函数
bool processMsg(SOCKET clifd)
{
	int nRes = recv(clifd, g_recvBuf, 10240, 0);
	//成功接收消息，返回接受到的字节数，接受失败返回0
	if (nRes <= 0)
	{
		printf("客户端下线。。。。%d\n", WSAGetLastError());
		return false;
	}
	//将接受到的缓存信息强转为结构体类型，便于之后的信息判断
	struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
	//在内部无法定义结构体，只能在外面定义，用于消息的发送
	struct MsgHeader exitmsg;

	switch (msg->msgID)
	{
	case MSG_FILENAME:
		printf("要发送的文件是：%s\n", msg->fileInfo.fileName);
		readFile(clifd, msg);//查找文件
		break;
	case MSG_SENDFILE:
		sendFile(clifd, msg);//发送文件
		break;
	case MSG_DELETEFILE:
		deleteFileOnServer(clifd,msg->fileInfo.fileName);
		break;
	case MSG_DIRECTORY:
		createDirectory(clifd, msg->dirInfo.dirPath);
		break;
	case MSG_UPFILENAME:
		receiveFile(clifd,msg);
		break;
	case MSG_PATHNAME: // 改变路径
		changePath(clifd, msg);//改变路径
		break;
	case MSG_UP_READY_READ:
		upwriteFile(clifd, msg);//改变路径
		break;
	case MSG_SUCCESSED:
		exitmsg.msgID = MSG_SUCCESSED;
		if (SOCKET_ERROR == send(clifd, (char*)&exitmsg, sizeof(struct MsgHeader), 0))
		{
			printf("send faild :%d\n", WSAGetLastError());
			return false;
		}
		printf("客户端已成功接受！~\n");
		break;
	case ENDSEND:
		printf("客户端下线，bye~bye~\n");
		closesocket(clifd);
		return FALSE;
		break;
	case CHAT_ING:
		printf("recv:%s\n", msg->CHAT.chatbuf);
		memset(msg, 0, sizeof(msg));//清空缓存防止溢出
		HANDLE hThread[2];//创建线程句柄
		unsigned threadID[2];

		hThread[0] = (HANDLE)_beginthreadex(NULL, 0, p_send, &clifd, 0, &threadID[0]);//多线程send;
		hThread[1] = (HANDLE)_beginthreadex(NULL, 0, p_recv, &clifd, 0, &threadID[1]);//多线程recv;

		WaitForSingleObject(hThread[0], INFINITE);
		WaitForSingleObject(hThread[1], INFINITE);
		CloseHandle(hThread[0]);
		CloseHandle(hThread[1]);
		break;
	case CHAT_END:
		printf("客户端不想聊天啦\n");
		closesocket(clifd);
		memset(msg, 0, sizeof(msg));
		return false;
		break;
	}
	return true;
}
//多线程发送方函数
unsigned __stdcall p_send(void* pfd) {
	SOCKET fd;
	fd = *((SOCKET*)pfd);
	struct MsgHeader p_exitmsg;
	while (true)
	{
		printf("send>");
		gets_s(p_exitmsg.CHAT.chatbuf, PACKET_SIZE);
		//服务端对接受到的信息进行处理，如果close则关闭连接
		if (strlen(p_exitmsg.CHAT.chatbuf) > 200)
		{
			printf("the buffer is too big!\n");
			memset(&p_exitmsg, 0, sizeof(p_exitmsg));
			continue;
		}
		if (strcmp(p_exitmsg.CHAT.chatbuf, "close") == 0)
		{
			p_exitmsg.msgID = CHAT_END;
			if (SOCKET_ERROR == send(fd, (char*)&p_exitmsg, sizeof(struct MsgHeader), 0))
			{
				err("send");
			}
			printf("终止会话！\n");
			closesocket(fd);
			return false;
		}
		p_exitmsg.msgID = CHAT_ING;
		if (SOCKET_ERROR == send(fd, (char*)&p_exitmsg, sizeof(struct MsgHeader), 0))
		{
			printf("client had closed!\n");
			return false;
		}
		memset(&p_exitmsg, 0, sizeof(p_exitmsg));
	}
}
//多线程接受函数
unsigned __stdcall p_recv(void* pfd) {
	SOCKET fd;
	fd = *((SOCKET*)pfd);
	struct MsgHeader p_exitmsg;//用于发送信息
	while (true) {
		int nRes = recv(fd, g_recvBuf, 10240, 0);
		if (0 < nRes) {
			struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
			switch (msg->msgID)
			{
			case MSG_FILENAME:
				printf("要发送的文件是：%s\n", msg->fileInfo.fileName);
				readFile(fd, msg);//查找文件
				break;
			case MSG_SENDFILE:
				sendFile(fd, msg);//发送文件
				break;
			case MSG_PATHNAME:
				changePath(fd, msg);//改变路径
				break;
			case MSG_UP_READY_READ:
				upwriteFile(fd, msg);//改变路径
				break;
			case MSG_SUCCESSED:
				p_exitmsg.msgID = MSG_SUCCESSED;
				if (SOCKET_ERROR == send(fd, (char*)&p_exitmsg, sizeof(struct MsgHeader), 0))
				{
					printf("send faild :%d\n", WSAGetLastError());
					return false;
				}
				printf("客户端已成功接受！~\n");
				break;
			case ENDSEND:
				printf("客户端下线，bye~bye~\n");
				closesocket(fd);
				return FALSE;
				break;
			case CHAT_END:
				return false;
				break;
			case CHAT_ING:
				printf("recv:%s\n", msg->CHAT.chatbuf);
				memset(msg, 0, sizeof(msg));//清空缓存防止溢出
				break;
			}
		}
	}
}
//读文件，获取文件大小
bool readFile(SOCKET clifd, struct MsgHeader* pmsg)
{
	char path[2048];
	//printf("%s\n", uploadPath);
	strcpy(path, uploadPath);
	//printf("%s\n", path);

	//printf("%s\n", pmsg->fileInfo.fileName);
	strcat(path, pmsg->fileInfo.fileName);
	printf("%s\n", path);
	FILE* pread = fopen(path, "rb");
	//找不到就告诉客户端发送失败
	if (pread == NULL)
	{
		printf("找不到[%s]文件..\n", pmsg->fileInfo.fileName);
		struct MsgHeader msg;
		msg.msgID = MSG_OPENFILE_FAILD;
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("send faild:%d\n", WSAGetLastError());
		}
		return FALSE;
	}
	//找到文件后，获取文件大小
	fseek(pread, 0, SEEK_END);
	g_fileSize = ftell(pread);//函数 ftell 用于得到文件位置指针当前位置相对于文件首的偏移字节数
	fseek(pread, 0, SEEK_SET);//读指针移到文件开始地方，准备开始读
	struct MsgHeader msg;
	msg.msgID = MSG_FILESIZE;
	//把文件大小发给客户端
	msg.fileInfo.fileSize = g_fileSize;
	char tfname[200] = { 0 }, text[100];
	_splitpath(pmsg->fileInfo.fileName, NULL, NULL, tfname, text);//获取文件的名字和属性
	strcat(tfname, text);
	//把文件名发给客户端
	strcpy(msg.fileInfo.fileName, tfname);
	send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);
	//读取文件内容
	//分配内存空间
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
	if (g_fileBuf == NULL)
	{
		printf("内存不足，请重试\n");
		return false;
	}
	fread(g_fileBuf, sizeof(char), g_fileSize, pread);
	g_fileBuf[g_fileSize] = '\0';
	fclose(pread);
	return true;
}
//发文件
bool sendFile(SOCKET clifd, struct MsgHeader* pmsg)
{
	//告诉客户端准备接受文件了
	//如果文件的长度大于每个数据包能传送的大小(packet_size)那么就分快
	struct MsgHeader msg;
	msg.msgID = MSG_READY_READ;

	for (size_t i = 0; i < g_fileSize; i += PACKET_SIZE)
	{
		msg.packet.nStart = i;
		//判断是否为最后一个包，获取最后一个包的长度
		if (i + PACKET_SIZE + 1 > g_fileSize)
		{
			msg.packet.nsize = g_fileSize - i;
		}
		else
		{
			msg.packet.nsize = PACKET_SIZE;
		}
		memcpy(msg.packet.buf, g_fileBuf + msg.packet.nStart, msg.packet.nsize);
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("文件发送失败%d\n", WSAGetLastError());
			return false;
		}
	}

	return TRUE;
}

//接收文件
void receiveFile(SOCKET clifd, struct MsgHeader* pmsg) {

	strcpy(g_fileName, pmsg->fileInfo.fileName);
	//准备内存 pmsg->fileInfo.fileSize
	g_fileSize = pmsg->fileInfo.fileSize;
	//申请空间
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
	if (g_fileBuf == NULL)//内存申请失败
	{	
		printf("内存不足，请重试\n");
	}
	else
	{
		struct MsgHeader msg;
		msg.msgID = MSG_UPFILE;
		//printf("文件发送失败\n");
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			err("send");
			return;
		}
	}
}
//写文件
bool upwriteFile(SOCKET serfd, struct MsgHeader* pmsg)
{
	printf("write");
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
	if (nStart + nsize >= g_fileSize)
	{
		FILE* pwrite = fopen(g_fileName, "wb");
		if (pwrite == NULL)
		{
			printf("write file error..\n");
			return false;
		}
		fwrite(g_fileBuf, sizeof(char), g_fileSize, pwrite);
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

void deleteFileOnServer(SOCKET clifd,const char* fileName) {
	if (remove(fileName) == 0) {
		printf("文件 %s 删除成功\n", fileName);
		struct MsgHeader msg;
		msg.msgID = MSG_SUCCESSED;
		send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);
		return true;
	}
	else {
		perror("删除文件失败");

	}

}


int createDirectory(SOCKET clifd,const char* directoryPath) {
	// 使用mkdir()函数创建目录
	// 第一个参数是目录路径，第二个参数是权限（在Linux中使用八进制表示）
	int result = mkdir(directoryPath, 0777);

	if (result == 0) {
		printf("目录创建成功：%s\n", directoryPath);
		struct MsgHeader msg;
		msg.msgID = MSG_SUCCESSED;
		send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);
		//return true;
		return 1;

	}
	else {
		printf("目录创建失败：%s\n", directoryPath);
		return 0;
	}


// 发送文件
}
bool changePath(SOCKET clifd, struct MsgHeader* pmsg)
{
	strcpy(uploadPath, pmsg->fileInfo.filePath);
	//"C:\Log\FRE_Log_20231114.log"
	char ch = '\\';
	strncat(uploadPath, &ch, 1);
	printf("%s\n", uploadPath);
	//for(int i =0;i< uploadPathSize;)
	struct MsgHeader msg;
	msg.msgID = MSG_SUCCESSED;
	send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);
	return true;
}

// 服务器端接收并处理客户端请求的函数
void handleClientRequest(int clientSocket) {
	struct MsgHeader request;
	if (recv(clientSocket, (char*)&request, sizeof(struct MsgHeader), 0) == SOCKET_ERROR) {
		perror("recv");
		return;
	}

	if (request.msgID == MSG_DIRECTORY) {
		// 处理创建目录请求
		const char* directoryPath = request.dirInfo.dirPath;

		// 在服务器端执行创建目录的操作
		//int result = createDirectory(clifddirectoryPath);
		int result = 0;
		// 根据操作结果生成回复消息
		struct MsgHeader response;
		if (result == 0) {
			response.msgID = MSG_SUCCESS;
			strcpy(response.fileInfo.fileName, "目录创建成功");
		}
		else {
			response.msgID = MSG_FAILURE;
			strcpy(response.fileInfo.fileName, "目录创建失败");
		}

		// 发送回复消息给客户端
		if (send(clientSocket, (char*)&response, sizeof(struct MsgHeader), 0) == SOCKET_ERROR) {
			perror("send");
			return;
		}
	}
}