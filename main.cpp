#include"client.hpp"

void helloworld(const httplib::Request &req, httplib::Response &rsp)
{
	printf("httplib server recv a req:%s\n", req.path.c_str());
	rsp.set_content("<html><h1>HelloWorld</h1></html>", "text/html");//��http��Ӧ������� ������content type
	rsp.status = 200;//��Ӧ״̬��
}

void Scandir()
{
	//Ŀ¼���������ָ������Ŀ¼���ļ���Ϣ������xxx�ļ���һ����ͨ�ļ�
	//boost::filesystem::path().filename() ֻ��ȡ�ļ����ƣ� abc/filename.txt->filename.txt
	//boost::filesystem::exits()�ж��ļ��Ƿ����
	char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//����һ��Ŀ¼����������
	boost::filesystem::directory_iterator end;
	for (; begin != end; ++begin)
	{
		//begin->status()Ŀ¼�е�ǰ�ļ���״̬��Ϣ
		//boost::filesystem::is_directory()�жϵ�ǰ�ļ��Ƿ���һ��Ŀ¼
		if (boost::filesystem::is_directory(begin->status()))
		{//begin->path().string()��ȡ��ǰ�����ļ����ļ���
			std::cout << begin->path().string() << "��һ��Ŀ¼\n";
		}
		else
		{
			std::cout << begin->path().string() << "��һ����ͨ�ļ�\n";
			std::cout << "�ļ���:" << begin->path().filename().string();//��ȡ�ļ�·�����е�����filename����Ҫ·��/download
		}
	}
}

void test()
{
	//Scandir();
	//Sleep(1000000);
	/*
	httplib::Server srv;
	srv.Get("/", helloworld);
	srv.listen("0.0.0.0", 9000);

	/*std::vector<Adapter> list;
	AdapterUtil::GetAllAdapter(&list);
	Sleep(100000);*/
}
void ClientRun()
{
	Client cli;
	cli.Start();
}
int main(int argc, char *argv[])
{
	//�����߳���Ҫ���пͻ���ģ���Լ������ģ��
	//����һ���߳����пͻ���ģ�飬���߳����з����ģ��(��Ϊ�ͻ��˺ͷ���˶�����ѭ�����޷�����)
	std::thread thr_client(ClientRun);//���ͻ������е�ʱ�򣬷���˻�û������
	
	Server srv;
	srv.Start();
	return 0;
}