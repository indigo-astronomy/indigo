/*
 * affine.c -- Affine Transforms for 2d objects
 * Copyright (C) 2002 Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 */

#include "affine.h"

static inline void Multiply( affine_transform_t *this, affine_transform_t *that )
{
    double output[2][2];
    register int i, j;

    for ( i = 0; i < 2; i ++ )
        for ( j = 0; j < 2; j ++ )
            output[ i ][ j ] = this->matrix[ i ][ 0 ] * that->matrix[ j ][ 0 ] +
                this->matrix[ i ][ 1 ] * that->matrix[ j ][ 1 ];

    this->matrix[ 0 ][ 0 ] = output[ 0 ][ 0 ];
    this->matrix[ 0 ][ 1 ] = output[ 0 ][ 1 ];
    this->matrix[ 1 ][ 0 ] = output[ 1 ][ 0 ];
    this->matrix[ 1 ][ 1 ] = output[ 1 ][ 1 ];
}

void affine_transform_init( affine_transform_t *this )
{
    this->matrix[ 0 ][ 0 ] = 1;
    this->matrix[ 0 ][ 1 ] = 0;
    this->matrix[ 1 ][ 0 ] = 0;
    this->matrix[ 1 ][ 1 ] = 1;
}

// Rotate by a given angle
void affine_transform_rotate( affine_transform_t *this, double angle )
{
    affine_transform_t affine;
    affine.matrix[ 0 ][ 0 ] = cos( angle * M_PI / 180 );
    affine.matrix[ 0 ][ 1 ] = 0 - sin( angle * M_PI / 180 );
    affine.matrix[ 1 ][ 0 ] = sin( angle * M_PI / 180 );
    affine.matrix[ 1 ][ 1 ] = cos( angle * M_PI / 180 );
    Multiply( this, &affine );
}

// Shear by a given value
void affine_transform_shear( affine_transform_t *this, double shear )
{
    affine_transform_t affine;
    affine.matrix[ 0 ][ 0 ] = 1;
    affine.matrix[ 0 ][ 1 ] = shear;
    affine.matrix[ 1 ][ 0 ] = 0;
    affine.matrix[ 1 ][ 1 ] = 1;
    Multiply( this, &affine );
}

void affine_transform_scale( affine_transform_t *this, double sx, double sy )
{
    affine_transform_t affine;
    affine.matrix[ 0 ][ 0 ] = sx;
    affine.matrix[ 0 ][ 1 ] = 0;
    affine.matrix[ 1 ][ 0 ] = 0;
    affine.matrix[ 1 ][ 1 ] = sy;
    Multiply( this, &affine );
}

// Obtain the mapped x coordinate of the input
double affine_transform_mapx( affine_transform_t *this, int x, int y )
{
    return this->matrix[0][0] * x + this->matrix[0][1] * y;
}

// Obtain the mapped y coordinate of the input
double affine_transform_mapy( affine_transform_t *this, int x, int y )
{
    return this->matrix[1][0] * x + this->matrix[1][1] * y;
}
