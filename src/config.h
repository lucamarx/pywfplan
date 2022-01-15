#pragma once
#include <string>

// Slot length (fixed to 5 minutes)
const unsigned int SLOT_LENGTH = 5;

// Number of 5 minutes slots in a day
constexpr unsigned int SLOTS_DAY = 24 * 60 / SLOT_LENGTH;

// Annealing iteration limit for each agent day
const unsigned int NOVER = 100;
