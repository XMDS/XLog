#include <jni.h>
#include <pthread.h>
#include <string>
#include <map>
#include <memory>
#include "mylog.h"
#include "CHook.h"
#include "inireader.h"

using namespace ARMHook;
using namespace std;

//时间类型和风格，见mylog.h
static unsigned int type = tLOCAL;
static unsigned int style = tDEFAULT; 
//prio等级 默认0打印信息到XLog默认文件，为-1忽略这个prio的信息不打印，为1为每种prio创建单独的log文件打印信息，共7种prio。
static int VERBOSE = 0;
static int DEBUG = 0;
static int INFO = 0;
static int WARN = 0;
static int ERROR = 0;
static int FATAL = 0;
static int SILENT = 0;

static int ignores = 0;
static int addfile = 0;
static bool log_test = false;

typedef std::shared_ptr<mylog> logs; //定义智能指针为新的log类型，保存mylog对象
std::map<int, string> ignore_tag; //忽略任意数量tag，保存的容器。 
std::map<string, logs> log_tag; //添加tag文件的mylog对象保存的容器
std::map<int, logs> log_prio; //添加prio文件的mylog对象保存的容器

int (*org__android_log_write)(int prio, const char* tag, const char* text);
int (*org__android_log_print)(int prio, const char* tag, const char* fmt, ...);
int (*org__android_log_vprint)(int prio, const char* tag, const char* fmt, va_list ap);
void (*org__android_log_assert)(const char* cond, const char* tag, const char* fmt, ...);

inline static int GetLogLevel(int prio)
{
	switch (prio)
	{
	case ANDROID_LOG_VERBOSE:
		return VERBOSE;
	case ANDROID_LOG_DEBUG:
		return DEBUG;
	case ANDROID_LOG_INFO:
		return INFO;
	case ANDROID_LOG_WARN:
		return WARN;
	case ANDROID_LOG_ERROR:
		return ERROR;
	case ANDROID_LOG_FATAL:
		return FATAL;
	case ANDROID_LOG_SILENT:
		return SILENT;
	default:
		break;
	}
	return 0;
}

inline static void WriteNotes(char* section, char* key, char* text)
{
	inireader.WriteString(section, key, text);
}

void MakeLogPach(char*& pach, const char* name)
{
	memset(pach, NULL, 255);
	strcpy(pach, log_prio[ANDROID_LOG_DEFAULT]->LogPath);
	pach[strlen(pach) - 4] = '\0';
	strcat(pach, name);
}

void print_my_log(int prio, const char* tag, const char* text, bool func_type, va_list arg)
{
	bool tag_check = false;
	bool add_check = false;
	static bool remove_check = true;

	if (addfile > 0 && tag) {
		map<string, logs>::iterator iter;
		for (iter = log_tag.begin(); iter != log_tag.end(); iter++) {
			if (strcasecmp(iter->first.c_str(), tag) == 0) {
				add_check = true; //添加tag log文件
			}
		}
	}
	if (ignores > 0 && tag) {
		map<int, string>::iterator iter;
		for (iter = ignore_tag.begin(); iter != ignore_tag.end(); iter++) {
			if (strcasecmp(iter->second.c_str(), tag) == 0) {
				tag_check = true; //忽略tag
			}
		}
	}
	int level = GetLogLevel(prio);

	if (level == 0) { //默认prio等级，打印日志
		if (!tag_check) { //如果不忽略tag
			if (!add_check) { //如果没有添加tag文件
				func_type ? log_prio[ANDROID_LOG_DEFAULT]->WriteFormatAllInfo(prio, tag, (tType)type, (tStyle)style, text, arg)
					: log_prio[ANDROID_LOG_DEFAULT]->WriteAllInfo(prio, tag, text, (tType)type, (tStyle)style); //打印默认
			}
			else { //如果添加tag文件
				func_type ? log_tag[tag]->WriteFormatAllInfo(prio, tag, (tType)type, (tStyle)style, text, arg)
					: log_tag[tag]->WriteAllInfo(prio, tag, text, (tType)type, (tStyle)style); //根据tag新文件打印
			}
		}
		else
			return; //忽略这个tag名称的日志，不打印
	}
	else if (level == -1) //忽略这个prio等级的日志，不打印
		return;
	else if (level == 1) { //根据prio新文件打印
		if (!tag_check) { //如果不忽略tag
			if (!add_check) { //如果没有添加tag文件 
				func_type ? log_prio[prio]->WriteFormatAllInfo(prio, tag, (tType)type, (tStyle)style, text, arg)
					: log_prio[prio]->WriteAllInfo(prio, tag, text, (tType)type, (tStyle)style); //根据prio新文件打印
			}
			else { //如果添加tag文件
				if (remove_check) { //如果没有删除tag
					for (auto iter = log_tag.begin(); iter != log_tag.end(); iter++) {
						remove(iter->second->LogPath); //prio优先级高于tag，当同时开启prio新文件和tag新文件生成时，删除tag创建的新文件
						remove_check = false; //只删除一次，不用每次进入函数都删除
					}
				}
				func_type ? log_prio[prio]->WriteFormatAllInfo(prio, tag, (tType)type, (tStyle)style, text, arg)
					: log_prio[prio]->WriteAllInfo(prio, tag, text, (tType)type, (tStyle)style); //根据prio新文件打印
			}
		}
		else
			return; //忽略这个tag名称的日志，不打印
	}
	else
		log_prio[ANDROID_LOG_DEFAULT]->WriteAllInfo(LOG_WARN, "XLOG", "The prio levels are wrong, they only have 3 values: -1, 0, 1. This log message will not be printed, Please check the configuration of the ini file.", (tType)type, (tStyle)style);
}

