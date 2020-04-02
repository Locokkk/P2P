#pragma once
#include<thread>
#include<boost/filesystem.hpp>
#include"util.hpp"
#include"httplib.h"

#define P2P_PORT 9000
#define MAX_IPBUFFER 16//ip缓冲区空间大小
#define MAX_RANGE (100 * 1024 * 1024)//1 左移10位 1k 左移20位 1M   100左移20位 100M
#define SHARED_PATH "./Shared/"//定义共享目录
#define DOWNLOAD_PATH "./Download/"//下载的文件放在这个目录

class Host
{
public:
	uint32_t _ip_addr;//要配对的主机IP地址
	bool _pair_ret;//用于存放配对结果
};

class Client
{
public:
	bool Start()
	{
		while (1)
		{//客户端程序需要循环运行，因为下载文件不是只下载一次
			//循环运行每次下载一个文件之后都会重新进行主机配对---不合理
			GetOnlineHost();
		}
		return true;
	}
	//主机配对的线程入口函数
	void HostPair(Host *host)
	{
		//1.组织http协议格式的请求数据
		//2.搭建一个tcp的客户端，将数据发送
		//3.等待服务器端的回复，并进行解析
		//这个过程使用第三方库httplib实现，httplib的实现流程也要知道
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);//网络字节序->字符串形式
		httplib::Client cli(buf, P2P_PORT);//客户端对象的实例化
		auto rsp = cli.Get("/hostpair");//发送主机配对请求,向服务端发送资源为/hostpair的Get请求，若连接失败，Get会返回空
		if (rsp && rsp->status == 200)//判断是在rsp存在的前提下，否则是空指针的访问
		{//匹配成功
			host->_pair_ret = true;
		}
		return;
	}
	//获取在线主机
	bool GetOnlineHost()
	{
		char ch = 'Y';//是否重新匹配，默认是进行匹配，若已经匹配过，online主机不为空，则让用户选择
		if (!_online_host.empty())
		{
			std::cout << "是否重新查看在线主机(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "开始主机匹配\n";
			//1.获取网卡信息，进而得到局域网中所有的IP地址列表
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			//获取所有主机IP地址，添加到hostlist
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); i++)//得到所有主机IP地址列表
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				//计算网络号
				uint32_t net = (ntohl(ip & mask));
				//计算最大主机号
				uint32_t max_host = (~ntohl(mask));//主机IP的计算：先转换成小端字节序的网络号和主机号，防止范围过大
				//std::vector<bool> ret_list(max_host);//判断配对是否成功，这个list大小也是max_host
				for (int j = 1; j <(int32_t)max_host; j++)
				{//不能全0和全1
					uint32_t host_ip = net + j;//从0到最大主机号得到各IP地址
					Host host;
					host._ip_addr = htonl(host_ip);//主机字节序->网络字节序 满足httplib库的Client的传参要求
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}
			//对hostlist中的主机创建线程进行配对
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); i++)
			{
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			std::cout << "正在进行主机匹配中，请稍后\n";
			//等待所有线程主机配对完毕，判断配对结果，将在线主机添加到onlinhostlist中
			for (int i = 0; i < host_list.size(); i++)
			{
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true)
				{
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}
			//将所有在线主机ip打印出来，供用户选择
			for (int i = 0; i < _online_host.size(); i++)
			{
				char buf[MAX_IPBUFFER] = { 0 };
				inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
				std::cout << "\t" << buf << std::endl;
			}
		//3.若配对请求得到响应，则对应主机为在线主机，则将IP添加到在线主机列表中
	    //4.打印在线主机列表供用户选择
		std::cout << "请选择配对主机，获取共享文件列表";
		fflush(stdout);
		std::string select_ip;//用户选择要下载文件的ip
		std::cin >> select_ip;
		GetShareList(select_ip);//用户选择主机之后，调用获取共享文件接口
		return true;
	}

	//获取共享文件列表
	bool GetShareList(const std::string &host_ip)
	{
		//向服务端发送一个文件列表获取请求
		//1.先发送请求
		//2.得到响应之后，解析正文(文件名称)
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{//此次响应不正确的
			std::cerr << "获取文件列表响应错误" << std::endl;
			return false;
		}
		//打印正文---打印服务端响应的文件名称供用户选择
		//body:filename1\r\nfilename2...
		std::cout << rsp->body << std::endl;
		std::cout << "\n请选择要下载的文件:";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}

	//下载文件
	bool DownloadFile(const std::string &host_ip, const std::string &filename)
	{
		//若文件一次性下载，遇到大文件比较危险
		//1.向服务端发送文件下载请求
		//2.得到响应结果中的body正文就是文件数据
		//3.创建文件，将文件数据写入文件中，关闭文件
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		std::cout << "向服务端发送文件下载请求:" << host_ip << ":" <<req_path << std::endl;
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "下载文件，获取响应信息失败\n" << rsp->status << std::endl;
			return false;
		}
		std::cout << "获取文件下载响应成功\n";
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{//因为download_path有可能被干掉
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}
		std::cout << "文件下载成功\n";
		return true;
	}

	bool RangeDownload(const std::string &host_ip, const std::string &name)
	{
		//1.发送HEAD请求，通过响应中的Content_Length获取文件大小
		std::string req_path = "/download/" + name;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "获取文件大小信息失败\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");//想获取什么值，给个字段名就行了
		int64_t filesize = StringUtil::Str2Dig(clen);//得到数字的文件大小
		//2.根据文件大小进行分块
		int range_count = filesize / MAX_RANGE;
		//a.若文件大小小于块大小小，则直接下载文件
		if (filesize < MAX_RANGE)
		{
			std::cout << "文件较小，直接下载\n";
			return DownloadFile(host_ip, name);
		}
		//计算分块个数
		//b.若文件大小不能整除块大小，则分块个数为文件大小除以分块大小再加一
		//c.若文件大小整除块大小，则分块个数为文件大小除以分块大小
		std::cout << "文件过大，分块下载\n";
		if ((filesize % MAX_RANGE) == 0)
		{
			range_count = filesize / MAX_RANGE;
		}
		else
		{
			range_count = (filesize / MAX_RANGE) + 1;
		}
		int64_t range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; i++)
		{
			range_start = i * MAX_RANGE;
			if (i == range_count - 1)
			{//最末尾的分块，end位置的计算要注意是文件大小-1
				range_end = filesize - 1;
			}
			else
			{//不是最末位的分块，end位置直接用 当前分块数*块大小，再注意细节处理
				range_end = (i + 1)* MAX_RANGE - 1;
			}
			//3.逐一请求分块数据,得到响应之后写入文件的指定位置
			httplib::Headers header;
			header.insert(httplib::make_range_header({ {range_start, range_end} }));//设置一个range区间
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
            auto rsp = cli.Get(req_path.c_str(), header);
			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "区间下载文件失败\n";
				return false;
			}
			FileUtil::Write(name, rsp->body, range_start);
		}
		std::cout << "文件下载成功\n";
		return true;
	}
