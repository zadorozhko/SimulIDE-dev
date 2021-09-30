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

#include <QDomDocument>

#include "editorwindow.h"
#include "circuitwidget.h"
#include "mainwindow.h"
#include "mcuinterface.h"
#include "mcubase.h"
#include "simulator.h"
#include "compiler.h"
#include "utils.h"
#include "simuapi_apppath.h"

#include "gcbdebugger.h"
#include "inodebugger.h"
#include "b16asmdebugger.h"
#include "avrgccdebugger.h"
#include "sdccdebugger.h"
#include "picasmdebugger.h"

EditorWindow* EditorWindow::m_pSelf = NULL;

EditorWindow::EditorWindow( QWidget* parent )
            : EditorWidget( parent )
{
    m_pSelf = this;

    m_debugDoc = NULL;
    m_debugger = NULL;
    m_state = DBG_STOPPED;
    m_driveCirc = false;
    m_stepOver = false;

    CodeEditor::readSettings();
    loadCompilers();
}
EditorWindow::~EditorWindow(){}

bool EditorWindow::upload()
{
    return uploadFirmware( false );
}

bool EditorWindow::uploadFirmware( bool debug )
{
    CodeEditor* ce = getCodeEditor();
    if( !ce ) return false;

    bool ok = ce->compile( debug );
    if( ok ) ok = ce->getCompiler()->upload();

    return ok;
}

void EditorWindow::debug()
{
    if( initDebbuger() )
    {
        m_editorToolBar->setVisible( false);
        m_debuggerToolBar->setVisible( true );

        runAct->setEnabled( true );
        stepAct->setEnabled( true );
        stepOverAct->setEnabled( true );
        resetAct->setEnabled( true );
        pauseAct->setEnabled( false );

        Simulator::self()->addToUpdateList( &m_outPane );
}   }

void EditorWindow::run()
{ 
    setStepActs();

    if( m_state == DBG_STOPPED ) return;
    m_state = DBG_RUNNING;
    if( m_driveCirc ) Simulator::self()->resumeSim();
    McuInterface::self()->stepOne( m_debugDoc->debugLine() );
}

void EditorWindow::step()
{ 
    setStepActs();
    stepDebug();
}

void EditorWindow::stepOver()
{
    /*setStepActs();
    QList<int> subLines = m_compiler->getSubLines();
    bool over = subLines.contains( m_debugLine ) ? true : false;
    stepDebug( over );*/
}

void EditorWindow::pause()   
{
    if( m_state < DBG_STEPING )  return;
    if( m_driveCirc ) Simulator::self()->pauseSim();

    m_resume = m_state;
    m_state  = DBG_PAUSED;

    runAct->setEnabled( true );
    stepAct->setEnabled( true );
    stepOverAct->setEnabled( true );
    resetAct->setEnabled( true );
    pauseAct->setEnabled( false );
}

void EditorWindow::stop()
{ 
    stopDebbuger();
    m_debuggerToolBar->setVisible( false );
    m_editorToolBar->setVisible( true);

    Simulator::self()->remFromUpdateList( &m_outPane );
}

bool EditorWindow::initDebbuger()
{
    m_outPane.appendLine( "-------------------------------------------------------\n" );
    m_outPane.appendLine( tr("Starting Debbuger...")+"\n" );

    m_debugDoc = NULL;
    m_debugger = NULL;
    m_state = DBG_STOPPED;
    bool ok = uploadFirmware( true );

    if( ok )  // OK: Start Debugging
    {
        m_debugDoc = getCodeEditor();
        m_debugDoc->setReadOnly( true );
        m_debugger = m_debugDoc->getCompiler();

        stepOverAct->setVisible( m_stepOver );
        McuInterface::self()->setDebugging( true );
        reset();
        setDriveCirc( m_driveCirc );
        CircuitWidget::self()->powerCircDebug( m_driveCirc );

        m_outPane.appendLine("\n"+tr("Debugger Started")+"\n");
    }else{
        m_outPane.appendLine( "\n"+tr("Error Starting Debugger")+"\n" );
        stopDebbuger();
    }
    return ok;
}

void EditorWindow::stepDebug( bool over )
{
    if( m_state == DBG_RUNNING ) return;

    /*if( over ){
        addBreakPoint( m_debugLine+1 );
        run();
    }else */
    {
        m_state = DBG_STEPING;
        McuInterface::self()->stepOne( m_debugDoc->debugLine() );
        if( m_driveCirc ) Simulator::self()->resumeSim();
}   }

