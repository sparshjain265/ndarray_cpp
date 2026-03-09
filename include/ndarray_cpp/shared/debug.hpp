/**
 * MIT License
 *
 * Copyright (c) 2026 Sparsh Jain
 *
 */

#ifndef SRC_SHARED_DEBUG_HPP
#define SRC_SHARED_DEBUG_HPP

#ifdef NDEBUG
#define DEBUG_ONLY [[maybe_unused]]
#else
#define DEBUG_ONLY
#endif // NDEBUG

#endif /* SRC_SHARED_DEBUG_HPP */
