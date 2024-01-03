#pragma once

#include "IFormat.h"

class YamlFormat : public IFormat {
 public:
};

template <>
class FormatFactory<YamlFormat> {
 public:
    static constexpr intptr_t FORMAT = 2;
    static bool ApplicableFileName(const wchar_t *szFileName);
    static bool ApplicableFile(const wchar_t *szFileName, std::wstring &error);
    static IFormat *CreateFromFile(const wchar_t *szFileName);
};
