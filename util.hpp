#pragma once

#include<iostream>
#include<vector>
#include<sstream>
#include<fstream>
#include<boost/filesystem.hpp>
#ifdef _WIN32
//Windowsͷ�ļ�
#include<stdint.h>
#include<WS2tcpip.h>
#include<Iphlpapi.h>//��ȡ������Ϣ�ӿڵ�ͷ�ļ�
#pragma comment(lib, "Iphlpapi.lib")//��ȡ������Ϣ�ӿ����ӵĿ�
#pragma comment(lib, "ws2_32.lib")//windows�µ�socket�� inet_pton
#else
//Linuxͷ�ļ�

#endif

class StringUtil
{
public:
	static int64_t Str2Dig(const std::string &num)
	{
		std::stringstream tmp;
		tmp << num;
		int64_t res;
		tmp >> res;//����res���͵Ĳ�ͬ�������أ����������res��
		return res;
	}
};

//ofstream����� ifstream������
class FileUtil
{//�ļ����������࣬Ŀ����Ϊ�����ⲿֱ��ʹ���ļ������ӿڼ��ɣ������޸Ŀ�ֱ���޸ĸ����ʵ�ֶ��ļ��������޸ģ������޸�ԭ��
public:
	static int64_t GetFileSize(const std::string &name)
	{
		return boost::filesystem::file_size(name);
	}
	static bool Write(const std::string &name, const std::string &body, int32_t offset = 0)//32λ���֧��4G
	{
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "wb+");
		if (fp == NULL)
		{
			std::cout << "hhh\n";
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = (int)fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size())
		{
			std::cerr << "���ļ�д������ʧ��\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;

		/*std::cout << "д���ļ�����:" << name << "size:" << body.size() << "\n";
		std::ofstream ofs(name, std::ios::binary);//�ļ�������򿪣��������򴴽�
		if (ofs.is_open() == false)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		ofs.seekp(offset, std::ios::beg);//��ת��дλ��,����ļ���ʼλ�ÿ�ʼƫ��
		ofs.write(&body[0], body.size());
		if (ofs.good() == false)
		{
			std::cerr << "���ļ�д������ʧ��\n";
			ofs.close();
			return false;
		}
		ofs.close();
		return true;*/
	}

	//ָ�������ʾ��һ������Ͳ���
	//const & ��ʾ��һ�������Ͳ���
	//& ��ʾ��һ����������Ͳ���
	static bool Read(const std::string &name, std::string *body)
	{
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		int ret = (int)fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "���ļ���ȡ����ʧ��\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
		/*
		std::ifstream ifs(name);//ʵ����������ļ�����
		if (ifs.is_open() == false)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "��ȡ�ļ�����:" << name << "size:" << filesize << "\n";
		//std::cout << "Ҫ��ȡ���ļ���С:" << name << ":" << filesize << std::endl;
		ifs.read(&(*body)[0], filesize);

		/*
		if (ifs.good() == false)
		{
			std::cerr << "��ȡ�ļ�����ʧ��\n";
			ifs.close();
			return false;
		}**

		ifs.close();
		return true;
		*/
	}

	static bool ReadRange(const std::string &name, std::string *body, int64_t len, int64_t offset)
	{//��offsetλ�ÿ�ʼ��ȡ
		body->resize(len);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = (int)fread(&(*body)[0], 1, len, fp);
		if (ret != len)
		{
			std::cerr << "���ļ���ȡ����ʧ��\n";
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
	uint32_t _ip_addr;//�����ϵ�IP��ַ
	uint32_t _mask_addr;//�����ϵ���������
};

class AdapterUtil
{
public:
#ifdef _WIN32
	//windows�µĻ�ȡ������Ϣʵ��
	static bool GetAllAdapter(std::vector<Adapter> *list)
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
			//GetAdaptersInfo win�»�ȡ������Ϣ�Ľӿ�---������Ϣ�п����ж������˴���һ��ָ��
		//��ȡ������Ϣ�п��ܻ�ʧ�ܣ���Ϊ�ռ䲻�㣬�����һ������Ͳ������������û���������������Ϣռ�ÿռ��С
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);
		//all_adapter_size���ڻ�ȡʵ������������Ϣ��ռ�ռ���ֽ�
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			//��������ʾ�������ռ䲻��
			//���¸�ָ������ռ�
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//���»�ȡ������Ϣ
		}
		while (p_adapters)
		{//����
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)
			{//��Ϊ��Щ������û�з��䣬����IP��ַΪ0
				//��ȡ��һ��
				list->push_back(adapter);//��������Ϣ��ӵ�vector�з��ظ��û�
				adapter._mask_addr;
				//std::cout << "�������ƣ�" << p_adapters->AdapterName << std::endl;
				//std::cout << "����������" << p_adapters->Description << std::endl;
				//std::cout << "IP��ַ��" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				//std::cout << "�������룺" << p_adapters->IpAddressList.IpMask.String << std::endl;
				//std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
};
#else
	bool GetAllAdapter(std::vector<Adapter> *list);//linux�µĻ�ȡ������Ϣʵ�� 
#endif