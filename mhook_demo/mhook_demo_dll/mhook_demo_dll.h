// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 MHOOK_DEMO_DLL_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// MHOOK_DEMO_DLL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef MHOOK_DEMO_DLL_EXPORTS
#define MHOOK_DEMO_DLL_API __declspec(dllexport)
#else
#define MHOOK_DEMO_DLL_API __declspec(dllimport)
#endif

// 此类是从 mhook_demo_dll.dll 导出的
class MHOOK_DEMO_DLL_API Cmhook_demo_dll {
public:
	Cmhook_demo_dll(void);
	// TODO: 在此添加您的方法。
};

extern MHOOK_DEMO_DLL_API int nmhook_demo_dll;

MHOOK_DEMO_DLL_API int fnmhook_demo_dll(void);

MHOOK_DEMO_DLL_API LRESULT CALLBACK GefApiCBTProc( int	nCode, 
	WPARAM	wParam,  
	LPARAM	lParam 
	);
