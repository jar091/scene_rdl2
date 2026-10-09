// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#include "Dso.h"

#include "SceneClass.h"
#include "SceneObject.h"
#include "Types.h"

#include <scene_rdl2/common/platform/Platform.h>
#include <scene_rdl2/common/except/exceptions.h>
#include <scene_rdl2/render/util/Files.h>
#include <scene_rdl2/render/util/Strings.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <libgen.h>
#include <unistd.h>
#endif
#include <filesystem>

namespace scene_rdl2 {
namespace rdl2 {

namespace {

// Minimal loader shim: same call shape on every platform, so the class
// bodies below stay branch-free. The plugin suffix is a platform property
// of the producer (CMake MODULE libraries), not of this loader.
#if defined(_WIN32)

const char* const sDsoExtension = ".dll";
const char* const sDsoProxyExtension = ".dll.proxy";

std::wstring utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) {
        return std::wstring();
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                        static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                        &wide[0], len);
    return wide;
}

void* dsoOpen(const std::string& path)
{
    const std::wstring widePath = utf8ToWide(path);
    HMODULE handle = LoadLibraryExW(widePath.c_str(), nullptr,
                                    LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                                        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    if (handle == nullptr) {
        // Relative paths (no directory component) are rejected by
        // LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR; retry with default semantics.
        handle = LoadLibraryExW(widePath.c_str(), nullptr, 0);
    }
    return static_cast<void*>(handle);
}

void* dsoSymbol(void* handle, const char* name)
{
    return reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(handle), name));
}

void dsoClose(void* handle)
{
    FreeLibrary(static_cast<HMODULE>(handle));
}

std::string dsoError()
{
    const DWORD code = GetLastError();
    LPSTR buffer = nullptr;
    const DWORD len = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    std::string message = "Win32 error " + std::to_string(code);
    if (len != 0 && buffer != nullptr) {
        message += ": ";
        message.append(buffer, len);
        LocalFree(buffer);
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
    }
    return message;
}

#else

const char* const sDsoExtension = ".so";
const char* const sDsoProxyExtension = ".so.proxy";

void* dsoOpen(const std::string& path)
{
    return dlopen(path.c_str(), RTLD_LAZY);
}

void* dsoSymbol(void* handle, const char* name)
{
    dlerror(); // clear any stale error before the lookup
    return dlsym(handle, name);
}

void dsoClose(void* handle)
{
    dlclose(handle);
}

std::string dsoError()
{
    const char* error = dlerror();
    return error ? std::string(error) : std::string();
}

#endif

} // namespace

namespace internal {

std::string
classNameFromFileName(const std::string& baseName,
                      const std::string& expectedExtension)
{
    // Base name must be at least expectedExtension + 1 chars ("a<extension>").
    if (baseName.size() < expectedExtension.size() + 1) {
        return "";
    }

    // Final characters must match expectedExtension.
    std::string extension =
        baseName.substr(baseName.size() - expectedExtension.size(), std::string::npos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension != expectedExtension) {
        return "";
    }

    // Class name is the file name without the extension.
    return baseName.substr(0, baseName.size() - expectedExtension.size());
}

} // namespace internal

/* static */
std::string
Dso::classNameFromFileName(const std::string& filePath)
{
    std::filesystem::path p(filePath);
    std::string directory(p.parent_path().string());
    std::string baseName(p.stem().string());

    // Bail early if we can't determine the class name.

    std::string className(internal::classNameFromFileName(baseName, sDsoProxyExtension));
    if (className.empty()) {
        className = internal::classNameFromFileName(baseName, sDsoExtension);
    }
    return className;
}


Dso::Dso(const std::string& className, const std::string& searchPath, bool proxyModeEnabled) :
    mHandle(nullptr),
    mDeclareFunc(nullptr),
    mCreateFunc(nullptr),
    mDestroyFunc(nullptr)
{
    MNRY_ASSERT(!className.empty(), "Dso must be constructed with a non-empty SceneClass name.");
    mFilePath = className + sDsoExtension;

    if (proxyModeEnabled) {
        mFilePath += ".proxy";
    }

    // We need a valid path before we try to dlopen() it.
    if (searchPath.empty()) {
        mFilePath = util::findFile(mFilePath, ".");
    } else {
        mFilePath = util::findFile(mFilePath, searchPath);
    }

    // If the file path is empty, we couldn't find it.
    if (mFilePath.empty()) {
        throw except::IoError(util::buildString("Couldn't find DSO for '",
                className, "' in search path '", searchPath, "'."));
    }

    // Attempt to open the DSO.
    mHandle = dsoOpen(mFilePath);
    if (!mHandle) {
        std::stringstream errMsg;
        errMsg << "Found RDL2 DSO '" << mFilePath << "', but failed to"
            " open it";
        const std::string error = dsoError();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }
}

