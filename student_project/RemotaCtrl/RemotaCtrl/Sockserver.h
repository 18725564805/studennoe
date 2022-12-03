#pragma once
#include "framework.h"

#pragma pack(push) //��ϵͳ��Ĭ�϶���������
#pragma pack(1)    // �޸�ϵͳ��Ĭ�϶�����
class Cpacket
{
public:
	Cpacket():m_sHead(0),m_Length(0),m_sCmd(0),m_sSun(0) {}

	//���ݴ�����캯��  nSize����pDataָ������ݵĳ���
	Cpacket(WORD Cmd, const BYTE*pData, size_t nSize) {
		m_sHead = 0xFEFF;
		m_Length = nSize + 4;
		m_sCmd = Cmd;
		if (nSize > 0) {
			m_strData.resize(nSize);
			memcpy((void*)m_strData.c_str(), (void*)pData, nSize);
		}
		else {
			m_strData.clear();
		}
		m_sSun = 0; //��У��
		for (int j = 0; j < m_strData.size(); j++) {
			m_sSun += BYTE(m_strData[j]) & 0xFF;
		}
	}

	Cpacket(const Cpacket& cp) {
		m_Length = cp.m_Length;
		m_sCmd = cp.m_sCmd;
		m_sHead = cp.m_sHead;
		m_sSun = cp.m_sSun;
		m_strData = cp.m_strData;
	}

	Cpacket& operator=(const Cpacket& cp) {
		if (this != &cp) {
			m_Length = cp.m_Length;
			m_sCmd = cp.m_sCmd;
			m_sHead = cp.m_sHead;
			m_sSun = cp.m_sSun;
			m_strData = cp.m_strData;
		}
		return *this;
	}

	//���ݽ������
	Cpacket(const BYTE* pData, int& nSize) { 
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)pData == 0xFEFF) {
				m_sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}

		//һ�������İ����������ô����ֽڣ���û�а����ݣ�ֻ��һ�������
		if (i + 4 + 2 + 2 >= nSize) { //�����ܲ�ȫ������ʧ�ܡ�
			nSize = 0;
			return;
		}

		m_Length = *(WORD*)(pData + i);	i += 4;
		if (m_Length + i > nSize) { //���յ����ݰ���ȫ,����ʧ�ܣ�ֱ�ӷ���.
			nSize = 0;
			return;
		}
		
		m_sCmd = *(WORD*)(pData+i) ; i += 2;
		if (m_Length > 4) {
			m_strData.reserve(m_Length - 4);
			memcpy((void*)m_strData.c_str(), pData + i, m_Length - 4);
			i += m_Length - 4;
		}


		m_sSun = *(WORD*)(pData + i); i += 2; //У��λ
		size_t sun = 0;
		for (int j = 0; j < m_strData.size(); j++) {
			sun += BYTE(m_strData[j]) & 0xFF;
		}

		if (sun == m_sSun) {
			nSize =i;
			return;
		}
		nSize = 0;
	}
	~Cpacket() {}

	int Size() {
		return m_Length + 6;
	}

	const char* Data() {
		strOut.resize(this->Size());
		BYTE* pData = (BYTE*)&strOut;
		*(WORD*)pData = m_sHead; pData += 2;
		*(DWORD*)pData = m_Length; pData += 4;
		*(WORD*)pData = m_sCmd; pData += 2;
		memcpy((void*)pData, m_strData.c_str(), m_strData.size());
		pData += m_strData.size();
		return strOut.c_str();
	}

public:
	WORD m_sHead; //��ͷ �̶�λ 0xFEFF
	DWORD m_Length;//�����ȣ��Ӱ����ʼ����У��λ�õĳ��ȣ�
	WORD m_sCmd;//������
	std::string m_strData; //������
	WORD m_sSun;//���ݰ��ĺ�У��
   std::string strOut;//���յ����ݰ�,���������ݶ������strOut��
};
#pragma pack(pop)//��ϵͳ��Ĭ�϶�������ԭ

class CSockserver
{
public:
	static CSockserver* getinstance() {
		if (pserver == NULL) {
			pserver = new CSockserver;
		}
		return pserver;
	}

	bool Initsockserv() {
		if (m_sock == -1) {
			return false;
		}

		sockaddr_in serv;
		serv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		serv.sin_family = AF_INET;
		serv.sin_port = htons(8888);
		if (bind(m_sock, (sockaddr*)&serv, sizeof(serv)) == -1) {
			return false;
		}

		if (listen(m_sock, 1) == -1) {
			return false;
		}

		return true;
	}

	bool InitAccept() {
		if (m_sock == -1) {
			return false;
		}
		sockaddr_in cli;
		int len = sizeof(cli);
		//���������ڴ˴����ȴ��ͻ�������
		m_client = accept(m_sock, (sockaddr*)&cli, &len);
		if (m_client == -1) {
			return false;
		}

		return true;
	}
#define BUFFER_SIZE 4096

	int Dealcommnat() {
		if (m_client == -1) return -1;
		char* buf = new char[BUFFER_SIZE];
		size_t index = 0;
		while (1) {
			int ret = recv(m_client, buf + index, BUFFER_SIZE - index, 0);
			if (ret <= 0) {
				return -1;
			}
			index += ret;
			ret = index;
			m_pack = Cpacket((BYTE*)buf, ret);
			if (ret > 0) {
				memmove(buf, buf + ret, BUFFER_SIZE - ret);
				index -= ret;
				return m_pack.m_sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, int size) {
		if (m_client == -1) return false;
		return send(m_client, pData, size, 0) > 0;
	}

	bool Send(Cpacket& pack) {
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilepath(std::string& path) {
		if ((m_pack.m_sCmd <= 4) && (m_pack.m_sCmd >= 2)) {
			path = m_pack.m_strData;
			return true;
		}
		return false;
	}

private:
	CSockserver(){
		WSADATA wsaData;
		if (0 != WSAStartup(MAKEWORD(1, 1), &wsaData)) {
			MessageBox(NULL, _T("�������"), _T("������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}

	~CSockserver(){
		closesocket(m_sock); 
		WSACleanup();
	}

	CSockserver& operator = (const CSockserver & sa){

	};

	class Cheap {
		public:
			Cheap() {
				CSockserver::getinstance();
			}
			~Cheap() {
				if (pserver != NULL) {
					CSockserver* tmp = pserver;
					pserver = NULL;
					delete tmp;
				}
			}
	};


private:
	SOCKET m_sock;
	SOCKET m_client;
	Cpacket m_pack;
	static CSockserver* pserver;
	static Cheap help;
};

