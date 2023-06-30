#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>

#define LOG_FILE "place route to log.txt here"
#define CONF_FILE "place route to config.txt here"

LPCSTR serviceName = "Service"; //Servcice name
LPCSTR servicePath = "place route to service.exe here";

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle;

std::string cmd_7z;

int addLogMessage(const char* str);
void ControlHandler(DWORD request);
int InstallService();
int new_StartService();
int InitService();
int StopService();
int RemoveService();
void ServiceMain(int argc, char** argv);

int main(int argc, char* argv[]) {
	int ret;
	if (argc - 1 == 0) {
		SERVICE_TABLE_ENTRY ServiceTable[2];
		ServiceTable[0].lpServiceName = (LPWSTR)serviceName;
		ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
		ServiceTable[1].lpServiceName = NULL;
		ServiceTable[1].lpServiceProc = NULL;
		if (!StartServiceCtrlDispatcher(ServiceTable))
		{
			addLogMessage("StartServiceCtrlDispatcher ERROR");
		}
	}
	else if (strcmp(argv[argc - 1], "install") == 0)
	{
		printf("Installing Service...\n");
		ret = InstallService();
	}
	else if (strcmp(argv[argc - 1], "remove") == 0)
	{
		printf("Removing Service...\n");
		ret = RemoveService();
	}
	else if (strcmp(argv[argc - 1], "start") == 0)
	{
		printf("Starting Service...\n");
		ret = new_StartService();
	}
	else if (strcmp(argv[argc - 1], "stop") == 0)
	{
		printf("Stoping Service...\n");
		ret = StopService();
	}
}

int addLogMessage(const char* str)
{
	errno_t err;
	FILE* log;
	if ((err = fopen_s(&log, LOG_FILE, "a+")) != 0)
	{
		return -1;
	}
	fprintf(log, "LOG_MESSAGE: %s\n", str);
	fclose(log);
	return 0;
}

int InstallService()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!hSCManager)
	{
		addLogMessage("Can't open SCManager.");
		return -1;
	}

	SC_HANDLE hService = CreateServiceA(hSCManager,
		serviceName,
		serviceName,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		servicePath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	if (!hService)
	{
		int err = GetLastError();
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
			addLogMessage("ERROR: ERROR_ACCESS_DENIED");
			break;

		case ERROR_CIRCULAR_DEPENDENCY:
			addLogMessage("ERROR: ERROR_CIRCULAR_DEPENDENCY");
			break;

		case ERROR_DUPLICATE_SERVICE_NAME:
			addLogMessage("ERROR: ERROR_DUPLICATE_SERVICE_NAME");
			break;

		case ERROR_INVALID_HANDLE:
			addLogMessage("ERROR: ERROR_INVALID_HANDLE");
			break;

		case ERROR_INVALID_NAME:
			addLogMessage("ERROR: ERROR_INVALID_NAME");
			break;

		case ERROR_INVALID_PARAMETER:
			addLogMessage("ERROR: ERROR_INVALID_PARAMETER");
			break;

		case ERROR_INVALID_SERVICE_ACCOUNT:
			addLogMessage("ERROR: ERROR_INVALID_SERVICE_ACCOUNT");
			break;

		case ERROR_SERVICE_EXISTS:
			addLogMessage("ERROR: ERROR_SERVICE_EXISTS");
			break;

		default:
			addLogMessage("ERROR: UNKNOWN");
		}

		CloseServiceHandle(hSCManager);
		return -1;
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	addLogMessage("Service is installed successfully.");
	return 0;
}

int InitService()
{
	std::string str;
	std::string backup_dir;
	std::string arch_name;
	char buf[1000] = { 0 };
	FILE* confFile = NULL;
	addLogMessage("Start InitService()");
	if (fopen_s(&confFile, CONF_FILE, "r") != 0)
	{
		addLogMessage("ERROR: can't open conf_file");
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		serviceStatus.dwWin32ExitCode = -1;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return -1;
	}
	fgets(buf, sizeof(buf), confFile);
	buf[strlen(buf) - 1] = 0;
	backup_dir = buf;
	fgets(buf, sizeof(buf), confFile);
	buf[strlen(buf) - 1] = 0;
	arch_name = buf;
	str = "Backup dir: " + backup_dir;
	addLogMessage(str.c_str());
	str = "Result archive name: " + arch_name;
	addLogMessage(str.c_str());
	cmd_7z = std::string(" \"C:\\Program Files\\7-Zip\\7z.exe\" u ") + arch_name + std::string(" ");
	while (fgets(buf, sizeof(buf), confFile) != NULL) {
		buf[strlen(buf) - 1] = 0;
		cmd_7z += backup_dir + std::string("\\") + buf + std::string(" ");
	}
	cmd_7z += std::string("-tzip -ssw -r");
	str = "7z command:" + cmd_7z;
	addLogMessage(str.c_str());
	return 0;
}

void ServiceMain(int argc, char** argv)
{
	int i = 0;
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;
	serviceStatusHandle = RegisterServiceCtrlHandlerA(serviceName, (LPHANDLER_FUNCTION)ControlHandler);
	if (serviceStatusHandle == (SERVICE_STATUS_HANDLE)0)
	{
		return;
	}
	int error = InitService();
	if (error)
	{
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		serviceStatus.dwWin32ExitCode = -1;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;
	}
	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);
	while (serviceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		system(cmd_7z.c_str());
		addLogMessage("Backup is complete succesfully.");
		Sleep(5000);
	}
	return;
}



