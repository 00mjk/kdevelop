/*
   Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "coderepresentation.h"
#include <QtCore/qfile.h>
#include <KDE/KTextEditor/Document>
#include <language/duchain/indexedstring.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/icore.h>

namespace KDevelop {

static bool onDiskChangesForbidden = false;

class EditorCodeRepresentation : public DynamicCodeRepresentation {
  public:
  EditorCodeRepresentation(KTextEditor::Document* document) : m_document(document) {
  }
  QString line(int line) const {
    if(line < 0 || line >= m_document->lines())
      return QString();
    return m_document->line(line);
  }
  
  virtual int lines() const {
      return m_document->lines();
  }
  
  QString text() const {
    return m_document->text();
  }
  
  bool setText(QString text) {
    return m_document->setText(text);
  }
  
  bool fileExists(){
    return QFile(m_document->url().path()).exists();
  }
  
  void startEdit() {
      m_document->startEditing();
  }
  
  void endEdit() {
      m_document->endEditing();
  }
  
  bool replace(const KTextEditor::Range& range, QString oldText, QString newText, bool ignoreOldText) {
      QString old = m_document->text(range);
      if(oldText != old && !ignoreOldText) {
          return false;
      }
      
      return m_document->replaceText(range, newText);
  }
  
  virtual QString rangeText(KTextEditor::Range range) const {
      return m_document->text(range);
  }
  
  private:
    KTextEditor::Document* m_document;
};

class FileCodeRepresentation : public CodeRepresentation {
  public:
    FileCodeRepresentation(IndexedString document) : m_document(document) {
        QString localFile(document.toUrl().toLocalFile());
  
        QFile file( localFile );
        if ( file.open(QIODevice::ReadOnly) ) {
          data = file.readAll();
          lineData = data.split('\n');
        }
        m_exists = file.exists();
    }
    
    QString line(int line) const {
    if(line < 0 || line >= lineData.size())
      return QString();
      
      return QString::fromLocal8Bit(lineData[line]);
    }
    
    virtual int lines() const {
        return lineData.count();
    }
    
    QString text() const {
      return QString::fromLocal8Bit(data);
    }
    
    bool setText(QString text) {
      Q_ASSERT(!onDiskChangesForbidden);
      QString localFile(m_document.toUrl().toLocalFile());

      QFile file( localFile );
      if ( file.open(QIODevice::WriteOnly) ) {
          QByteArray data = text.toLocal8Bit();
          if(file.write(data) == data.size())
              return true;
      }
      return false;
    }
    
    bool fileExists(){
      return m_exists;
    }
    
  private:
    //We use QByteArray, because the column-numbers are measured in utf-8
    IndexedString m_document;
    bool m_exists;
    QList<QByteArray> lineData;
    QByteArray data;
};

class ArtificialStringData : public QSharedData {
    public:
    ArtificialStringData(QString data) {
        setData(data);
    }
    void setData(QString data) {
        m_data = data;
        m_lineData = m_data.split('\n');
    }
    QString data() const {
        return m_data;
    }
    QStringList lines() const {
        return m_lineData;
    }
    private:
    QString m_data;
    QStringList m_lineData;
};

class StringCodeRepresentation : public CodeRepresentation {
  public:
    StringCodeRepresentation(KSharedPtr<ArtificialStringData> _data) : data(_data) {
    }
    
    QString line(int line) const {
    if(line < 0 || line >= data->lines().size())
      return QString();
      
      return data->lines()[line];
    }
    
    virtual int lines() const {
        return data->lines().count();
    }
    
    QString text() const {
        return data->data();
    }
    
    bool setText(QString text) {
        data->setData(text);
        return true;
    }
    
    bool fileExists(){
        return false;
    }
    
  private:
    KSharedPtr<ArtificialStringData> data;
};

static QHash<IndexedString, KSharedPtr<ArtificialStringData> > artificialStrings;

CodeRepresentation::Ptr createCodeRepresentation(IndexedString url) {
    if(artificialStrings.contains(url))
        return CodeRepresentation::Ptr(new StringCodeRepresentation(artificialStrings[url]));
    
  IDocument* document = ICore::self()->documentController()->documentForUrl(url.toUrl());
  if(document && document->textDocument())
    return CodeRepresentation::Ptr(new EditorCodeRepresentation(document->textDocument()));
  else
    return CodeRepresentation::Ptr(new FileCodeRepresentation(url));
}

void CodeRepresentation::setDiskChangesForbidden(bool changesForbidden)
{
    onDiskChangesForbidden = changesForbidden;
}

InsertArtificialCodeRepresentation::InsertArtificialCodeRepresentation(IndexedString file, QString text) : m_file(file) {
    Q_ASSERT(!artificialStrings.contains(m_file));
    artificialStrings.insert(file, KSharedPtr<ArtificialStringData>(new ArtificialStringData(text)));
}

InsertArtificialCodeRepresentation::~InsertArtificialCodeRepresentation() {
    Q_ASSERT(artificialStrings.contains(m_file));
    artificialStrings.remove(m_file);
}

void InsertArtificialCodeRepresentation::setText(QString text) {
    Q_ASSERT(artificialStrings.contains(m_file));
    artificialStrings[m_file]->setData(text);
}

QString InsertArtificialCodeRepresentation::text() {
    Q_ASSERT(artificialStrings.contains(m_file));
    return artificialStrings[m_file]->data();
}

}

