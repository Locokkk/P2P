#include"client.hpp"

void helloworld(const httplib::Request &req, httplib::Response &rsp)
{
	printf("httplib server recv a req:%s\n", req.path.c_str());
	rsp.set_content("<html><h1>HelloWorld</h1></html>", "text/html");//给http响应添加正文 后面是content type
	rsp.status = 200;//响应状态码
}

void Scandir()
{
	//目录迭代，检测指定共享目录的文件信息：例如xxx文件是一个普通文件
	//boost::filesystem::path().filename() 只获取文件名称， abc/filename.txt->filename.txt
	//boost::filesystem::exits()判断文件是否存在
	char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//定义一个目录迭代器对象
	boost::filesystem::directory_iterator end;
	for (; begin != end; ++begin)
	{
		//begin->status()目录中当前文件的状态信息
		//boost::filesystem::is_directory()判断当前文件是否是一个目录
		if (boost::filesystem::is_directory(begin->status()))
		{//begin->path().string()获取当前迭代文件的文件名
			std::cout << begin->path().string() << "是一个目录\n";
		}
		else
		{
			std::cout << begin->path().string() << "是一个普通文件\n";
			std::cout << "文件名:" << begin->path().filename().string();//获取文件路径名中的名称filename，不要路径/download
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
	//在主线程中要运行客户端模块以及服务端模块
	//创建一个线程运行客户端模块，主线程运行服务端模块(因为客户端和服务端都是死循环，无法返回)
	std::thread thr_client(ClientRun);//若客户端运行的时候，服务端还没有启动
	
	Server srv;
	srv.Start();
	return 0;
}