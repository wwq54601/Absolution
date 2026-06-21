#pragma once

#include "file_analysis.h"
#include "../codebase/indexer.h"

class FileAnalyzer {
public:
    FileAnalysis analyze(const CodeFile& file);
};