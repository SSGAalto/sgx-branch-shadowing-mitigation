/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstring>
#include <mem/Pointers.h>
#include <misc/Logger.h>
#include "SegJumpR13.h"

SegJumpR13::SegJumpR13(void *pos)
        : ShadowSegment(s_hex_size, pos)
{};

void SegJumpR13::finalize()
{
    ShadowSegment::finalize();
}

std::shared_ptr<ShadowSegment> SegJumpR13::make(void *pos)
{
    return std::make_shared<SegJumpR13>(pos);
}
