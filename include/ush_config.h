#ifndef USH_CONFIG_H
#define USH_CONFIG_H

#include <stdint.h>
#include <stddef.h>

/* Enable a minimal set of commands to reduce footprint */
#define USH_CONFIG_ENABLE_COMMAND_HELP   1
#define USH_CONFIG_ENABLE_COMMAND_CAT    1
#define USH_CONFIG_ENABLE_COMMAND_CD     1
#define USH_CONFIG_ENABLE_COMMAND_LS     1
#define USH_CONFIG_ENABLE_COMMAND_PWD    1
#define USH_CONFIG_ENABLE_COMMAND_ECHO   1
#define USH_CONFIG_ENABLE_COMMAND_XXD    1

/* Features */
#define USH_CONFIG_ENABLE_FEATURE_COMMANDS     1
#define USH_CONFIG_ENABLE_FEATURE_AUTOCOMPLETE 1
#define USH_CONFIG_ENABLE_FEATURE_SHELL_STYLES 1

/* Command configs */
#define USH_CONFIG_FILENAME_ALIGN_SPACE 16
#define USH_CONFIG_CMD_XXD_COLUMNS      16

/* Minimal translations (only what library may reference) */
#define USH_CONFIG_TRANSLATION_OK                        "ok"
#define USH_CONFIG_TRANSLATION_ERROR                     "error"
#define USH_CONFIG_TRANSLATION_DIRECTORY_NOT_FOUND       "directory_not_found"
#define USH_CONFIG_TRANSLATION_NESTED_DIRECTORIES_EXIST  "nested_directories_exist"
#define USH_CONFIG_TRANSLATION_CANNOT_FIND_PARENT_NODE   "cannot_find_parent_node"
#define USH_CONFIG_TRANSLATION_DIRECTORY_ALREADY_MOUNTED "directory_already_mounted"
#define USH_CONFIG_TRANSLATION_SYNTAX_ERROR              "syntax_error"
#define USH_CONFIG_TRANSLATION_WRONG_ARGUMENTS           "wrong_arguments"
#define USH_CONFIG_TRANSLATION_FILE_NOT_EXECUTABLE       "file_not_executable"
#define USH_CONFIG_TRANSLATION_FILE_NOT_WRITABLE         "file_not_writable"
#define USH_CONFIG_TRANSLATION_FILE_NOT_READABLE         "file_not_readable"
#define USH_CONFIG_TRANSLATION_NO_HELP_AVAILABLE         "no_help_available"
#define USH_CONFIG_TRANSLATION_FILE_NOT_FOUND            "file_not_found"
#define USH_CONFIG_TRANSLATION_READ_ONLY_FILE            "read_only_file"

/* USH_ASSERT: call application handler on failure (no stdio) */
void ush_assert_failed(const char *file, int line);
#define USH_ASSERT(cond) do { if (!(cond)) { ush_assert_failed(__FILE__, __LINE__); } } while (0)

#endif /* USH_CONFIG_H */