int My__android_log_write(int prio, const char* tag, const char* text)
{
	va_list lst;
	print_my_log(prio, tag, text, false, lst);
	va_end(lst);
	return org__android_log_write(prio, tag, text);
}

int My__android_log_print(int prio, const char* tag, const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);
	print_my_log(prio, tag, fmt, true, lst);
	int ret = org__android_log_print(prio, tag, fmt, lst);
	va_end(lst);
	return ret;
}

int My__android_log_vprint(int prio, const char* tag, const char* fmt, va_list ap)
{
	print_my_log(prio, tag, fmt, true, ap);
	return org__android_log_vprint(prio, tag, fmt, ap);
}

void My__android_log_assert(const char* cond, const char* tag, const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);

	bool tag_check = false;
	bool add_check = false;

	if (addfile > 0 && tag) {
		for (auto iter = log_tag.begin(); iter != log_tag.end(); iter++) {
			if (strcasecmp(iter->first.c_str(), tag) == 0) {
				add_check = true; //添加tag log文件
			}
		}
	}
	
	if (ignores > 0 && tag) {
		for (auto iter = ignore_tag.begin(); iter != ignore_tag.end(); iter++) {
			if (strcasecmp(iter->second.c_str(), tag) == 0) {
				tag_check = true; //忽略tag
			}	
		}
	}
	
	if (!tag_check) { //如果不忽略tag
		if (!add_check) { //如果没有添加tag文件
			log_prio[ANDROID_LOG_DEFAULT]->WriteFormatAllInfo(LOG_FATAL, tag, (tType)type, (tStyle)style, fmt, lst); //打印默认
		}
		else { //如果添加tag文件
			log_tag[tag]->WriteFormatAllInfo(LOG_FATAL, tag, (tType)type, (tStyle)style, fmt, lst); //根据tag新文件打印
		}
	}
	org__android_log_assert(cond, tag, fmt, lst);
	va_end(lst);
}

static void* my_hook(void* arg)
{
	CHook::Inline((void*)__android_log_write, (void*)My__android_log_write, (void**)&org__android_log_write);
	CHook::Inline((void*)__android_log_print, (void*)My__android_log_print, (void**)&org__android_log_print);
	CHook::Inline((void*)__android_log_vprint, (void*)My__android_log_vprint, (void**)&org__android_log_vprint);
	CHook::Inline((void*)__android_log_assert, (void*)My__android_log_assert, (void**)&org__android_log_assert);
	
	log_prio[ANDROID_LOG_DEFAULT]->WriteAllInfo(LOG_INFO, "XTest", "test init...", tLOCAL, tDEFAULT);
	__android_log_write(ANDROID_LOG_DEBUG, "XTest", "'__android_log_write' test");
	for (int i = 0; i < 9; i++)
		__android_log_print(i, "XTest", "'__android_log_print %d' test", i);

	return NULL;
}

static jstring GetPackageName(JNIEnv* env, jobject jActivity) //获取app包名
{
	jmethodID method = env->GetMethodID(env->GetObjectClass(jActivity), "getPackageName", "()Ljava/lang/String;");
	return (jstring)env->CallObjectMethod(jActivity, method);
}

static jobject GetGlobalContext(JNIEnv* env) //获取app线程的全局对象
{
	jclass activityThread = env->FindClass("android/app/ActivityThread");
	jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
	jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);
	jmethodID getApplication = env->GetMethodID(activityThread, "getApplication", "()Landroid/app/Application;");
	jobject context = env->CallObjectMethod(at, getApplication);
	return context;
}

