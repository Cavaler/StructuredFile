#include <PersistVariablesFar3.h>

// clang-format off

PERSIST_bool_VARIABLE_(g_bExportPrettyPrint, L"ExportPrettyPrint", false)
PERSIST_bool_VARIABLE_(g_bEditPrettyPrint, L"EditPrettyPrint", true)
PERSIST_TYPED_VARIABLE_(int, g_nIndent, L"Indent", 4, 0, 32)
PERSIST_bool_VARIABLE_(g_bSaveUnixEOL, L"SaveUnixEOL", true)

// clang-format on

#include <PersistVariablesUndef.h>
