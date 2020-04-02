#pragma once

#include<iostream>
#include<vector>
#include<sstream>
#include<fstream>
#include<boost/filesystem.hpp>
#ifdef _WIN32
//Windows头文件
#include<stdint.h>
#include<WS2tcpip.h>
#include<Iphlpapi.h>//获取网卡信息接口的头文件
#pragma comment(lib, "Iphlpapi.lib")//获取网卡信息接口链接的库
#pragma comment(lib, "ws2_32.lib")//windows下的socket库 inet_pton
#else
//Linux头文件

#endif

class StringUtil
{
public:
	static int64_t Str2Dig(const std::string &num)
	{
		std::stringstream tmp;
		tmp << num;
		int64_t res;
		tmp >> res;//根据res类型的不同进行重载，将结果放入res中
		return res;
	}
};

//ofstream是输出 ifstream是输入
class FileUtil
{//文件操作工具类，目的是为了让外部直接使用文件操作接口即可；后期修改可直接修改该类而实现对文件操作的修改，不用修改原文
public:
	static int64_t GetFileSize(const std::string &name)
	{
		return boost::filesystem::file_size(name);
	}
	static bool Write(const std::string &name, const std::string &body, int32_t offset = 0)//32位最大支持4G
	{
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "wb+");
		if (fp == NULL)
		{
			std::cout << "hhh\n";
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = (int)fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size())
		{
			std::cerr << "向文件写入数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;

		/*std::cout << "写入文件数据:" << name << "size:" << body.size() << "\n";
		std::ofstream ofs(name, std::ios::binary);//文件存在则打开，不存在则创建
		if (ofs.is_open() == false)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		ofs.seekp(offset, std::ios::beg);//跳转读写位置,相对文件起始位置开始偏移
		ofs.write(&body[0], body.size());
		if (ofs.good() == false)
		{
			std::cerr << "向文件写入数据失败\n";
			ofs.close();
			return false;
		}
		ofs.close();
		return true;*/
	}

	//指针参数表示是一个输出型参数
	//const & 表示是一个输入型参数
	//& 表示是一个输入输出型参数
	static bool Read(const std::string &name, std::string *body)
	{
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		int ret = (int)fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "从文件读取数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
		/*
		std::ifstream ifs(name);//实例化输入的文件对象
		if (ifs.is_open() == false)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "读取文件数据:" << name << "size:" << filesize << "\n";
		//std::cout << "要读取的文件大小:" << name << ":" << filesize << std::endl;
		ifs.read(&(*body)[0], filesize);

		/*
		if (ifs.good() == false)
		{
			std::cerr << "读取文件数据失败\n";
			ifs.close();
			return false;
		}**

		ifs.close();
		return true;
		*/
	}

	static bool ReadRange(const std::string &name, std::string *body, int64_t len, int64_t offset)
	{//从offset位置开始读取
		body->resize(len);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = (int)fread(&(*body)[0], 1, len, fp);
		if (ret != len)
		{
			std::cerr << "从文件读取数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
};

class Adapter
{
public:
	uint32_t _ip_addr;//网卡上的IP地址
	uint32_t _mask_addr;//网卡上的子网掩码
};

class AdapterUtil
{
public:
#ifdef _WIN32
	//windows下的获取网卡信息实现
	static bool GetAllAdapter(std::vector<Adapter> *list)
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
			//GetAdaptersInfo win下获取网卡信息的接口---网卡信息有可能有多个，因此传入一个指针
		//获取网卡信息有可能会失败，因为空间不足，因此有一个输出型参数，用于向用户返回所有网卡信息占用空间大小
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);
		//all_adapter_size用于获取实际所有网卡信息所占空间的字节
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			//这个错误表示缓冲区空间不足
			//重新给指针申请空间
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//重新获取网卡信息
		}
		while (p_adapters)
		{//链表
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)
			{//因为有些网卡并没有分配，所以IP地址为0
				//获取下一个
				list->push_back(adapter);//将网卡信息添加到vector中返回给用户
				adapter._mask_addr;
				//std::cout << "网卡名称：" << p_adapters->AdapterName << std::endl;
				//std::cout << "网卡描述：" << p_adapters->Description << std::endl;
				//std::cout << "IP地址：" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				//std::cout << "子网掩码：" << p_adapters->IpAddressList.IpMask.String << std::endl;
				//std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
};
#else
	bool GetAllAdapter(std::vector<Adapter> *list);//linux下的获取网卡信息实现 
#endif