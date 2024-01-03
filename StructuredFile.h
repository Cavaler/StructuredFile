#pragma once

#include <FAR.h>

#include "version.h"

#include "StructuredFile_msg.h"

#include "IFormat.h"
#include "JsonFormat.h"
#include "YamlFormat.h"
#include "TomlFormat.h"

#define DECLARE_PERSIST_VARS
#include "PersistVars.h"

void LoadSettings();
void SaveSettings();
