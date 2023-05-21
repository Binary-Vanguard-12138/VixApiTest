// VixTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "vix.h"
/*
 * Certain arguments differ when using VIX with VMware Server 2.0
 * and VMware Workstation.
 *
 * Comment out this definition to use this code with VMware Server 2.0.
 */
#define USE_WORKSTATION

#define  TOOLS_TIMEOUT  300
#define  GUEST_USERNAME "User"
#define  GUEST_PASSWORD "123456"

#ifdef _WIN32
#define DEST_FILE ".\\outfile"
#else
#define DEST_FILE "./outfile.txt"
#endif

#ifdef USE_WORKSTATION

#define  CONNTYPE    VIX_SERVICEPROVIDER_VMWARE_WORKSTATION

#define  HOSTNAME ""
#define  HOSTPORT 0
#define  USERNAME ""
#define  PASSWORD ""

#define  VMPOWEROPTIONS   VIX_VMPOWEROP_LAUNCH_GUI   // Launches the VMware Workstaion UI
 // when powering on the virtual machine.

#define VMXPATH_INFO "where vmxpath is an absolute path to the .vmx file " \
                     "for the virtual machine."

#else    // USE_WORKSTATION

 /*
  * For VMware Server 2.0
  */

#define CONNTYPE VIX_SERVICEPROVIDER_VMWARE_VI_SERVER

#define HOSTNAME "https://192.2.3.4:8333/sdk"
  /*
   * NOTE: HOSTPORT is ignored, so the port should be specified as part
   * of the URL.
   */
#define HOSTPORT 0
#define USERNAME "root"
#define PASSWORD "hideme"

#define  VMPOWEROPTIONS VIX_VMPOWEROP_NORMAL

#define VMXPATH_INFO "where vmxpath is a datastore-relative path to the " \
                     ".vmx file for the virtual machine, such as "        \
                     "\"[standard] ubuntu/ubuntu.vmx\"."

#endif    // USE_WORKSTATION


   /*
	* Global variables.
	*/

static char* progName;


/*
 * Local functions.
 */

 ////////////////////////////////////////////////////////////////////////////////
static void
usage()
{
	fprintf(stderr, "Usage: %s <vmxpath>\n", progName);
	fprintf(stderr, "%s\n", VMXPATH_INFO);
}

