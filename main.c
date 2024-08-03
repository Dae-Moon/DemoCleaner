#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shlobj.h>

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED) {
        //std::string tmp = (const char*)lpData;
        //std::cout << "path: " << tmp << std::endl;
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}

int BrowseFolder(const wchar_t* saved_path, wchar_t* out)
{
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = (L"Browse for folder...");
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)saved_path;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        //get the name of the folder and put it in path
        SHGetPathFromIDList(pidl, out);

        //free memory used
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->lpVtbl->Free(imalloc, pidl);
            imalloc->lpVtbl->Release(imalloc);
        }

        return 1;
    }

    return 0;
}

DWORD PrintLastError() {
    // Получаем код последней ошибки
    DWORD errorCode = GetLastError();

    // Буфер для хранения сообщения об ошибке
    LPWSTR errorMessage = NULL;
    
    // Форматируем сообщение об ошибке
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPWSTR)&errorMessage,
        0,
        NULL
    );

    // Выводим сообщение об ошибке
    if (errorMessage != NULL) {
        wprintf(L"Error %lu: %s\n", errorCode, errorMessage);
        // Освобождаем выделенную память
        LocalFree(errorMessage);
    }
    else {
        wprintf(L"The error message could not be received.\n");
    }

    return errorCode;
}

int delete_dem_files(const wchar_t* path) {

    if (path[0] == 0) {
        wprintf(L"Invalid path.\n");
        return 0;
    }

    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    wchar_t search_path[1024];
    wchar_t full_path[1024];

    // Create the search path for .dem files
    swprintf(search_path, sizeof(search_path) / sizeof(search_path[0]), L"%s\\*.dem", path);

    // Find the first file in the directory
    hFind = FindFirstFile(search_path, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        //wprintf(L"FindFirstFile failed (%d)\n", GetLastError());
        return PrintLastError() == ERROR_FILE_NOT_FOUND ? 1 : 0;
    }

    do {
        // Skip directories
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        // Create the full path to the file
        swprintf(full_path, sizeof(full_path) / sizeof(full_path[0]), L"%s\\%s", path, findFileData.cFileName);

        // Attempt to delete the file
        if (DeleteFile(full_path)) {
            wprintf(L"Deleted: %s\n", findFileData.cFileName);
        }
        else {
            wprintf(L"Failed to delete %s (%d)\n", full_path, GetLastError());
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    // Close the find handle
    FindClose(hFind);
    return 1;
}

#define RESTART getch(); goto restart
int main(int argc, wchar_t* argv[])
{
    wchar_t path[1024];
    wchar_t currentDirectory[MAX_PATH];

    if (GetCurrentDirectory(MAX_PATH, currentDirectory) == 0) {
        PrintLastError();
        return 0;
    }

restart:
    system("cls");
    
    if (argc == 2) {
        wcscpy_s(path, _countof(path), argv[1]);
    } else if (BrowseFolder(currentDirectory, path) == 0) {

        // Read the directory path from the console
        wprintf(L"Enter the directory path: ");
        if (fgetws(path, _countof(path), stdin) == NULL) {
            wprintf(L"Error reading input.\n");
            RESTART;
        }

        // Remove the newline character if present
        size_t len = wcslen(path);
        if (len > 0 && path[len - 1] == L'\n') {
            path[len - 1] = L'\0';
        }
    }

    if (delete_dem_files(path) == 0) {
        RESTART;
    }

    wprintf(L"Do you want to open a folder? (N/Y)");
    if (tolower(getch()) == 'y') {
        HINSTANCE result = ShellExecute(NULL, L"explore", path, NULL, NULL, SW_SHOW);

        if ((int)result <= 32) {
            //wprintf(L"Ошибка при открытии папки: %ld\n", (long)result);
            PrintLastError();
            RESTART;
        }
    }

	goto restart;
    return 0;
}