/*
 * Copyright (C) 2016 Floris Creyf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef CURVE_H
#define CURVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vector.h"

TMvec3 tmGetBezier(float t, TMvec3 *p, int degree);
TMvec3 tmGetPath(float t, TMvec3 *p, int curveCount);

#ifdef __cplusplus
}
#endif

#endif /* CURVE_H */
