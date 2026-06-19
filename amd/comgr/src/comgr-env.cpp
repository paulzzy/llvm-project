//===- comgr-env.cpp - Comgr environment variables ------------------------===//
//
// Part of Comgr, under the Apache License v2.0 with LLVM Exceptions. See
// amd/comgr/LICENSE.TXT in this repository for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the management of Comgr's environment variables. See
/// amd/comgr/README.md for descriptions of these.
///
//===----------------------------------------------------------------------===//

#include "comgr-env.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"

#ifndef _WIN32
#include <dlfcn.h>
#endif

using namespace llvm;

namespace COMGR {
namespace env {

bool shouldSaveTemps() {
  static char *SaveTemps = getenv("AMD_COMGR_SAVE_TEMPS");
  return SaveTemps && StringRef(SaveTemps) != "0";
}

bool shouldSaveLLVMTemps() {
  static char *SaveTemps = getenv("AMD_COMGR_SAVE_LLVM_TEMPS");
  return SaveTemps && StringRef(SaveTemps) != "0";
}

std::optional<bool> shouldUseVFS() {
  if (shouldSaveTemps())
    return false;

  static char *UseVFS = getenv("AMD_COMGR_USE_VFS");
  if (UseVFS) {
    if (StringRef(UseVFS) == "0")
      return false;
    else if (StringRef(UseVFS) == "1")
      return true;
  }

  return std::nullopt;
}

std::optional<StringRef> getRedirectLogs() {
  static char *RedirectLogs = getenv("AMD_COMGR_REDIRECT_LOGS");
  if (!RedirectLogs || StringRef(RedirectLogs) == "0") {
    return std::nullopt;
  }
  return StringRef(RedirectLogs);
}

bool needTimeStatistics() {
  static char *TimeStatistics = getenv("AMD_COMGR_TIME_STATISTICS");
  return TimeStatistics && StringRef(TimeStatistics) != "0";
}

bool shouldEmitVerboseLogs() {
  static char *VerboseLogs = getenv("AMD_COMGR_EMIT_VERBOSE_LOGS");
  return VerboseLogs && StringRef(VerboseLogs) != "0";
}

llvm::StringRef getLLVMPath() {
  static const char *EnvLLVMPath = std::getenv("LLVM_PATH");
  return EnvLLVMPath;
}

// Probe whether path P names a clang binary whose derived resource directory
// exists on disk. The binary itself need not exist; clang's Driver only uses
// the path to derive the resource dir.
static bool probeClangResourceDir(StringRef P) {
  SmallString<256> ResourceDir(
      sys::path::parent_path(sys::path::parent_path(P)));
  sys::path::append(ResourceDir, "lib", "clang");
  return sys::fs::is_directory(ResourceDir);
}

std::string getClangBinaryPath() {
  static const std::string Cached = []() -> std::string {
    const char *EnvLLVMPath = std::getenv("LLVM_PATH");
    if (EnvLLVMPath && StringRef(EnvLLVMPath) != "")
      return (Twine(EnvLLVMPath) + "/bin/clang").str();

#ifndef _WIN32
    Dl_info Info;
    if (dladdr(reinterpret_cast<void *>(&getClangBinaryPath), &Info) &&
        Info.dli_fname) {
      StringRef SoDir = sys::path::parent_path(Info.dli_fname);

      SmallString<256> RocmLayout(sys::path::parent_path(SoDir));
      sys::path::append(RocmLayout, "llvm", "bin", "clang");
      if (probeClangResourceDir(RocmLayout))
        return std::string(RocmLayout);

      SmallString<256> StandardLayout(sys::path::parent_path(SoDir));
      sys::path::append(StandardLayout, "bin", "clang");
      if (probeClangResourceDir(StandardLayout))
        return std::string(StandardLayout);
    }
#endif

    return std::string("/bin/clang");
  }();
  return Cached;
}

StringRef getCachePolicy() {
  static const char *EnvCachePolicy = std::getenv("AMD_COMGR_CACHE_POLICY");
  return EnvCachePolicy;
}

StringRef getCacheDirectory() {
  // By default the cache is enabled
  static const char *Enable = std::getenv("AMD_COMGR_CACHE");
  bool CacheDisabled = StringRef(Enable) == "0";
  if (CacheDisabled)
    return "";

  StringRef EnvCacheDirectory = std::getenv("AMD_COMGR_CACHE_DIR");
  if (!EnvCacheDirectory.empty())
    return EnvCacheDirectory;

  // mark Result as static to keep it cached across calls
  static SmallString<256> Result;
  if (!Result.empty())
    return Result;

  if (sys::path::cache_directory(Result)) {
    sys::path::append(Result, "comgr");
    return Result;
  }

  return "";
}

StringRef getDriverOptionsAppend() {
  static const char *Options = std::getenv("AMD_COMGR_DRIVER_OPTIONS_APPEND");
  return Options ? Options : "";
}

} // namespace env
} // namespace COMGR
