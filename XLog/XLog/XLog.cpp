#include <jni.h>
#include <pthread.h>
#include <string>
#include <map>
#include <memory>
#include "mylog.h"
#include "ARMHook/CHook.h"
#include "shared/ini/inireader.h"

using namespace ARMHook;
using namespace std;

static unsigned int type = tLOCAL;
static unsigned int style = tDEFAULT;
static int VERBOSE = 0;
static int DEBUG = 0;
static int INFO = 0;
static int WARN = 0;
static int ERROR = 0;
static int FATAL = 0;
static int SILENT = 0;
static int ignores = 0;
static int addfile = 0;
typedef std::shared_ptr<mylog> log;
std::map<int, string> ignore_tag;
std::map<string, log> log_tag;
std::map<int, log> log_prio;

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

	if (addfile > 0 && tag) {
		map<string, log>::iterator iter;
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
	if (level == 0) {
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
			func_type ? log_prio[prio]->WriteFormatAllInfo(prio, tag, (tType)type, (tStyle)style, text, arg) 
				: log_prio[prio]->WriteAllInfo(prio, tag, text, (tType)type, (tStyle)style);
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
	CHook::Internal((void*)__android_log_write, (void*)My__android_log_write, (void**)&org__android_log_write);
	CHook::Internal((void*)__android_log_print, (void*)My__android_log_print, (void**)&org__android_log_print);
	CHook::Internal((void*)__android_log_vprint, (void*)My__android_log_vprint, (void**)&org__android_log_vprint);
	CHook::Internal((void*)__android_log_assert, (void*)My__android_log_assert, (void**)&org__android_log_assert);
	return NULL;
}

static jstring GetPackageName(JNIEnv* env, jobject jActivity)
{
	jmethodID method = env->GetMethodID(env->GetObjectClass(jActivity), "getPackageName", "()Ljava/lang/String;");
	return (jstring)env->CallObjectMethod(jActivity, method);
}

static jobject GetGlobalContext(JNIEnv* env)
{
	jclass activityThread = env->FindClass("android/app/ActivityThread");
	jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
	jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);
	jmethodID getApplication = env->GetMethodID(activityThread, "getApplication", "()Landroid/app/Application;");
	jobject context = env->CallObjectMethod(at, getApplication);
	return context;
}

static jobject GetStorageDir(JNIEnv* env) // /storage/emulated/0 instead of /sdcard (example)
{
	jclass classEnvironment = env->FindClass("android/os/Environment");
	return (jstring)env->CallStaticObjectMethod(classEnvironment, env->GetStaticMethodID(classEnvironment, "getExternalStorageDirectory", "()Ljava/io/File;"));
}

static jstring GetAbsolutePath(JNIEnv* env, jobject jFile)
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
	static mylog lg(LogPach);
	log_prio[ANDROID_LOG_UNKNOWN] = log(&lg);
	log_prio[ANDROID_LOG_DEFAULT] = log(&lg);
	inireader.SetIniPath(iniPach);
	if (access(iniPach, F_OK) == -1) {
		inireader.WriteBoolean("XLog", "Enable", true);
		inireader.WriteInteger("Time", "Type", type);
		inireader.WriteInteger("Time", "Style", style);
		inireader.WriteInteger("Filter", "VERBOSE", VERBOSE);
		inireader.WriteInteger("Filter", "DEBUG", DEBUG);
		inireader.WriteInteger("Filter", "INFO", INFO);
		inireader.WriteInteger("Filter", "WARN", WARN);
		inireader.WriteInteger("Filter", "ERROR", ERROR);
		inireader.WriteInteger("Filter", "FATAL", FATAL);
		inireader.WriteInteger("Filter", "SILENT", SILENT);
		inireader.WriteInteger("Filter", "IgnoreNumTag", ignores);
		inireader.WriteInteger("Filter", "AddFileNumTag", addfile);
	}
	if (inireader.ReadBoolean("XLog", "Enable", false)) {
		type = inireader.ReadInteger("Time", "Type", type);
		style = inireader.ReadInteger("Time", "Style", style);
		
		static char* log_pach = new char[255];
		if ((VERBOSE = inireader.ReadInteger("Filter", "VERBOSE", VERBOSE)) == 1) {
			MakeLogPach(log_pach, "[VERBOSE].log");
			log_prio[ANDROID_LOG_VERBOSE] = log(new mylog(log_pach));
		}
		if ((DEBUG = inireader.ReadInteger("Filter", "DEBUG", DEBUG)) == 1) {
			MakeLogPach(log_pach, "[DEBUG].log");
			log_prio[ANDROID_LOG_DEBUG] = log(new mylog(log_pach));
		}
		if ((INFO = inireader.ReadInteger("Filter", "INFO", INFO)) == 1) {
			MakeLogPach(log_pach, "[INFO].log");
			log_prio[ANDROID_LOG_INFO] = log(new mylog(log_pach));
		}
		if ((WARN = inireader.ReadInteger("Filter", "WARN", WARN)) == 1) {
			MakeLogPach(log_pach, "[WARN].log");
			log_prio[ANDROID_LOG_WARN] = log(new mylog(log_pach));
		}
		if ((ERROR = inireader.ReadInteger("Filter", "ERROR", ERROR)) == 1) {
			MakeLogPach(log_pach, "[ERROR].log");
			log_prio[ANDROID_LOG_ERROR] = log(new mylog(log_pach));
		}
		if ((FATAL = inireader.ReadInteger("Filter", "FATAL", FATAL)) == 1) {
			MakeLogPach(log_pach, "[FATAL].log");
			log_prio[ANDROID_LOG_FATAL] = log(new mylog(log_pach));
		}
		if ((SILENT = inireader.ReadInteger("Filter", "SILENT", SILENT)) == 1) {
			MakeLogPach(log_pach, "[SILENTL].log");
			log_prio[ANDROID_LOG_SILENT] = log(new mylog(log_pach));
		}
		
		ignores = inireader.ReadInteger("Filter", "IgnoreNumTag", ignores);
		addfile = inireader.ReadInteger("Filter", "AddFileNumTag", addfile);
		static char tag[128], buf[255], fmt[255];
		for (int i = 0; i < ignores; i++) {
			sprintf(tag, "IgnoreTag%d", i + 1);
			ignore_tag[i] = inireader.ReadString("Filter", tag, NULL, buf, sizeof(buf));
		}
		for (int i = 0; i < addfile; i++) {
			sprintf(tag, "AddFileTag%d", i + 1);
			char* str = inireader.ReadString("Filter", tag, NULL, buf, sizeof(buf));
			sprintf(fmt, "-[%s].log", str);
			MakeLogPach(log_pach, fmt);
			lg.WriteFormat("%s", str);
			log_tag[str] = log(new mylog(log_pach));
		}
		pthread_t id;
		pthread_create(&id, NULL, &my_hook, NULL);
	}
	
	env->ReleaseStringUTFChars(PackageName, name);
	env->ReleaseStringUTFChars(StoragePach, pach);


	return JNI_VERSION_1_6;
}