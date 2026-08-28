#pragma once
#include <stdexcept>

// Thrown from inside the triangulation-generation write loops when the
// shared abort flag is tripped (disk limit exceeded). Caught in main.cpp
// to distinguish "cleanly cancelled" from "real error".
struct TaskAbortedException : public std::runtime_error
{
    TaskAbortedException() : std::runtime_error("Task aborted: disk limit exceeded") {}
};