int StopService()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	SC_HANDLE hService = OpenServiceA(hSCManager, serviceName, SERVICE_STOP);
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		int err = GetLastError();
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
			addLogMessage("ControlService -- ERROR_ACCESS_DENIED");
			break;
		case ERROR_INVALID_HANDLE:
			addLogMessage("ControlService -- ERROR_INVALID_HANDLE");
			break;
		case ERROR_PATH_NOT_FOUND:
			addLogMessage("ControlService -- ERROR_PATH_NOT_FOUND");
			break;
		case ERROR_SERVICE_ALREADY_RUNNING:
			addLogMessage("ControlService -- ERROR_SERVICE_ALREADY_RUNNING");
			break;
		case ERROR_SERVICE_DATABASE_LOCKED:
			addLogMessage("ControlService -- ERROR_SERVICE_DATABASE_LOCKED");
			break;
		case ERROR_SERVICE_DEPENDENCY_DELETED:
			addLogMessage("ControlService -- ERROR_SERVICE_DEPENDENCY_DELETED");
			break;
		case ERROR_SERVICE_DEPENDENCY_FAIL:
			addLogMessage("ControlService -- ERROR_SERVICE_DEPENDENCY_FAIL");
			break;
		case ERROR_SERVICE_DISABLED:
			addLogMessage("ControlService -- ERROR_SERVICE_DISABLED");
			break;
		case ERROR_SERVICE_LOGON_FAILED:
			addLogMessage("ControlService -- ERROR_SERVICE_LOGON_FAILED");
			break;
		case ERROR_SERVICE_MARKED_FOR_DELETE:
			addLogMessage("ControlService -- ERROR_SERVICE_MARKED_FOR_DELETE");
			break;
		case ERROR_SERVICE_NO_THREAD:
			addLogMessage("ControlService -- ERROR_SERVICE_NO_THREAD");
			break;
		case ERROR_SERVICE_REQUEST_TIMEOUT:
			addLogMessage("ControlService -- ERROR_SERVICE_REQUEST_TIMEOUT");
			break;
		default:
			addLogMessage("ControlService -- UNKNOWN ERROR");
			break;
		}
		CloseServiceHandle(hSCManager);
		return -1;
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return 0;
}

void ControlHandler(DWORD request)
{
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		addLogMessage("Service is stopped.");

		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;

	case SERVICE_CONTROL_SHUTDOWN:
		addLogMessage("Shutdown service...");

		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;

	default:
		break;
	}

	SetServiceStatus(serviceStatusHandle, &serviceStatus);

	return;
}

int RemoveService()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)
	{
		addLogMessage("Can't open SCManager.");
		return -1;
	}
	SC_HANDLE hService = OpenServiceA(hSCManager, serviceName, SERVICE_STOP | DELETE);
	if (!hService)
	{
		addLogMessage("Can't remove service");
		CloseServiceHandle(hSCManager);
		return -1;
	}
	DeleteService(hService);
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	addLogMessage("Service is removed successfully.");
	return 0;
}

int new_StartService()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (hSCManager == NULL)
	{
		printf("OpenSCManager error");
	}
	SC_HANDLE hService = OpenServiceA(hSCManager, serviceName, SERVICE_START);
	if (hService == NULL)
	{
		printf("Can't open ServiceA");
	}
	if (!StartService(hService, 0, NULL))
	{
		int err = GetLastError();
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
			addLogMessage("StartService -- ERROR_ACCESS_DENIED");
			break;
		case ERROR_INVALID_HANDLE:
			addLogMessage("StartService -- ERROR_INVALID_HANDLE");
			break;
		case ERROR_PATH_NOT_FOUND:
			addLogMessage("StartService -- ERROR_PATH_NOT_FOUND");
			break;
		case ERROR_SERVICE_ALREADY_RUNNING:
			addLogMessage("StartService -- ERROR_SERVICE_ALREADY_RUNNING");
			break;
		case ERROR_SERVICE_DATABASE_LOCKED:
			addLogMessage("StartService -- ERROR_SERVICE_DATABASE_LOCKED");
			break;
		case ERROR_SERVICE_DEPENDENCY_DELETED:
			addLogMessage("StartService -- ERROR_SERVICE_DEPENDENCY_DELETED");
			break;
		case ERROR_SERVICE_DEPENDENCY_FAIL:
			addLogMessage("StartService -- ERROR_SERVICE_DEPENDENCY_FAIL");
			break;
		case ERROR_SERVICE_DISABLED:
			addLogMessage("StartService -- ERROR_SERVICE_DISABLED");
			break;
		case ERROR_SERVICE_LOGON_FAILED:
			addLogMessage("StartService -- returned ERROR_SERVICE_LOGON_FAILED");
			break;
		case ERROR_SERVICE_MARKED_FOR_DELETE:
			addLogMessage("StartService -- ERROR_SERVICE_MARKED_FOR_DELETE");
			break;
		case ERROR_SERVICE_NO_THREAD:
			addLogMessage("StartService -- ERROR_SERVICE_NO_THREAD");
			break;
		case ERROR_SERVICE_REQUEST_TIMEOUT:
			addLogMessage("StartService -- ERROR_SERVICE_REQUEST_TIMEOUT");
			break;
		default:
			addLogMessage("StartService -- UNKNOWN ERROR");
			break;
		}
		CloseServiceHandle(hSCManager);
		return -1;
	}
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return 0;
}