#include "TomlFormat.h"
#include "UTF8.h"

#include <fstream>

#include "toml/toml.hpp"

class TomlFormatImpl : public TomlFormat {
 public:
    TomlFormatImpl(const wchar_t *szFileName);

    virtual const wchar_t *FormatName() const override { return L"toml"; }
    virtual const wchar_t *HostFileName() const override { return filename_.c_str(); }
    virtual const wchar_t *CurrentDirectory() const override { return curdir_.c_str(); }
    virtual bool Changed() const override { return changed_; }

    virtual const path &GetPath() const override { return curpath_; }
    virtual panelitem_vector &GetFindData() override { return m_arrItems; }

    virtual bool SetPath(const path &curpath) override;
    virtual bool GetFile(const path_element &item, const std::wstring &path, int indent) override;
    virtual bool SetFile(const path_element &item, const std::wstring &path) override;
    virtual bool DeleteFile(const path_element &item) override;
    virtual bool ApplyChanges(int indent) override;

 protected:
    std::wstring filename_;
    toml::value data_;
    bool changed_ = false;

    std::wstring curdir_;
    path curpath_;
    panelitem_vector m_arrItems;

    static toml::value &NextValue(toml::value &current, const path_element &elem);
    static inline toml::value failed;
    toml::value &CurrentValue();

    void FillItems();
};

TomlFormatImpl::TomlFormatImpl(const wchar_t *szFileName) : filename_(szFileName) {
    in_stream ifs(szFileName, std::ios_base::binary);
    data_ = toml::parse<toml::preserve_comments>(ifs, EncodeUTF8(szFileName));

    FillItems();
}

toml::value &TomlFormatImpl::NextValue(toml::value &current, const path_element &elem) {
    if (current.is_array() && elem.index.has_value() && elem.index.value() < current.size()) {
        return current.at(elem.index.value());
    } else if (current.is_table() && current.contains(EncodeUTF8(elem.name))) {
        return current.at(EncodeUTF8(elem.name));
    } else
        return failed;
}

toml::value &TomlFormatImpl::CurrentValue() {
    toml::value *current = &data_;

    int count = 0;
    for (const auto &elem : curpath_) {
        auto &next = NextValue(*current, elem);
        if (!next.is_uninitialized()) {
            current = &next;
            ++count;
        } else
            break;
    }

    curpath_.resize(count);

    return *current;
}

void TomlFormatImpl::FillItems() {
    m_arrItems.clear();

    auto &curr = CurrentValue();

    auto fill_value = [this](const std::wstring &key, size_t index, const toml::value &value) {
        CPluginPanelItem pitem;

        SetFindDataName(pitem, key.c_str());
        pitem.UData() = index;

        if (value.is_table() || value.is_array()) {
            pitem.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        } else {
            std::stringstream stm;
            stm << value;
            auto descr = DecodeUTF8(stm.str());
            pitem.Description = _wcsdup(descr.c_str());
            pitem.FileSize = descr.size();
        }

        m_arrItems.push_back(pitem);
    };

    if (curr.is_table()) {
        for (const auto &item : curr.as_table()) {
            fill_value(DecodeUTF8(item.first), -1, item.second);
        }
    } else if (curr.is_array()) {
        for (auto index = 0; index < curr.as_array().size(); ++index) {
            fill_value(std::to_wstring(index), index, curr.at(index));
        }
    }
}

bool TomlFormatImpl::SetPath(const path &curpath) {
    curpath_ = curpath;
    FillItems();
    curdir_ = CombinePath(curpath_);

    return true;
}

bool TomlFormatImpl::GetFile(const path_element &item, const std::wstring &path, int /*indent*/) {
    auto &curr = CurrentValue();

    auto next = NextValue(curr, item);
    if (next.is_uninitialized())
        return false;

    out_stream ofs(path);
    ofs << next;

    return true;
}

bool TomlFormatImpl::SetFile(const path_element &item, const std::wstring &path) {
    auto &curr = CurrentValue();

    in_stream ifs(path, std::ios_base::binary);
    auto data = toml::parse<toml::preserve_comments>(ifs, EncodeUTF8(path));

    if (item.index.has_value())
        curr[item.index.value()] = data;
    else
        curr[EncodeUTF8(item.name)] = data;

    changed_ = true;
    FillItems();

    return true;
}

bool TomlFormatImpl::DeleteFile(const path_element &item) {
    auto &curr = CurrentValue();

    if (item.index.has_value())
        curr.as_array().erase(curr.as_array().begin() + item.index.value());
    else
        curr.as_table().erase(EncodeUTF8(item.name));

    changed_ = true;
    FillItems();

    return true;
}

bool TomlFormatImpl::ApplyChanges(int /*indent*/) {
    if (!changed_)
        return true;

    out_stream ofs(filename_);
    ofs << data_;

    return true;
}

bool FormatFactory<TomlFormat>::ApplicableFileName(const wchar_t *szFileName) {
    std::wstring strFileName(szFileName);

    return strFileName.ends_with(L".toml");  //    || strFileName.ends_with(L".ini");
}

bool FormatFactory<TomlFormat>::ApplicableFile(const wchar_t *szFileName, std::wstring &error) {
    try {
        in_stream ifs(szFileName, std::ios_base::binary);
        auto res = toml::parse<toml::preserve_comments>(ifs, EncodeUTF8(szFileName));
        return res.is_array() || res.is_table();
    } catch (std::exception &ex) {
        error = DecodeUTF8(ex.what());
        return false;
    }
}

IFormat *FormatFactory<TomlFormat>::CreateFromFile(const wchar_t *szFileName) {
    return new TomlFormatImpl(szFileName);
}
