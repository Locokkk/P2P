#pragma once
#include<thread>
#include<boost/filesystem.hpp>
#include"util.hpp"
#include"httplib.h"

#define P2P_PORT 9000
#define MAX_IPBUFFER 16//ip�������ռ��С
#define MAX_RANGE (100 * 1024 * 1024)//1 ����10λ 1k ����20λ 1M   100����20λ 100M
#define SHARED_PATH "./Shared/"//���干��Ŀ¼
#define DOWNLOAD_PATH "./Download/"//���ص��ļ��������Ŀ¼

class Host
{
public:
	uint32_t _ip_addr;//Ҫ��Ե�����IP��ַ
	bool _pair_ret;//���ڴ����Խ��
};

class Client
{
public:
	bool Start()
	{
		while (1)
		{//�ͻ��˳�����Ҫѭ�����У���Ϊ�����ļ�����ֻ����һ��
			//ѭ������ÿ������һ���ļ�֮�󶼻����½����������---������
			GetOnlineHost();
		}
		return true;
	}
	//������Ե��߳���ں���
	void HostPair(Host *host)
	{
		//1.��֯httpЭ���ʽ����������
		//2.�һ��tcp�Ŀͻ��ˣ������ݷ���
		//3.�ȴ��������˵Ļظ��������н���
		//�������ʹ�õ�������httplibʵ�֣�httplib��ʵ������ҲҪ֪��
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);//�����ֽ���->�ַ�����ʽ
		httplib::Client cli(buf, P2P_PORT);//�ͻ��˶����ʵ����
		auto rsp = cli.Get("/hostpair");//���������������,�����˷�����ԴΪ/hostpair��Get����������ʧ�ܣ�Get�᷵�ؿ�
		if (rsp && rsp->status == 200)//�ж�����rsp���ڵ�ǰ���£������ǿ�ָ��ķ���
		{//ƥ��ɹ�
			host->_pair_ret = true;
		}
		return;
	}
	//��ȡ��������
	bool GetOnlineHost()
	{
		char ch = 'Y';//�Ƿ�����ƥ�䣬Ĭ���ǽ���ƥ�䣬���Ѿ�ƥ�����online������Ϊ�գ������û�ѡ��
		if (!_online_host.empty())
		{
			std::cout << "�Ƿ����²鿴��������(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "��ʼ����ƥ��\n";
			//1.��ȡ������Ϣ�������õ������������е�IP��ַ�б�
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			//��ȡ��������IP��ַ����ӵ�hostlist
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); i++)//�õ���������IP��ַ�б�
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				//���������
				uint32_t net = (ntohl(ip & mask));
				//�������������
				uint32_t max_host = (~ntohl(mask));//����IP�ļ��㣺��ת����С���ֽ��������ź������ţ���ֹ��Χ����
				//std::vector<bool> ret_list(max_host);//�ж�����Ƿ�ɹ������list��СҲ��max_host
				for (int j = 1; j <(int32_t)max_host; j++)
				{//����ȫ0��ȫ1
					uint32_t host_ip = net + j;//��0����������ŵõ���IP��ַ
					Host host;
					host._ip_addr = htonl(host_ip);//�����ֽ���->�����ֽ��� ����httplib���Client�Ĵ���Ҫ��
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}
			//��hostlist�е����������߳̽������
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); i++)
			{
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			std::cout << "���ڽ�������ƥ���У����Ժ�\n";
			//�ȴ������߳����������ϣ��ж���Խ����������������ӵ�onlinhostlist��
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
			//��������������ip��ӡ���������û�ѡ��
			for (int i = 0; i < _online_host.size(); i++)
			{
				char buf[MAX_IPBUFFER] = { 0 };
				inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
				std::cout << "\t" << buf << std::endl;
			}
		//3.���������õ���Ӧ�����Ӧ����Ϊ������������IP��ӵ����������б���
	    //4.��ӡ���������б��û�ѡ��
		std::cout << "��ѡ�������������ȡ�����ļ��б�";
		fflush(stdout);
		std::string select_ip;//�û�ѡ��Ҫ�����ļ���ip
		std::cin >> select_ip;
		GetShareList(select_ip);//�û�ѡ������֮�󣬵��û�ȡ�����ļ��ӿ�
		return true;
	}

	//��ȡ�����ļ��б�
	bool GetShareList(const std::string &host_ip)
	{
		//�����˷���һ���ļ��б��ȡ����
		//1.�ȷ�������
		//2.�õ���Ӧ֮�󣬽�������(�ļ�����)
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{//�˴���Ӧ����ȷ��
			std::cerr << "��ȡ�ļ��б���Ӧ����" << std::endl;
			return false;
		}
		//��ӡ����---��ӡ�������Ӧ���ļ����ƹ��û�ѡ��
		//body:filename1\r\nfilename2...
		std::cout << rsp->body << std::endl;
		std::cout << "\n��ѡ��Ҫ���ص��ļ�:";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}

	//�����ļ�
	bool DownloadFile(const std::string &host_ip, const std::string &filename)
	{
		//���ļ�һ�������أ��������ļ��Ƚ�Σ��
		//1.�����˷����ļ���������
		//2.�õ���Ӧ����е�body���ľ����ļ�����
		//3.�����ļ������ļ�����д���ļ��У��ر��ļ�
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		std::cout << "�����˷����ļ���������:" << host_ip << ":" <<req_path << std::endl;
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "�����ļ�����ȡ��Ӧ��Ϣʧ��\n" << rsp->status << std::endl;
			return false;
		}
		std::cout << "��ȡ�ļ�������Ӧ�ɹ�\n";
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{//��Ϊdownload_path�п��ܱ��ɵ�
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}
		std::cout << "�ļ����سɹ�\n";
		return true;
	}

	bool RangeDownload(const std::string &host_ip, const std::string &name)
	{
		//1.����HEAD����ͨ����Ӧ�е�Content_Length��ȡ�ļ���С
		std::string req_path = "/download/" + name;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "��ȡ�ļ���С��Ϣʧ��\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");//���ȡʲôֵ�������ֶ���������
		int64_t filesize = StringUtil::Str2Dig(clen);//�õ����ֵ��ļ���С
		//2.�����ļ���С���зֿ�
		int range_count = filesize / MAX_RANGE;
		//a.���ļ���СС�ڿ��СС����ֱ�������ļ�
		if (filesize < MAX_RANGE)
		{
			std::cout << "�ļ���С��ֱ������\n";
			return DownloadFile(host_ip, name);
		}
		//����ֿ����
		//b.���ļ���С�����������С����ֿ����Ϊ�ļ���С���Էֿ��С�ټ�һ
		//c.���ļ���С�������С����ֿ����Ϊ�ļ���С���Էֿ��С
		std::cout << "�ļ����󣬷ֿ�����\n";
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
			{//��ĩβ�ķֿ飬endλ�õļ���Ҫע�����ļ���С-1
				range_end = filesize - 1;
			}
			else
			{//������ĩλ�ķֿ飬endλ��ֱ���� ��ǰ�ֿ���*���С����ע��ϸ�ڴ���
				range_end = (i + 1)* MAX_RANGE - 1;
			}
			//3.��һ����ֿ�����,�õ���Ӧ֮��д���ļ���ָ��λ��
			httplib::Headers header;
			header.insert(httplib::make_range_header({ {range_start, range_end} }));//����һ��range����
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
            auto rsp = cli.Get(req_path.c_str(), header);
			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "���������ļ�ʧ��\n";
				return false;
			}
			FileUtil::Write(name, rsp->body, range_start);
		}
		std::cout << "�ļ����سɹ�\n";
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
		//�����Կͻ�������Ĵ���ʽ��Ӧ��ϵ
		_srv.Get("/hostpair", HostPair);//�յ�ʲô��������ʲô�ӿڴ���
		_srv.Get("/list", ShareList);
		//_srv.Get("filename");//filename��Ҫѡ�Ǹ��ļ�������ȷ��->������ʽ
		_srv.Get("/download/.*", DownLoad);//ƥ�������ַ������  .:ƥ���\n��\r�������ַ�  *:��ʾƥ��ǰ����ַ������ Ϊ�˱���ƥ�䵽ǰ���hostpair��list������/download��ʾɸѡ����Ҫ�����ļ�������
		
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}

