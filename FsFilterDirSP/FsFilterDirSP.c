#include <fltKernel.h>
#include <dontuse.h>

PFLT_FILTER FilterHandle;

//****************************************
//* Function Declear
//****************************************
FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
//****************************************
//* Function Declear End
//****************************************

//****************************************
//* Filter Registration
//****************************************
const FLT_OPERATION_REGISTRATION CallBacks[] = {
    {IRP_MJ_CREATE,0,MiniPreCreate,MiniPostCreate},
    {IRP_MJ_WRITE,0,MiniPreWrite,NULL},
    {IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    0,
    NULL,
    CallBacks,
    MiniUnload,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
//****************************************
//* Filter Registration End
//****************************************

//****************************************
//* CallBack Function Implementation
//****************************************
FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
    PFLT_FILE_NAME_INFORMATION FileNameInfo;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

    if (NT_SUCCESS(status))
    {
        status = FltParseFileNameInformation(FileNameInfo);
        if (NT_SUCCESS(status))
        {
            UNICODE_STRING Name;
            UNICODE_STRING U_CheckName;
            PWCHAR CheckName = L"openme.txt";

            //U_CheckName这种初始化来的UNICODE_STRING后面不需要进行释放
            RtlInitUnicodeString(&U_CheckName, CheckName);

            //这种申请堆内存来的需要进行释放
            Name.Buffer = ExAllocatePoolWithTag(NonPagedPool, FileNameInfo->Name.MaximumLength * sizeof(WCHAR), 'pdsF');
            Name.Length = FileNameInfo->Name.Length;
            Name.MaximumLength = FileNameInfo->Name.MaximumLength;

            RtlCopyUnicodeString(&Name, &FileNameInfo->Name);

            if (wcsstr(Name.Buffer,U_CheckName.Buffer) != NULL)
            {
                KdPrint(("Write File: %wZ Blocked \r\n", Name));
                Data->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                Data->IoStatus.Information = 0;
                FltReleaseFileNameInformation(FileNameInfo);
                ExFreePoolWithTag(Name.Buffer, 'pdsF');
                return FLT_PREOP_COMPLETE;
            }
            KdPrint(("Wreate File: %wZ \r\n", Name));
            ExFreePoolWithTag(Name.Buffer, 'pdsF');
        }
        FltReleaseFileNameInformation(FileNameInfo);
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Data);

    KdPrint(("Post Create Entered. \r\n"));

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
    PFLT_FILE_NAME_INFORMATION FileNameInfo;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

    if (NT_SUCCESS(status))
    {
        status = FltParseFileNameInformation(FileNameInfo);
        if (NT_SUCCESS(status))
        {
            UNICODE_STRING Name;

            Name.Buffer = ExAllocatePoolWithTag(NonPagedPool, FileNameInfo->Name.MaximumLength * sizeof(WCHAR), 'pdsF');
            Name.Length = FileNameInfo->Name.Length;
            Name.MaximumLength = FileNameInfo->Name.MaximumLength;
            
            RtlCopyUnicodeString(&Name, &FileNameInfo->Name);
            KdPrint(("Create File: %wZ \r\n", Name));
            
            ExFreePoolWithTag(Name.Buffer, 'pdsF');
            KdPrint(("Pool Free Successful. \r\n"));
        }
        FltReleaseFileNameInformation(FileNameInfo);
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
//****************************************
//* CallBack Function Implementation End
//****************************************

//****************************************
//* FilterUnload Function
//****************************************
NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    KdPrint(("MiniUnload Entered.\r\n"));
    FltUnregisterFilter(FilterHandle);

    return STATUS_SUCCESS;
}
//****************************************
//* FilterUnload Function End
//****************************************

//****************************************
//* Driver Entry
//****************************************
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

    if (NT_SUCCESS(status))
    {
        status = FltStartFiltering(FilterHandle);

        if (!NT_SUCCESS(status))
        {
            FltUnregisterFilter(FilterHandle);
        }
    }

    return status;
}
//****************************************
//* Driver Entry End
//****************************************