Dso::~Dso()
{
    if (mHandle) {
#if defined(_WIN32)
        // Never unmap the module while an exception is unwinding: the
        // in-flight exception object's vtable/type_info may live in this
        // module (e.g. a DSO whose declare() threw, as the proxy tests
        // exercise). Leaking the module handle is harmless; touching a
        // freed image inside the catch handler is not.
        if (std::uncaught_exceptions() > 0) {
            mHandle = nullptr;
            return;
        }
#endif
        dsoClose(mHandle);
    }
}

ClassDeclareFunc
Dso::getDeclare()
{
    // Return cached function pointer if we already looked it up.
    if (mDeclareFunc) {
        return mDeclareFunc;
    }

    // Attempt to load the rdl_declare symbol.
    MNRY_ASSERT(mHandle, "Tried to load symbol from bad DSO handle.");
    void* declareSymbol = dsoSymbol(mHandle, "rdl2_declare");
    if (!declareSymbol) {
        // Technically null symbols are valid, but a null function pointer
        // is useless to us, so that's really a failure case, too.
        std::stringstream errMsg;
        errMsg << "Failed to load symbol 'rdl2_declare' from RDL2 DSO '" <<
            mFilePath << "'";
        const std::string error = dsoError();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }

    // Cache the function pointer.
    mDeclareFunc = (ClassDeclareFunc)declareSymbol;

    return mDeclareFunc;
}

ObjectCreateFunc
Dso::getCreate()
{
    // Return cached function pointer if we already looked it up.
    if (mCreateFunc) {
        return mCreateFunc;
    }

    // Attempt to load the rdl_create symbol.
    MNRY_ASSERT(mHandle, "Tried to load symbol from bad DSO handle.");
    void* createSymbol = dsoSymbol(mHandle, "rdl2_create");
    if (!createSymbol) {
        // Technically null symbols are valid, but a null function pointer
        // is useless to us, so that's really a failure case, too.
        std::stringstream errMsg;
        errMsg << "Failed to load symbol 'rdl2_create' from RDL2 DSO '" <<
            mFilePath << "'";
        const std::string error = dsoError();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }

    // Cache the function pointer.
    mCreateFunc = (ObjectCreateFunc)createSymbol;

    return mCreateFunc;
}

ObjectDestroyFunc
Dso::getDestroy()
{
    // Return the cached function pointer if we already looked it up.
    if (mDestroyFunc) {
        return mDestroyFunc;
    }

    // Attempt to load the rdl_destroy symbol.
    MNRY_ASSERT(mHandle, "Tried to load symbol from bad DSO handle.");
    void* destroySymbol = dsoSymbol(mHandle, "rdl2_destroy");
    if (!destroySymbol) {
        // Technically null symbols are valid, but a null function pointer
        // is useless to us, so that's really a failure case, too.
        std::stringstream errMsg;
        errMsg << "Failed to load symbol 'rdl2_destroy' from RDL2 DSO '" <<
            mFilePath << "'";
        const std::string error = dsoError();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }

    // Cache the function pointer.
    mDestroyFunc = (ObjectDestroyFunc)destroySymbol;

    return mDestroyFunc;
}

bool
Dso::isValidDso(const std::string& filePath, bool proxyModeEnabled)
{
    // Break the path into directory and basename components.
    std::filesystem::path p = filePath;
    std::string directory(p.parent_path().string());
    std::string baseName(p.filename().string());

    // Bail early if we can't determine the class name.
    const char* extension = (proxyModeEnabled) ? sDsoProxyExtension : sDsoExtension;
    std::string className = internal::classNameFromFileName(baseName, extension);
    if (className.empty()) {
        return false;
    }

    // Attempt to load it as an RDL Dso object and get the expected function
    // pointers.
    try {
        std::unique_ptr<Dso> dso(new Dso(className, directory, proxyModeEnabled));
        dso->getDeclare();
        if (!proxyModeEnabled) {
            dso->getCreate();
            dso->getDestroy();
        }
    } catch (...) {
        return false;
    }

    // It's a valid DSO.
    return true;
}

} // namespace rdl2
} // namespace scene_rdl2