private:
	std::vector<Host> _online_host;
};

class Server
{
public:
	bool Start()
	{
		//添加针对客户端请求的处理方式对应关系
		_srv.Get("/hostpair", HostPair);//收到什么请求则用什么接口处理
		_srv.Get("/list", ShareList);
		//_srv.Get("filename");//filename你要选那个文件啊？不确定->正则表达式
		_srv.Get("/download/.*", DownLoad);//匹配任意字符任意次  .:匹配除\n或\r的任意字符  *:表示匹配前面的字符任意次 为了避免匹配到前面的hostpair和list，加上/download表示筛选的是要下载文件的请求
		
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}

private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;
		return;
	}
	//获取共享文件列表--在主机上设置一个共享文件目录，想共享什么文件放进去就行了
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.查看目录是否存在在，若不存在则创建
		if (!boost::filesystem::exists(SHARED_PATH))
		{//不存在则创建
			boost::filesystem::create_directory(SHARED_PATH);
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);//实例化目录迭代器
		boost::filesystem::directory_iterator end;//实例化目录迭代器的末尾   注意：不需要传对象，否则认为在实例化目录
		//开始迭代目录
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{//当前版本只获取普通文件名称，不做多层级的目录操作
				continue;
			}
			std::string name = begin->path().filename().string();//只要文件名，不要路径
			rsp.body += name + "\r\n";  //filename1\r\nfilename2\r\n
		}
		rsp.status = 200;
		return;
	}
	static void DownLoad(const httplib::Request &req, httplib::Response &rsp)
	{//下载就是把内容读取到rsp中
		//std::cout << "服务端收到文件下载请求:" << req.path << std::endl;
		//req.path---客户端请求的资源路径  是/download/filename的形式，我们只需要filename
		boost::filesystem::path req_path(req.path);
		//boost::filesystem::path().filename() 只获取文件名称， abc/filename.txt->filename.txt
		std::string name = req_path.filename().string();//只获取文件名称
		//std::cout << "服务端收到文件下载名称" << name << "路径:" << SHARED_PATH << std::endl;
		std::string realpath = SHARED_PATH + name;//实际文件路径应该是在共享的目录下
		//判断文件是否存在boost::filesystem::exists()
		//std::cout << "服务端收到实际的文件下载路径:" << realpath << std::endl;
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{//文件不存在或者文件是个目录
			rsp.status = 404;
			return;
		}
		if (req.method == "GET")
		{//要对请求做出判断，即查看是完整传输还是分块传输
			//通过是否有Range头部字段来确定是哪种传输
			if (req.has_header("Range"))
			{//有Range字段，就是分块传输
				//需要知道分块区间---通过get_header_value()和httplib::detail::parse_range_header()解析Range字段
				std::string range_str = req.get_header_value("Range");//获取到bytes=start-end
				httplib::Ranges ranges;//vector<Range>   Range-std::pair<start, end>
				httplib::detail::parse_range_header(range_str, ranges);//解析客户端的Range字段
				int64_t range_start = ranges[0].first;//pair中的first就是range的start位置
				int64_t range_end = ranges[0].second;//pair中的second就是range的end位置
				int64_t range_len = range_end - range_start + 1;
				std::cout << "range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
			}
			else
			{
				//没有Range字段，是完整的传输
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{//读取文件失败，服务器内部出错，要是true的话，即正常执行了读取文件操作
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
			}
		}
		else
		{
			//这个是针对HEAD请求的----客户端不要正文，只要头部
			int64_t filesize = FileUtil::GetFileSize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//设置响应的头信息 k v的
			rsp.status = 200;
		}
		return;
	}
private:
	httplib::Server _srv;
};