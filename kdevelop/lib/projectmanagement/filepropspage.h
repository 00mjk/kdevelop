#ifndef FILEPROPSPAGE_H
#define FILEPROPSPAGE_H
#include "filepropspagebase.h"
#include <qstring.h>
#include <qlist.h>


class ClassFileProp {
 public:
  QString m_classname;
  QString m_implfile;
  QString m_headerfile;
  QString m_baseclass;
  QString m_description; // rich text
  /** to idetify this object*/
  QString m_key; 
  bool m_change_baseclass;
};

class FilePropsPage : public FilePropsPageBase
{ 
    Q_OBJECT

public:
    FilePropsPage( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~FilePropsPage();
    void setClassFileProps(QList<ClassFileProp> props,bool different_header_impl=true);
    QList<ClassFileProp> getClassFileProps();

public slots:
    void slotSelectionChanged(); 
 virtual void slotClassnameChanged(const QString&);
 protected:
 
 QList<ClassFileProp>* m_props;
 bool m_different_header_impl;
 uint m_current_class;
 
};


#endif // FILEPROPSPAGE_H
