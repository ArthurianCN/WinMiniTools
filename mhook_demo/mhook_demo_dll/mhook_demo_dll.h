// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� MHOOK_DEMO_DLL_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// MHOOK_DEMO_DLL_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef MHOOK_DEMO_DLL_EXPORTS
#define MHOOK_DEMO_DLL_API __declspec(dllexport)
#else
#define MHOOK_DEMO_DLL_API __declspec(dllimport)
#endif

// �����Ǵ� mhook_demo_dll.dll ������
class MHOOK_DEMO_DLL_API Cmhook_demo_dll {
public:
	Cmhook_demo_dll(void);
	// TODO: �ڴ�������ķ�����
};

extern MHOOK_DEMO_DLL_API int nmhook_demo_dll;

MHOOK_DEMO_DLL_API int fnmhook_demo_dll(void);

MHOOK_DEMO_DLL_API LRESULT CALLBACK GefApiCBTProc( int	nCode, 
	WPARAM	wParam,  
	LPARAM	lParam 
	);
