#ifndef __FILECREATE_FILETYPE_H__
#define __FILECREATE_FILETYPE_H__

#include <qstring.h>
#include <qptrlist.h>

class FileCreateFileType {

public:

  FileCreateFileType() : m_enabled(false) {
    m_subtypes.setAutoDelete(true);
  }

  void setName(const QString & name) { m_name = name; }
  QString name() const { return m_name; }
  void setExt(const QString & ext) { m_ext = ext; }
  QString ext() const { return m_ext; }
  void setCreateMethod(const QString & createMethod) { m_createMethod = createMethod; }
  QString createMethod() const { return m_createMethod; }
  void setSubtypeRef(const QString & subtypeRef) { m_subtypeRef = subtypeRef; }
  QString subtypeRef() const { return m_subtypeRef; }
  void setIcon(const QString & iconName) { m_iconName = iconName; }
  QString icon() const { return m_iconName; }
  void setDescr(const QString & descr) { m_descr = descr; }
  QString descr() const { return m_descr; }
  void setEnabled(bool on) { m_enabled = on; }
  bool enabled() const { return m_enabled; }

  void setSubtypesEnabled(bool enabled = true);
  
  void addSubtype(const FileCreateFileType * subtype) { m_subtypes.append(subtype); }
  QPtrList<FileCreateFileType> subtypes() const { return m_subtypes; }
  
private:
  QString m_name;
  QString m_ext;
  QString m_createMethod;
  QString m_subtypeRef;
  QString m_iconName;
  QString m_descr;

  bool m_enabled;
  
  QPtrList<FileCreateFileType> m_subtypes;

};

#endif