void EditorWindow::lineReached( int line ) // Processor reached PC related to source line
{
    m_debugDoc->setDebugLine( line );

    if( ( m_state == DBG_RUNNING )           // We are running to Breakpoint
     && !m_debugDoc->hasBreakPoint( line ) ) // Breakpoint not reached, Keep stepping
    {
        McuInterface::self()->stepOne( line );
        return;
    }
    EditorWindow::self()->pause(); // EditorWindow: calls this->pause as well

    int cycle = McuInterface::self()->cycle();
    m_outPane.appendLine( tr("Clock Cycles: ")+QString::number( cycle-m_lastCycle ));
    m_lastCycle = cycle;
    m_debugDoc->updateScreen();
}

void EditorWindow::stopDebbuger()
{
    if( m_state > DBG_STOPPED )
    {
        CircuitWidget::self()->powerCircOff();
        McuInterface::self()->setDebugging( false );

        m_state = DBG_STOPPED;
        m_debugDoc->setDebugLine( 0 );
        m_debugDoc->setReadOnly( false );
        m_debugDoc->updateScreen();
    }
    m_outPane.appendLine( "\n"+tr("Debugger Stopped ")+"\n" );
}

void EditorWindow::reset()
{
    if( m_state == DBG_RUNNING ) pause();

    m_lastCycle = 0;
    m_state = DBG_PAUSED;

    McuBase::self()->reset();
    m_debugDoc->setDebugLine( 1 );
    m_debugDoc->updateScreen();
}

void EditorWindow::setDriveCirc( bool drive )
{
    m_driveCirc = drive;

    if( m_state == DBG_PAUSED )
    { if( drive ) Simulator::self()->pauseSim(); }
}

BaseDebugger* EditorWindow::createDebugger( QString name, CodeEditor* ce , QString code )
{
    BaseDebugger* debugger = NULL;
    QString type = m_compilers.value( name ).type;
    QString file = m_compilers.value( name ).file;
    if( type.isEmpty() )
    {
        type = m_assemblers.value( name ).type;
        file = m_assemblers.value( name ).file;
    }
    if     ( type == "arduino")  debugger = new InoDebugger( ce, &m_outPane );
    else if( type == "avrgcc" )  debugger = new AvrGccDebugger( ce, &m_outPane );
    else if( type == "sdcc" )    debugger = new SdccDebugger( ce, &m_outPane );
    else if( type == "gcbasic" ) debugger = new GcbDebugger( ce, &m_outPane );

    else if( type == "gputils" ) debugger = new PicAsmDebugger( ce, &m_outPane );
    //else if( type == "b16asm" )  debugger = new B16AsmDebugger( ce, &m_outPane );
    else
    {
        debugger = new BaseDebugger( ce, &m_outPane );
        if( name != "None" ) code = type.right( 2 );
        debugger->setLstType( code.right( 1 ).toInt() );
        debugger->setLangLevel( code.left( 1 ).toInt() );
    }
    if( name != "None" ) debugger->loadCompiler( file );
    return debugger;
}

void EditorWindow::loadCompilers()
{
    QString compilsPath = SIMUAPI_AppPath::self()->availableDataFilePath("codeeditor/compilers/compilers");
    loadCompilerSet( compilsPath, &m_compilers );
    compilsPath = SIMUAPI_AppPath::self()->availableDataFilePath("codeeditor/compilers/assemblers");
    loadCompilerSet( compilsPath, &m_assemblers );
}

void EditorWindow::loadCompilerSet( QString compilsPath, QMap<QString, compilData_t>* compList )
{
    QDir compilsDir = QDir( compilsPath );

    compilsDir.setNameFilters( QStringList( "*.xml" ) );

    QStringList xmlList = compilsDir.entryList( QDir::Files, QDir::Name );
    if( xmlList.isEmpty() ) return;                  // No compilers to load
    qDebug() <<"\n"<<tr("    Loading Compilers at:")<<"\n"<<compilsPath<<"\n";

    for( QString compilFile : xmlList )
    {
        QString compilFilePath = compilsDir.absoluteFilePath( compilFile );
        if( !compilFilePath.isEmpty( ))
        {
            QDomDocument domDoc = fileToDomDoc( compilFilePath, "EditorWindow::loadCompilers");
            if( domDoc.isNull() ) continue;

            QDomElement el = domDoc.documentElement();
            if( (el.tagName() == "compiler")
              && el.hasAttribute( "name" )
              && el.hasAttribute( "type" )  )
            {
                QString compiler = el.attribute( "name" ) ;

                compilData_t compilData;
                compilData.file = compilFilePath;
                compilData.type = el.attribute( "type" ) ;

                compList->insert( compiler, compilData );
                qDebug() << tr("        Found Compiler: ") << compiler;
            }
            else qDebug() << tr("Error Loadind Compiler at:") <<"\n" << compilFilePath <<"\n";
}   }   }

#include  "moc_editorwindow.cpp"