private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;
		return;
	}
	//��ȡ�����ļ��б�--������������һ�������ļ�Ŀ¼���빲��ʲô�ļ��Ž�ȥ������
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.�鿴Ŀ¼�Ƿ�����ڣ����������򴴽�
		if (!boost::filesystem::exists(SHARED_PATH))
		{//�������򴴽�
			boost::filesystem::create_directory(SHARED_PATH);
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);//ʵ����Ŀ¼������
		boost::filesystem::directory_iterator end;//ʵ����Ŀ¼��������ĩβ   ע�⣺����Ҫ�����󣬷�����Ϊ��ʵ����Ŀ¼
		//��ʼ����Ŀ¼
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{//��ǰ�汾ֻ��ȡ��ͨ�ļ����ƣ�������㼶��Ŀ¼����
				continue;
			}
			std::string name = begin->path().filename().string();//ֻҪ�ļ�������Ҫ·��
			rsp.body += name + "\r\n";  //filename1\r\nfilename2\r\n
		}
		rsp.status = 200;
		return;
	}
	static void DownLoad(const httplib::Request &req, httplib::Response &rsp)
	{//���ؾ��ǰ����ݶ�ȡ��rsp��
		//std::cout << "������յ��ļ���������:" << req.path << std::endl;
		//req.path---�ͻ����������Դ·��  ��/download/filename����ʽ������ֻ��Ҫfilename
		boost::filesystem::path req_path(req.path);
		//boost::filesystem::path().filename() ֻ��ȡ�ļ����ƣ� abc/filename.txt->filename.txt
		std::string name = req_path.filename().string();//ֻ��ȡ�ļ�����
		//std::cout << "������յ��ļ���������" << name << "·��:" << SHARED_PATH << std::endl;
		std::string realpath = SHARED_PATH + name;//ʵ���ļ�·��Ӧ�����ڹ����Ŀ¼��
		//�ж��ļ��Ƿ����boost::filesystem::exists()
		//std::cout << "������յ�ʵ�ʵ��ļ�����·��:" << realpath << std::endl;
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{//�ļ������ڻ����ļ��Ǹ�Ŀ¼
			rsp.status = 404;
			return;
		}
		if (req.method == "GET")
		{//Ҫ�����������жϣ����鿴���������仹�Ƿֿ鴫��
			//ͨ���Ƿ���Rangeͷ���ֶ���ȷ�������ִ���
			if (req.has_header("Range"))
			{//��Range�ֶΣ����Ƿֿ鴫��
				//��Ҫ֪���ֿ�����---ͨ��get_header_value()��httplib::detail::parse_range_header()����Range�ֶ�
				std::string range_str = req.get_header_value("Range");//��ȡ��bytes=start-end
				httplib::Ranges ranges;//vector<Range>   Range-std::pair<start, end>
				httplib::detail::parse_range_header(range_str, ranges);//�����ͻ��˵�Range�ֶ�
				int64_t range_start = ranges[0].first;//pair�е�first����range��startλ��
				int64_t range_end = ranges[0].second;//pair�е�second����range��endλ��
				int64_t range_len = range_end - range_start + 1;
				std::cout << "range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
			}
			else
			{
				//û��Range�ֶΣ��������Ĵ���
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{//��ȡ�ļ�ʧ�ܣ��������ڲ�����Ҫ��true�Ļ���������ִ���˶�ȡ�ļ�����
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
			}
		}
		else
		{
			//��������HEAD�����----�ͻ��˲�Ҫ���ģ�ֻҪͷ��
			int64_t filesize = FileUtil::GetFileSize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//������Ӧ��ͷ��Ϣ k v��
			rsp.status = 200;
		}
		return;
	}
private:
	httplib::Server _srv;
};