static jobject GetStorageDir(JNIEnv* env) //获取安卓目录 /storage/emulated/0 instead of /sdcard (example)
{
	jclass classEnvironment = env->FindClass("android/os/Environment");
	return (jstring)env->CallStaticObjectMethod(classEnvironment, env->GetStaticMethodID(classEnvironment, "getExternalStorageDirectory", "()Ljava/io/File;"));
}

static jstring GetAbsolutePath(JNIEnv* env, jobject jFile) //转换为绝对路径
{
	jmethodID method = env->GetMethodID(env->GetObjectClass(jFile), "getAbsolutePath", "()Ljava/lang/String;");
	return (jstring)env->CallObjectMethod(jFile, method);
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
		return JNI_ERR;

	jstring PackageName = GetPackageName(env, GetGlobalContext(env));
	jstring StoragePach = GetAbsolutePath(env, GetStorageDir(env));
	const char* name = env->GetStringUTFChars(PackageName, NULL);
	const char* pach = env->GetStringUTFChars(StoragePach, NULL);
	char LogPach[255], iniPach[255];
	sprintf(LogPach, "%s/Android/data/%s/XLog.log", pach, name);
	sprintf(iniPach, "%s/Android/data/%s/XLog.ini", pach, name);
	inireader.SetIniPath(iniPach);
	if (access(iniPach, F_OK) == -1) {
		WriteNotes(NULL, "### XLog", "1.0 ### by: XMDS ###\n");
		inireader.WriteBoolean("XLog", "Enable", true);
		WriteNotes("XLog", "# (default Enable", "1) Set to enable or disable XLog. use 'y','Y','t','T','1' set Enable, 'n','N','f','F','0' is disable.\n");
		inireader.WriteBoolean("XLog", "Test", log_test);
		WriteNotes("XLog", "# (default Test", "0) Sets the XLog test text on and off. It is used for unknown app debugging. The log file will print the test text with the Tag name [XTest]. You can set 'AddFileTag1 = XTest' to view the test text alone. use 'y','Y','t','T','1' set Enable, 'n','N','f','F','0' is disable.\n");
		inireader.WriteInteger("Time", "Type", type);
		WriteNotes("Time", "# (default Type", "0) Set the time type for printing log. 0 is LOCAL, 1 is UTC.\n");
		inireader.WriteInteger("Time", "Style", style);
		WriteNotes("Time", "# (default Style", "0) Set the time style for printing log. 0 is hour:minute:second, 1 is year-month-day hour:minute:second, 2 is UTC Style.\n");
		inireader.WriteInteger("Filter", "VERBOSE", VERBOSE);
		inireader.WriteInteger("Filter", "DEBUG", DEBUG);
		inireader.WriteInteger("Filter", "INFO", INFO);
		inireader.WriteInteger("Filter", "WARN", WARN);
		inireader.WriteInteger("Filter", "ERROR", ERROR);
		inireader.WriteInteger("Filter", "FATAL", FATAL);
		inireader.WriteInteger("Filter", "SILENT", SILENT);
		WriteNotes("Filter", "# (default Prio", "0) Set the log to filter text at the Prio level (7 levels in total).\n# 0 is means no filtering, and all are printed to the XLog.log file by default.\n# -1 is to filter the text of this level, do not print.\n# 1 To create a new XLog[Prio].log file, print the text separately.\n# Example: INFO = 1. See: XLog[INFO].log\n");
		inireader.WriteInteger("Filter", "IgnoreNumTag", ignores);
		inireader.WriteString("Filter", "IgnoreTag1", "");
		inireader.WriteString("Filter", "IgnoreTag2", "");
		inireader.WriteString("Filter", "IgnoreTag3", "");
		WriteNotes("Filter", "# (default IgnoreNumTag", "0) Set the log to ignore messages with Tag names. 'IgnoreNumTag' sets the number of tag names that need to be ignored, and the option 'IgnoreTagID' at the back sets the tag names you want to ignore. The ID number increases in turn. The number of settings depends on 'IgnoreNumTag', which has no limit.\n# Example: IgnoreNumTag = 10, IgnoreTag1 = AndroidModLoader ... See: XLog will no longer print messages with the Tag name set above.\n");
		inireader.WriteInteger("Filter", "AddFileNumTag", addfile);
		inireader.WriteString("Filter", "AddFileTag1", "");
		inireader.WriteString("Filter", "AddFileTag2", "");
		inireader.WriteString("Filter", "AddFileTag3", "");
		WriteNotes("Filter", "# (default AddFileNumTag", "0) Set to create a new log file with a separate [Tag] name to print the text. 'AddFileNumTag' sets the number of [tag].log files you need to add, and then 'AddFileTagID' sets the tag name, and the number ID gradually increases. The number of files added depends on the number set by 'AddFileNumTag', which theoretically has no upper limit.\n# Example: AddFileNumTag = 10, AddFileTag1 = AB ... AddFileTag10 = AndroidModLoader, See: XLog-[AB].log, XLog-[AndroidModLoader].log, ...\n");
	}
	if (inireader.ReadBoolean("XLog", "Enable", false)) {
		log_test = inireader.ReadBoolean("XLog", "Test", log_test);
		
		static mylog lg(LogPach); //多余的写法，为什么不直接new到容器里。哈哈
		log_prio[ANDROID_LOG_UNKNOWN] = logs(&lg);
		log_prio[ANDROID_LOG_DEFAULT] = logs(&lg);
		type = inireader.ReadInteger("Time", "Type", type);
		style = inireader.ReadInteger("Time", "Style", style);
		
		static char* log_pach = new char[255];
		if ((VERBOSE = inireader.ReadInteger("Filter", "VERBOSE", VERBOSE)) == 1) {
			MakeLogPach(log_pach, "[VERBOSE].log");
			log_prio[ANDROID_LOG_VERBOSE] = logs(new mylog(log_pach));
		}
		if ((DEBUG = inireader.ReadInteger("Filter", "DEBUG", DEBUG)) == 1) {
			MakeLogPach(log_pach, "[DEBUG].log");
			log_prio[ANDROID_LOG_DEBUG] = logs(new mylog(log_pach));
		}
		if ((INFO = inireader.ReadInteger("Filter", "INFO", INFO)) == 1) {
			MakeLogPach(log_pach, "[INFO].log");
			log_prio[ANDROID_LOG_INFO] = logs(new mylog(log_pach));
		}
		if ((WARN = inireader.ReadInteger("Filter", "WARN", WARN)) == 1) {
			MakeLogPach(log_pach, "[WARN].log");
			log_prio[ANDROID_LOG_WARN] = logs(new mylog(log_pach));
		}
		if ((ERROR = inireader.ReadInteger("Filter", "ERROR", ERROR)) == 1) {
			MakeLogPach(log_pach, "[ERROR].log");
			log_prio[ANDROID_LOG_ERROR] = logs(new mylog(log_pach));
		}
		if ((FATAL = inireader.ReadInteger("Filter", "FATAL", FATAL)) == 1) {
			MakeLogPach(log_pach, "[FATAL].log");
			log_prio[ANDROID_LOG_FATAL] = logs(new mylog(log_pach));
		}
		if ((SILENT = inireader.ReadInteger("Filter", "SILENT", SILENT)) == 1) {
			MakeLogPach(log_pach, "[SILENTL].log");
			log_prio[ANDROID_LOG_SILENT] = logs(new mylog(log_pach));
		}
		
		ignores = inireader.ReadInteger("Filter", "IgnoreNumTag", ignores);
		addfile = inireader.ReadInteger("Filter", "AddFileNumTag", addfile);
		char tag[32], buf[255], fmt[255];
		for (int i = 0; i < ignores; i++) {
			sprintf(tag, "IgnoreTag%d", i + 1);
			ignore_tag[i] = inireader.ReadString("Filter", tag, NULL, buf, sizeof(buf));
		}
		for (int i = 0; i < addfile; i++) {
			sprintf(tag, "AddFileTag%d", i + 1);
			char* str = inireader.ReadString("Filter", tag, NULL, buf, sizeof(buf));
			sprintf(fmt, "-[%s].log", str);
			MakeLogPach(log_pach, fmt);
			log_tag[str] = logs(new mylog(log_pach));
		}
		
		char t1[255], t2[255];
		lg.Write("XLog 1.0 by: XMDS");
		lg.Write("Open Source: https://github.com/XMDS/XLog.git");
		lg.WriteFormat("Current Time: LOCAL: %s (UTC: %s)", lg.GetTime(t1, sizeof(t1), tLOCAL, tAll_1), lg.GetTime(t2, sizeof(t2), tUTC, tAll_2));
		lg.Write("");

		pthread_t id;
		pthread_create(&id, NULL, &my_hook, NULL);
		
		delete log_pach;
	}
	env->ReleaseStringUTFChars(PackageName, name);
	env->ReleaseStringUTFChars(StoragePach, pach);
	
	return JNI_VERSION_1_6;
}