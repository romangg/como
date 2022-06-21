/*
 *   Copyright © 2010 Fredrik Höglund <fredrik@kde.org>
 *   Copyright 2014 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; see the file COPYING.  if not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#ifndef CONTRASTSHADER_H
#define CONTRASTSHADER_H

#include <kwingl/utils.h>

class QMatrix4x4;

namespace KWin
{

class ContrastShader
{
public:
    ContrastShader();

    void init();

    void setColorMatrix(const QMatrix4x4& matrix);
    void setTextureMatrix(const QMatrix4x4& matrix);
    void setModelViewProjectionMatrix(const QMatrix4x4& matrix);
    void setOpacity(float opacity);

    float opacity() const;
    bool isValid() const;

    void bind();
    void unbind();

protected:
    void setIsValid(bool value);
    void reset();

private:
    bool mValid;
    std::unique_ptr<GLShader> shader;
    int m_mvpMatrixLocation;
    int m_textureMatrixLocation;
    int m_colorMatrixLocation;
    int m_opacityLocation;
    float m_opacity;
};

} // namespace KWin

#endif
