/***************************************************************************
 *   Copyright (C) 2012 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include "shape.h"
#include "circuit.h"

static const char* Shape_properties[] = {
    QT_TRANSLATE_NOOP("App::Property","H size"),
    QT_TRANSLATE_NOOP("App::Property","V size"),
    QT_TRANSLATE_NOOP("App::Property","Border"),
    QT_TRANSLATE_NOOP("App::Property","Color"),
    QT_TRANSLATE_NOOP("App::Property","Opacity"),
    QT_TRANSLATE_NOOP("App::Property","Z Value")
};

Shape::Shape( QObject* parent, QString type, QString id )
     : Component( parent, type, id )
{
    Q_UNUSED( Shape_properties );

    m_graphical = true;
    
    m_hSize = 50;
    m_vSize = 30;
    m_border = 2;
    m_color  = QColor( Qt::gray );
    
    setZValue( -1 );
    
    m_area = QRectF( -m_hSize/2, -m_vSize/2, m_hSize, m_vSize );
}
Shape::~Shape(){}

QList<propGroup_t> Shape::propGroups()
{
    propGroup_t mainGroup { tr("Main") };
    mainGroup.propList.append( {"H_size", tr("Size X"),"Pixels"} );
    mainGroup.propList.append( {"V_size", tr("Size Y"),"Pixels"} );
    mainGroup.propList.append( {"Border", tr("Border"),"Pixels"} );
    mainGroup.propList.append( {"Z_Value", tr("Z Value"),""} );

    propGroup_t colorGroup { tr("Color") };
    colorGroup.propList.append( {"Color", tr("Color"),""} );
    colorGroup.propList.append( {"Opacity", tr("Opacity"),""} );

    return {mainGroup, colorGroup};
}

void Shape::setHSize( int size )
{
    m_hSize = size;
    m_area = QRectF( -m_hSize/2, -m_vSize/2, m_hSize, m_vSize );
    Circuit::self()->update();
}

void Shape::setVSize( int size )
{
    m_vSize = size;
    m_area = QRectF( -m_hSize/2, -m_vSize/2, m_hSize, m_vSize );
    Circuit::self()->update();
}

void Shape::setBorder( int border ) 
{ 
    if( border < 0 ) border = 0;
    m_border = border; 
    update();
}

void Shape::setColor( QColor color )
{
    m_color = color;
    update();
}

#include "moc_shape.cpp"
