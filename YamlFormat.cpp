#include "YamlFormat.h"
#include "UTF8.h"

#include <fstream>

#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>

class YamlFormatImpl : public YamlFormat {
 public:
    YamlFormatImpl(const wchar_t *szFileName);

    virtual const wchar_t *FormatName() const override { return L"json"; }
    virtual const wchar_t *HostFileName() const override { return filename_.c_str(); }
    virtual const wchar_t *CurrentDirectory() const override { return curdir_.c_str(); }
    virtual bool Changed() const override { return changed_; }

    virtual const path &GetPath() const override { return curpath_; }
    virtual panelitem_vector &GetFindData() override { return m_arrItems; }

    virtual bool SetPath(const path &curpath) override;
    virtual bool GetFile(const path_element &item, const std::wstring &path, int indent = -1) override;
    virtual bool SetFile(const path_element &item, const std::wstring &path) override;
    virtual bool DeleteFile(const path_element &item) override;
    virtual bool ApplyChanges(int indent = -1) override;

 protected:
    std::wstring filename_;
    YAML::Node data_;
    bool changed_ = false;

    std::wstring curdir_;
    path curpath_;
    panelitem_vector m_arrItems;

    static std::unique_ptr<YAML::Node> NextNode(YAML::Node &current, const path_element &elem);
    std::unique_ptr<YAML::Node> CurrentNode();
    void FillItems();
};

YamlFormatImpl::YamlFormatImpl(const wchar_t *szFileName) : filename_(szFileName) {
    in_stream ifs(szFileName);
    data_ = YAML::Load(ifs);

    FillItems();
}

std::string dump(YAML::Node node, int indent = -1) {
    YAML::Emitter emit;
    if (indent > 0) {
        emit.SetIndent(indent);
        // emit.SetStringFormat(YAML::Literal);
    }
    emit << node;
    return emit.c_str();
}

std::unique_ptr<YAML::Node> YamlFormatImpl::NextNode(YAML::Node &current, const path_element &elem) {
    if (current.IsSequence() && elem.index.has_value() && elem.index.value() < current.size()) {
        return std::make_unique<YAML::Node>(current[elem.index.value()]);
    } else if (auto val = current[EncodeUTF8(elem.name)]; val) {
        return std::make_unique<YAML::Node>(val);
    } else
        return {};
}

std::unique_ptr<YAML::Node> YamlFormatImpl::CurrentNode() {
    auto current = std::make_unique<YAML::Node>(data_);

    int count = 0;
    for (const auto &elem : curpath_) {
        auto next = NextNode(*current, elem);
        if (next) {
            current = std::move(next);
            ++count;
        } else
            break;
    }

    curpath_.resize(count);

    return current;
}

void YamlFormatImpl::FillItems() {
    m_arrItems.clear();

    auto curr = CurrentNode();

    auto fill_value = [this](const std::wstring &key, size_t index, const YAML::Node &value) {
        CPluginPanelItem pitem;

        SetFindDataName(pitem, key.c_str());
        pitem.UData() = index;

        if (value.IsMap() || value.IsSequence())
            pitem.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        else {
            auto descr = value.as<std::string>();
            pitem.Description = _wcsdup(DecodeUTF8(descr).c_str());
            pitem.FileSize = descr.size();
        }

        m_arrItems.push_back(pitem);
    };

    if (curr->IsMap()) {
        for (auto item : *curr) {
            fill_value(DecodeUTF8(item.first.as<std::string>()), -1, item.second);
        }
    } else if (curr->IsSequence()) {
        for (auto index = 0; index < curr->size(); ++index) {
            fill_value(std::to_wstring(index), index, (*curr)[index]);
        }
    }
}

bool YamlFormatImpl::SetPath(const path &curpath) {
    curpath_ = curpath;
    FillItems();
    curdir_ = CombinePath(curpath_);

    return true;
}

bool YamlFormatImpl::GetFile(const path_element &item, const std::wstring &path, int indent) {
    auto curr = CurrentNode();

    auto next = NextNode(*curr, item);
    if (!next)
        return false;

    out_stream ofs(path);
    ofs << dump(*next, indent);

    return true;
}

bool YamlFormatImpl::SetFile(const path_element &item, const std::wstring &path) {
    auto curr = CurrentNode();

    in_stream ifs(path);
    auto data = YAML::Load(ifs);

    if (item.index.has_value())
        (*curr)[item.index.value()] = data;
    else
        (*curr)[EncodeUTF8(item.name)] = data;

    changed_ = true;
    FillItems();

    return true;
}

bool YamlFormatImpl::DeleteFile(const path_element &item) {
    auto curr = CurrentNode();

    if (item.index.has_value())
        curr->remove(item.index.value());
    else
        curr->remove(EncodeUTF8(item.name));

    changed_ = true;
    FillItems();

    return true;
}

bool YamlFormatImpl::ApplyChanges(int indent) {
    if (!changed_)
        return true;

    out_stream ofs(filename_);
    ofs << dump(data_, indent);

    return true;
}

bool FormatFactory<YamlFormat>::ApplicableFileName(const wchar_t *szFileName) {
    std::wstring strFileName(szFileName);

    return strFileName.ends_with(L".yaml") || strFileName.ends_with(L".yml");
}

bool FormatFactory<YamlFormat>::ApplicableFile(const wchar_t *szFileName, std::wstring &error) {
    try {
        in_stream ifs(szFileName);
        auto res = YAML::Load(ifs);
        return res.IsMap() || res.IsSequence();
    } catch (std::exception &ex) {
        error = DecodeUTF8(ex.what());
        return false;
    }

    return true;
}

IFormat *FormatFactory<YamlFormat>::CreateFromFile(const wchar_t *szFileName) {
    return new YamlFormatImpl(szFileName);
}
