#include "JsonFormat.h"
#include "UTF8.h"

#include <fstream>

#include "nlohmann/json.hpp"
using json = nlohmann::ordered_json;

class JsonFormatImpl : public JsonFormat {
 public:
    JsonFormatImpl(const wchar_t *szFileName);

    virtual const wchar_t *FormatName() const override { return L"json"; }
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
    json data_;
    bool changed_ = false;

    std::wstring curdir_;
    path curpath_;
    panelitem_vector m_arrItems;

    static json &NextJson(json &current, const path_element &elem);
    static inline json failed = json::value_t::discarded;
    json &CurrentJson();

    void FillItems();
};

JsonFormatImpl::JsonFormatImpl(const wchar_t *szFileName) : filename_(szFileName) {
    in_stream ifs(szFileName);
    data_ = json::parse(ifs);

    FillItems();
}

json &JsonFormatImpl::NextJson(json &current, const path_element &elem) {
    if (current.is_array() && elem.index.has_value() && elem.index.value() < current.size()) {
        return current.at(elem.index.value());
    } else if (auto it = current.find(EncodeUTF8(elem.name)); it != current.end()) {
        return *it;
    } else
        return JsonFormatImpl::failed;
}

json &JsonFormatImpl::CurrentJson() {
    json *current = &data_;

    int count = 0;
    for (const auto &elem : curpath_) {
        auto &next = NextJson(*current, elem);
        if (!next.is_discarded()) {
            current = &next;
            ++count;
        } else
            break;
    }

    curpath_.resize(count);

    return *current;
}

void JsonFormatImpl::FillItems() {
    m_arrItems.clear();

    auto &curr = CurrentJson();

    auto fill_value = [this](const std::wstring &key, size_t index, const json &value) {
        CPluginPanelItem pitem;

        auto descr = DecodeUTF8(value.dump());

        SetFindDataName(pitem, key.c_str());
        pitem.Description = _wcsdup(descr.c_str());
        pitem.FileSize = descr.size();
        pitem.UData() = index;

        if (value.is_object() || value.is_array())
            pitem.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        m_arrItems.push_back(pitem);
    };

    if (curr.is_object()) {
        for (const auto &item : curr.items()) {
            fill_value(DecodeUTF8(item.key()), -1, item.value());
        }
    } else if (curr.is_array()) {
        for (auto index = 0; index < curr.size(); ++index) {
            fill_value(std::to_wstring(index), index, curr.at(index));
        }
    }
}

bool JsonFormatImpl::SetPath(const path &curpath) {
    curpath_ = curpath;
    FillItems();
    curdir_ = CombinePath(curpath_);

    return true;
}

bool JsonFormatImpl::GetFile(const path_element &item, const std::wstring &path, int indent) {
    auto &curr = CurrentJson();

    auto next = NextJson(curr, item);
    if (next.is_discarded())
        return false;

    out_stream ofs(path);
    ofs << next.dump(indent);

    return true;
}

bool JsonFormatImpl::SetFile(const path_element &item, const std::wstring &path) {
    auto &curr = CurrentJson();

    in_stream ifs(path);
    auto data = json::parse(ifs);

    if (item.index.has_value())
        curr[item.index.value()] = data;
    else
        curr[EncodeUTF8(item.name)] = data;

    changed_ = true;
    FillItems();

    return true;
}

bool JsonFormatImpl::DeleteFile(const path_element &item) {
    auto &curr = CurrentJson();

    if (item.index.has_value())
        curr.erase(item.index.value());
    else
        curr.erase(EncodeUTF8(item.name));

    changed_ = true;
    FillItems();

    return true;
}

bool JsonFormatImpl::ApplyChanges(int indent) {
    if (!changed_)
        return true;

    out_stream ofs(filename_);
    ofs << data_.dump(indent);

    return true;
}

bool FormatFactory<JsonFormat>::ApplicableFileName(const wchar_t *szFileName) {
    return std::wstring(szFileName).ends_with(L".json");
}

bool FormatFactory<JsonFormat>::ApplicableFile(const wchar_t *szFileName, std::wstring &error) {
    try {
        in_stream ifs(szFileName);
        auto res = json::parse(ifs);
        return res.is_object() || res.is_array();
    } catch (std::exception &ex) {
        error = DecodeUTF8(ex.what());
        return false;
    }
}

IFormat *FormatFactory<JsonFormat>::CreateFromFile(const wchar_t *szFileName) {
    return new JsonFormatImpl(szFileName);
}
