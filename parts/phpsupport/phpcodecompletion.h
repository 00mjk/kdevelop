/***************************************************************************
                          phpcodecompletion.h  -  description
                             -------------------
    begin                : Tue Jul 17 2001
    copyright            : (C) 2001 by Sandy Meier
    email                : smeier@kdevelop.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PHPCODECOMPLETION_H
#define PHPCODECOMPLETION_H

#include <qobject.h>
#include <kregexp.h>
#include <kparts/part.h>

#include <ktexteditor/editinterface.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/codecompletioninterface.h>


class KDevCore;
class ClassStore;
class PHPSupportPart;

/**
 *@author Sandy Meier
 */

class FunctionCompletionEntry : public KTextEditor::CompletionEntry {
 public:
  QString prototype;
};

class PHPCodeCompletion : public QObject {
  Q_OBJECT

public: 
  PHPCodeCompletion(PHPSupportPart *part, KDevCore* core,ClassStore* store);
  ~PHPCodeCompletion();
protected slots:  
  void activePartChanged(KParts::Part *part);
  void cursorPositionChanged();
  void argHintHided();
  void completionBoxHided();

 protected:
  bool checkForVariable(QString lineStr,int col,int line);
  bool checkForGlobalFunction(QString lineStr,int col);
  bool checkForNewInstance(QString lineStr,int col,int line);

  bool checkForGlobalFunctionArgHint(QString lineStr,int col,int line);
  bool checkForMethodArgHint(QString lineStr,int col,int line);
  bool checkForNewInstanceArgHint(QString lineStr,int col,int line);

  void readGlobalPHPFunctionsFile();

  QValueList<KTextEditor::CompletionEntry> getClassMethodsAndVariables(QString className);
  QString getClassName(QString varName,QString maybeInstanceOf);
  QString searchCurrentClassName();
  QString searchClassNameForVariable(QString varName);
 private:
  int m_currentLine;
  QValueList<FunctionCompletionEntry> m_globalFunctions;
  KDevCore* m_core;
  ClassStore* m_classStore;
  bool m_argWidgetShow;
  bool m_completionBoxShow;
  KTextEditor::EditInterface *m_editInterface;
  KTextEditor::CodeCompletionInterface *m_codeInterface;
  KTextEditor::ViewCursorInterface *m_cursorInterface;
};

#endif
