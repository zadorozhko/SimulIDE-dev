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

#include "outpaneltext.h"
#include "mainwindow.h"
#include "simulator.h"

OutPanelText::OutPanelText( QWidget* parent )
            : QPlainTextEdit( parent )
            , Updatable()
{
    m_textBuffer = "";

    m_highlighter = new OutHighlighter( document() );

    QPalette p;
    p.setColor( QPalette::Base, QColor( 35, 30, 60 ) );
    p.setColor( QPalette::Text, QColor( 190, 190, 150) );
    setPalette(p);
    
    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setWeight( 50 );
    font.setFixedPitch(true);
    font.setPixelSize( 13*MainWindow::self()->fontScale() );
    setFont( font );

    setMaximumBlockCount( 1000 );
}
OutPanelText::~OutPanelText(){}

void OutPanelText::appendLine( const QString text )
{
    m_textBuffer.append( text+"\n" );
    if( !Simulator::self() || !Simulator::self()->isRunning() ) updateStep();
}

void OutPanelText::updateStep()
{
    if( m_textBuffer.isEmpty() ) return;

    if( this->document()->characterCount() > 100000 )
        setPlainText( this->toPlainText().right( 90000 ) );

    moveCursor( QTextCursor::End );
    insertPlainText( m_textBuffer );
    moveCursor( QTextCursor::End );
    m_textBuffer.clear();
}

// CLASS OutHighlighter ***********************************************

OutHighlighter::OutHighlighter( QTextDocument* parent )
              : QSyntaxHighlighter( parent )
{
    HighlightingRule rule;
    QTextCharFormat format;

    format.setForeground( QColor(110, 180, 100) );
    rule.pattern = QRegExp( "/[^\n]*" );
    rule.format = format;
    highlightingRules.append( rule );

    format.setForeground( QColor(120, 120, 250) );
    rule.pattern = QRegExp("[0-9]+");
    rule.format = format;
    highlightingRules.append(rule);

    format.setForeground( QColor(50, 200, 100) );
    rule.pattern = QRegExp( "\".*\"" );
    rule.format = format;
    highlightingRules.append( rule );

    format.setForeground( QColor(200, 255, 200) );
    format.setFontWeight( QFont::Bold );
    rule.pattern = QRegExp("\\bSUCCESS\\b");
    rule.format = format;
    highlightingRules.append(rule);
    
    format.setForeground( QColor(255, 255, 200) );
    rule.pattern = QRegExp("SENT:");
    rule.format = format;
    highlightingRules.append(rule);

    format.setForeground( QColor(255, 200, 100) );
    rule.pattern = QRegExp("\\bWARNING\\b");
    rule.format = format;
    highlightingRules.append(rule);

    format.setForeground( QColor(100, 50, 0) );
    format.setBackground( QColor(255, 255, 100) );
    rule.pattern = QRegExp("\\bERROR\\b");
    rule.format = format;
    highlightingRules.append(rule);
}
OutHighlighter::~OutHighlighter(){}

void OutHighlighter::highlightBlock( const QString &text )
{
    QString upText = text;
    upText = upText.toUpper(); // Do case insensitive

    for( const HighlightingRule &rule : highlightingRules )
    {
        QRegExp expression( rule.pattern );
        int index = expression.indexIn( upText );
        while( index >= 0 )
        {
            int length = expression.matchedLength();
            setFormat( index, length, rule.format );
            index = expression.indexIn( upText, index + length );
}   }   }

#include "moc_outpaneltext.cpp"
