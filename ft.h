#ifndef FONTCONVERT_FT_H
#define FONTCONVERT_FT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#undef FTERRORS_H_
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, NULL } };

#include <string>
#include <iostream>
#include <filesystem>

const struct
{
    int          err_code;
    const char*  err_msg;
} ft_errors[] =
#include <freetype/fterrors.h>

FT_Library library;

std::string getErrorString(FT_Error error) {
    int i = 0;
    while (ft_errors[i].err_code != error)
        i++;
    return ft_errors[i].err_msg;
}

void scanFontFiles(const std::string& folder, std::vector<std::pair<std::string, std::string>>& fonts) {
    fonts.clear();
    for (const auto& entry: std::filesystem::directory_iterator(folder)) {
        std::string path = entry.path().string();
        FT_Face face;
        auto error = FT_New_Face(library,
                            path.c_str(),
                            0,
                            &face);
        if (error) {
            std::cerr << "FreeType error while reading font file: " << path << ": " << getErrorString(error) << "\n";
            continue;
        }
        fonts.emplace_back(path, std::string(face->family_name) + " (" + std::string(face->style_name) + ")");
    }
}

#endif //FONTCONVERT_FT_H