int main(int argc, char* argv[])
{
	VixError err;
	char* vmxPath;
	VixHandle hostHandle = VIX_INVALID_HANDLE;
	VixHandle jobHandle = VIX_INVALID_HANDLE;
	VixHandle vmHandle = VIX_INVALID_HANDLE;

	progName = argv[0];
	if (argc > 1) {
		vmxPath = argv[1];
	}
	else {
		usage();
		exit(EXIT_FAILURE);
	}

	char szTempPath[MAX_PATH] = {};
	int nProcNum = 0, iProc = 0;

	jobHandle = VixHost_Connect(VIX_API_VERSION,
		CONNTYPE,
		HOSTNAME, // *hostName,
		HOSTPORT, // hostPort,
		USERNAME, // *userName,
		PASSWORD, // *password,
		0, // options,
		VIX_INVALID_HANDLE, // propertyListHandle,
		NULL, // *callbackProc,
		NULL); // *clientData);
	err = VixJob_Wait(jobHandle,
		VIX_PROPERTY_JOB_RESULT_HANDLE,
		&hostHandle,
		VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		goto abort;
	}

	jobHandle = VixVM_Open(hostHandle,
		vmxPath,
		NULL, // VixEventProc *callbackProc,
		NULL); // void *clientData);
	err = VixJob_Wait(jobHandle,
		VIX_PROPERTY_JOB_RESULT_HANDLE,
		&vmHandle,
		VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		goto abort;
	}

	jobHandle = VixVM_PowerOn(vmHandle,
		VMPOWEROPTIONS,
		VIX_INVALID_HANDLE,
		NULL, // *callbackProc,
		NULL); // *clientData);
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		goto abort;
	}

	printf("waiting for tools\n");
	jobHandle = VixVM_WaitForToolsInGuest(vmHandle,
		TOOLS_TIMEOUT,     // timeout in secs
		NULL,              // callback
		NULL);             // client data
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		fprintf(stderr, "failed to wait for tools in virtual machine (%lld %s)\n", err,
			Vix_GetErrorText(err, NULL));
		goto abort;
	}
	printf("tools up\n");

	jobHandle = VixVM_LoginInGuest(vmHandle,
		GUEST_USERNAME,           // guest OS user
		GUEST_PASSWORD,           // guest OS passwd
		0,                        // options
		NULL,                     // callback
		NULL);                    // client data
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		fprintf(stderr, "failed to login to virtual machine '%s'(%lld %s)\n", vmxPath, err,
			Vix_GetErrorText(err, NULL));
		goto abort;
	}
	printf("logged in to guest\n");

	printf("about to do work\n");
	jobHandle = VixVM_RunProgramInGuest(vmHandle,
		"cmd.exe",                // command
		" ping 8.8.8.8 -t",   // cmd args
		VIX_RUNPROGRAM_RETURN_IMMEDIATELY | VIX_RUNPROGRAM_ACTIVATE_WINDOW,                        // options
		VIX_INVALID_HANDLE,       // prop handle
		NULL,                     // callback
		NULL);                    // client data
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		fprintf(stderr, "failed to run program in virtual machine '%s'(%lld %s)\n",
			vmxPath, err, Vix_GetErrorText(err, NULL));
		goto abort;
	}

	jobHandle = VixVM_CopyFileFromHostToGuest(vmHandle,
		"D:\\Test.txt",     // src file
		"D:\\Test.txt",          // dst file
		0,                  // options
		VIX_INVALID_HANDLE, // prop list
		NULL,               // callback
		NULL);              // client data
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		fprintf(stderr, "failed to copy file to the host '%s'(%lld %s)\n",
			vmxPath, err, Vix_GetErrorText(err, NULL));
		goto abort;
	}

	jobHandle = VixVM_CopyFileFromGuestToHost(vmHandle,
		"D:\\Test.txt",     // src file
		"D:\\Test.txt",          // dst file
		0,                  // options
		VIX_INVALID_HANDLE, // prop list
		NULL,               // callback
		NULL);              // client data
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		fprintf(stderr, "failed to copy file to the host '%s'(%lld %s)\n",
			vmxPath, err, Vix_GetErrorText(err, NULL));
		goto abort;
	}
	
	jobHandle = VixVM_DeleteFileInGuest(vmHandle,
		"D:\\Test.txt",           // filepath
		NULL,                     // callback
		NULL);                    // client data
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		fprintf(stderr, "failed to delete file in virtual machine '%s'(%lld %s)\n",
			vmxPath, err, Vix_GetErrorText(err, NULL));
		goto abort;
	}

	jobHandle = VixVM_ListProcessesInGuest(vmHandle,
		0,                        // options
		NULL,                     // callback
		NULL);                    // client data
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if (VIX_FAILED(err)) {
		fprintf(stderr, "failed to delete file in virtual machine '%s'(%lld %s)\n",
			vmxPath, err, Vix_GetErrorText(err, NULL));
		Vix_ReleaseHandle(jobHandle);
		goto abort;
	}
	nProcNum = VixJob_GetNumProperties(jobHandle, VIX_PROPERTY_JOB_RESULT_ITEM_NAME);
	for (iProc = 0; iProc < nProcNum; iProc ++) {
		char* pname = NULL, *cmd = NULL, *owner = NULL;
		uint64 pid = 0;

		err = VixJob_GetNthProperties(jobHandle,
			iProc,
			VIX_PROPERTY_JOB_RESULT_ITEM_NAME, &pname,
			VIX_PROPERTY_JOB_RESULT_PROCESS_ID, &pid,
			VIX_PROPERTY_JOB_RESULT_PROCESS_OWNER, &owner,
			VIX_PROPERTY_JOB_RESULT_PROCESS_COMMAND, &cmd,
			VIX_PROPERTY_NONE);

		printf("%lld '%s' '%s' '%s'\n", pid, pname, owner, cmd);

		Vix_FreeBuffer(pname);
		Vix_FreeBuffer(cmd);
	}
	Vix_ReleaseHandle(jobHandle);

#if 0
	jobHandle = VixVM_PowerOff(vmHandle,
		VIX_VMPOWEROP_NORMAL,
		NULL, // *callbackProc,
		NULL); // *clientData);
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(jobHandle);
	if (VIX_FAILED(err)) {
		goto abort;
	}
#endif

abort:
	Vix_ReleaseHandle(vmHandle);
	VixHost_Disconnect(hostHandle);

	return 0;
}
