#pragma once
#include "DBOModel.h"
struct sObject;
namespace dbo {
// Builds the existing gameplay metadata from owned validated data. No binary
// cursor or renderer is used. Output is published only after construction
// succeeds.
bool CreateCompatibilityObject(const Model &model, sObject **output,
                               std::string &error);
bool DecodeCompatibilityObject(const void *bytes, size_t size, sObject **output,
                               std::string &error);
// Diagnostic for the most recent decode on this loading thread.
const std::string &LastCompatibilityError();
} // namespace dbo
