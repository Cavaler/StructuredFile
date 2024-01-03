#include "StructuredFile.h"

#include "UTF8.h"

path_element::path_element(const CPluginPanelItem &item) : name(item.FileName) {
    if (item.UData() != -1)
        index = item.UData();
}

std::weak_ordering path_element::operator<=>(const path_element &rhs) const {
    if (index.has_value() && rhs.index.has_value()) {
        return index.value() <=> rhs.index.value();
    } else {
        return name <=> rhs.name;
    }
}

path SplitPath(const std::wstring &dir) {
    path pth;

    return pth;
}

std::wstring CombinePath(const path &pth) {
    std::wstring result;

    for (const auto &elem : pth) {
        if (!result.empty())
            result.append(L"\\");
        result.append(elem.name);
    }

    return result;
}

std::ios_base::openmode FileSaveMode() {
    return g_bSaveUnixEOL ? std::ios_base::binary : 0;
}

in_stream::in_stream(const std::wstring &path, std::ios_base::openmode mode) : std::ifstream(path, mode) {
    if (!is_open())
        throw std::runtime_error(EncodeUTF8(GetMsg(MOpenError) + path) + "\n"s + strerror(errno));
}

out_stream::out_stream(const std::wstring &path) : std::ofstream(path, FileSaveMode()) {
    if (!is_open())
        throw std::runtime_error(EncodeUTF8(GetMsg(MSaveError) + path) + "\n"s + strerror(errno));
}
