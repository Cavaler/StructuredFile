#pragma once

#include "IFormat.h"

class JsonFormat : public IFormat {
 public:
};

template <>
class FormatFactory<JsonFormat> {
 public:
    static constexpr intptr_t FORMAT = 1;
    static bool ApplicableFileName(const wchar_t *szFileName);
    static bool ApplicableFile(const wchar_t *szFileName, std::wstring &error);
    static IFormat *CreateFromFile(const wchar_t *szFileName);
};
