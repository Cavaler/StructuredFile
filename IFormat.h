#pragma once

#include "FAR.h"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <fstream>

struct path_element {
    path_element() {}
    path_element(const CPluginPanelItem &item);
    std::wstring name;
    std::optional<unsigned int> index;

    std::weak_ordering operator<=>(const path_element &rhs) const;
};
using path = std::vector<path_element>;

path SplitPath(const std::wstring &dir);
std::wstring CombinePath(const path &pth);
std::ios_base::openmode FileSaveMode();

class in_stream : public std::ifstream {
 public:
    in_stream(const std::wstring &path, std::ios_base::openmode mode = 0);
};

class out_stream : public std::ofstream {
 public:
    out_stream(const std::wstring &path);
};

class IFormat {
 public:
    virtual ~IFormat() {}

    virtual const wchar_t *FormatName() const = 0;
    virtual const wchar_t *DefaultExtension() const { return FormatName(); }
    virtual const wchar_t *HostFileName() const = 0;
    virtual const wchar_t *CurrentDirectory() const = 0;
    virtual bool Changed() const = 0;

    virtual const path &GetPath() const = 0;
    virtual panelitem_vector &GetFindData() = 0;

    virtual bool SetPath(const path &curpath) = 0;
    virtual bool GetFile(const path_element &item, const std::wstring &path, int indent) = 0;
    virtual bool SetFile(const path_element &item, const std::wstring &path) = 0;
    virtual bool DeleteFile(const path_element &item) = 0;
    virtual bool ApplyChanges(int indent) = 0;
};

template <typename IFormat>
class FormatFactory {
 public:
    static bool ApplicableFileName(const wchar_t *szFileName);
    static bool ApplicableFile(const wchar_t *szFileName, std::wstring &error);
    static IFormat *CreateFromFile(const wchar_t *szFileName);
};
