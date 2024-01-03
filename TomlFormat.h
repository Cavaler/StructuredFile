#pragma once

#include "IFormat.h"

class TomlFormat : public IFormat {
 public:
};

template <>
class FormatFactory<TomlFormat> {
 public:
    static constexpr intptr_t FORMAT = 3;
    static bool ApplicableFileName(const wchar_t *szFileName);
    static bool ApplicableFile(const wchar_t *szFileName, std::wstring &error);
    static IFormat *CreateFromFile(const wchar_t *szFileName);
};
