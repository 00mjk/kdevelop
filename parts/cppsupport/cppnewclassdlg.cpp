/***************************************************************************
*   Copyright (C) 1998 by Sandy Meier                                     *
*   smeier@rz.uni-potsdam.de                                              *
*   Copyright (C) 1999 by Benoit.Cerrina                                  *
*   Benoit.Cerrina@writeme.com                                            *
*   Copyright (C) 2002 by Bernd Gehrmann                                  *
*   bernd@kdevelop.org                                                    *
*   Copyright (C) 2003 by Eray Ozkural                                    *
*   <erayo@cs.bilkent.edu.tr>                                             *
*   Copyright (C) 2003 by Alexander Dymo                                  *
*   cloudtemple@mksat.net                                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "cppnewclassdlg.h"

#include <qcheckbox.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qradiobutton.h>
#include <qregexp.h>
#include <qtextedit.h>
#include <qrect.h>
#include <qstyle.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qcombobox.h>
#include <qlistview.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <klineedit.h>
#include <kdeversion.h>

#include "kdevplugin.h"
#include "kdevproject.h"
#include "kdevsourceformatter.h"
#include "domutil.h"
#include "filetemplate.h"

#include "classstore.h"
#include "parsedmethod.h"
#include "classgeneratorconfig.h"

CppNewClassDialog::CppNewClassDialog(KDevPlugin *part, QWidget *parent, const char *name)
	: CppNewClassDialogBase(parent, name)
{
    headerModified = false;
    baseincludeModified = false;
    implementationModified = false;
    m_part = part;
    // read file template configuration
    //    KDevProject *project = part->project();
    QDomDocument &dom = *part->projectDom();
    interface_url = DomUtil::readEntry(dom, "/cppsupportpart/filetemplates/interfaceURL");
    implementation_url = DomUtil::readEntry(dom, "/cppsupportpart/filetemplates/implementationURL");
    interface_suffix = DomUtil::readEntry(dom, "/cppsupportpart/filetemplates/interfacesuffix", ".h");
    implementation_suffix = DomUtil::readEntry(dom, "/cppsupportpart/filetemplates/implementationsuffix", ".cpp");
    lowercase_filenames = DomUtil::readBoolEntry(dom, "/cppsupportpart/filetemplates/lowercasefilenames", true);
    m_parse = DomUtil::readEntry( *m_part->projectDom(), "/cppsupportpart/newclass/filenamesetting","none");
    //    name_handler_combo->setCurrentText(m_parse);
    baseclasses_view->setSorting(-1);
    constructors_view->setSorting(-1);

    accessMenu = new QPopupMenu(this);
    accessMenu->insertItem(i18n("Use as Private"),
        this, SLOT(changeToPrivate()), 0, 1);
    accessMenu->insertItem(i18n("Use as Protected"),
        this, SLOT(changeToProtected()), 0, 2);
    accessMenu->insertItem(i18n("Use as Public"),
        this, SLOT(changeToPublic()), 0, 3);
    accessMenu->insertSeparator();
    accessMenu->insertItem(i18n("Unset"),
        this, SLOT(changeToInherited()), 0, 5);

    overMenu = new QPopupMenu(this);
    overMenu->insertItem(i18n("Extend Base Class Functionality"),
        this, SLOT(extendFunctionality()), 0, 11);
    overMenu->insertItem(i18n("Replace Base Class Method"),
        this, SLOT(replaceFunctionality()), 0, 12);

    comp = basename_edit->completionObject();
    setCompletion(m_part->classStore());
    setCompletion(m_part->ccClassStore());
    classname_edit->setFocus();
}


CppNewClassDialog::~CppNewClassDialog()
{
    delete comp;
}

void CppNewClassDialog::setCompletion(ClassStore *store)
{
  QValueList<ParsedClass*> classlist = store->getSortedClassList();
  QValueList<ParsedClass*>::iterator it;
  for ( it = classlist.begin(); it != classlist.end(); ++it )
    comp->addItem((*it)->name());
}

void CppNewClassDialog::nameHandlerChanged(const QString &text)
{
	DomUtil::writeEntry( *m_part->projectDom(), "/cppsupportpart/newclass/filenamesetting",text);
	m_parse = text;
	classNameChanged(classname_edit->text());
}

void CppNewClassDialog::classNameChanged(const QString &text)
{
    QString str = text;

    if (!headerModified) {
        QString header = str + interface_suffix;
        switch( gen_config->fileCase() )
        {
            case ClassGeneratorConfig::LowerCase:
                header = header.lower();
                break;
            case ClassGeneratorConfig::UpperCase:
                header = header.upper();
                break;
            default:;
        }
        header = header.replace(QRegExp("(template *<.*> *)?(class *)?"), "");
        header_edit->setText(header);
    }
    if (!implementationModified) {
        QString implementation;
        if (str.contains("template"))
            implementation = str + "_impl" + interface_suffix;
        else
            implementation = str + implementation_suffix;
        switch( gen_config->fileCase() )
        {
            case ClassGeneratorConfig::LowerCase:
                implementation = implementation.lower();
                break;
            case ClassGeneratorConfig::UpperCase:
                implementation = implementation.upper();
                break;
            default:;
        }
        implementation = implementation.replace(QRegExp("(template *<.*> *)?(class *)?"), "");
        implementation_edit->setText(implementation);
    }
}

void CppNewClassDialog::baseclassname_changed(const QString &text)
{
    if ( (basename_edit->hasFocus()) && (!baseincludeModified) ) {
        QString header = text + interface_suffix;
        switch( gen_config->superCase() )
        {
            case ClassGeneratorConfig::LowerCase:
                header = header.lower();
                break;
            case ClassGeneratorConfig::UpperCase:
                header = header.upper();
                break;
            default:;
        }
        header = header.replace(QRegExp(" *<.*>"), "");
        baseinclude_edit->setText(header);
    }
}

void CppNewClassDialog::baseIncludeChanged(const QString &text)
{
    if (baseinclude_edit->hasFocus())
    {
        baseincludeModified = true;
        if ( baseclasses_view->selectedItem() )
            baseclasses_view->selectedItem()->setText(4, "true");
    }
    if ( baseclasses_view->selectedItem() )
    {
        baseclasses_view->selectedItem()->setText(3, text);
    }
}

void CppNewClassDialog::headerChanged()
{
	// Only if a change caused by the user himself
	if (header_edit->hasFocus())
		headerModified = true;
}


void CppNewClassDialog::implementationChanged()
{
	// Only if a change caused by the user himself
	if (implementation_edit->hasFocus())
		implementationModified = true;
}

void CppNewClassDialog::checkObjCInheritance(int val)
{
    childclass_box->setEnabled(!val);
    gtk_box->setEnabled(!val);
    qobject_box->setEnabled(!val);
    namespace_edit->setEnabled(!val);
    class_tabs->setTabEnabled(tab2, !val);
/*    virtual_box->setEnabled(!val);
    public_button->setEnabled(!val);
    protected_button->setEnabled(!val);
    private_button->setEnabled(!val);*/
    if (val && (baseclasses_view->childCount() > 1))
        if (KMessageBox::warningContinueCancel(this,
        i18n("Objective C does not support multiple inheritance.\nOnly the first base class in the list will be taken into account."),
        i18n("Warning"), KStdGuiItem::cont(), "Check Objective C inheritance rules" ) == KMessageBox::Cancel)
        objc_box->setChecked(false);
}

void CppNewClassDialog::checkQWidgetInheritance(int val)
{
    if (val)
    {
        qobject_box->setEnabled(val);
	qobject_box->setChecked(val);
        objc_box->setEnabled(!val);
	gtk_box->setEnabled(!val);
    }
    else if (qobject_box->isChecked())
    {
        objc_box->setEnabled(false);
	gtk_box->setEnabled(false);
    }
    else
    {
        objc_box->setEnabled(!val);
	gtk_box->setEnabled(!val);
    }


    if (val)
    {
        if (baseclasses_view->childCount() == 0)
        {
            addBaseClass();
            basename_edit->setText("QWidget");
        }
/*        constructors_cpp_edit->append(classname_edit->text() + "::" + classname_edit->text() +
            "(QWidget *parent, const char *name):\n    QWidget(parent, name)\n{\n}\n");
        constructors_h_edit->append(classname_edit->text() + "(QWidget *parent, const char *name);\n");*/
    }

    if (val && (baseclasses_view->childCount() > 1))
        if (KMessageBox::warningContinueCancel(this,
            i18n("Multiple inheritance requires QObject derivative to be first and unique in base class list."),
            i18n("Warning"), KStdGuiItem::cont(), "Check QWidget inheritance rules" ) == KMessageBox::Cancel)
            childclass_box->setChecked(false);
}

void CppNewClassDialog::qobject_box_stateChanged(int val)
{
    if ( childclass_box->isChecked() )
        return;

    if (baseclasses_view->childCount() == 0)
    {
        addBaseClass();
        basename_edit->setText("QObject");
    }

    
    objc_box->setEnabled(!val);
    gtk_box->setEnabled(!val);
}

void CppNewClassDialog::gtk_box_stateChanged(int val)
{
    class_tabs->setTabEnabled(tab2, !val);
    childclass_box->setEnabled(!val);
    objc_box->setEnabled(!val);
    qobject_box->setEnabled(!val);
    namespace_edit->setEnabled(!val);

    basename_edit->setEnabled(!val);
    virtual_box->setEnabled(!val);
    public_button->setEnabled(!val);
    protected_button->setEnabled(!val);
    private_button->setEnabled(!val);
    addbaseclass_button->setEnabled(!val);
    rembaseclass_button->setEnabled(!val);
    upbaseclass_button->setEnabled(!val);
    downbaseclass_button->setEnabled(!val);
    baseclasses_view->setEnabled(!val);
    baseinclude_edit->setEnabled(!val);
}


void CppNewClassDialog::accept()
{
  ClassGenerator generator(*this);
  if (generator.generate())
    QDialog::accept();

}

void CppNewClassDialog::setStateOfInheritanceEditors(bool state, bool hideList)
{
    basename_edit->setEnabled(state);
    virtual_box->setEnabled(state);
    public_button->setEnabled(state);
    protected_button->setEnabled(state);
    private_button->setEnabled(state);
    scope_box->setEnabled(state);
    baseinclude_edit->setEnabled(state);
    if (state)
        baseclasses_view->setEnabled(state);
    else
        baseclasses_view->setEnabled(hideList ? state : true);
    rembaseclass_button->setEnabled(state);
    if (!state)
    {
        upbaseclass_button->setEnabled(state);
        downbaseclass_button->setEnabled(state);
    }
}

void CppNewClassDialog::addBaseClass()
{
    baseincludeModified = false;
    if (baseclasses_view->selectedItem())
        baseclasses_view->selectedItem()->setSelected(false);
    QListViewItem* it = new QListViewItem(baseclasses_view, baseclasses_view->lastItem(),
        QString::null, "public", QString("%1").arg(scope_box->currentItem()), QString::null, "false");
    setStateOfInheritanceEditors(true);
    public_button->setChecked(true);
    virtual_box->setChecked(false);
    basename_edit->setText(QString::null);
    basename_edit->setFocus();
    baseclasses_view->setSelected(it, true);
}

void CppNewClassDialog::remBaseClass()
{
    bool basename_focused = false;
    if (basename_edit->hasFocus())
    {
        basename_focused = true;
        basename_edit->clearFocus();
    }
    if (baseclasses_view->selectedItem())
    {
        QListViewItem *it = baseclasses_view->selectedItem();
        remClassFromAdv(it->text(0));
        baseclasses_view->selectedItem()->setSelected(false);
        if (it->itemBelow())
            baseclasses_view->setSelected(it->itemBelow(), true);
        else if (it->itemAbove())
            baseclasses_view->setSelected(it->itemAbove(), true);
        delete it;
        if (baseclasses_view->childCount() == 0)
            setStateOfInheritanceEditors(false);
        baseincludeModified = false;
    }
    if (basename_focused)
        basename_edit->setFocus();
}

void CppNewClassDialog::remBaseClassOnly()
{
    if (baseclasses_view->selectedItem())
    {
        QListViewItem *it = baseclasses_view->selectedItem();
        baseclasses_view->selectedItem()->setSelected(false);
        if (it->itemBelow())
            baseclasses_view->setSelected(it->itemBelow(), true);
        else if (it->itemAbove())
            baseclasses_view->setSelected(it->itemAbove(), true);
        delete it;
        if (baseclasses_view->childCount() == 0)
            setStateOfInheritanceEditors(false);
        baseincludeModified = true;
    }
}

void CppNewClassDialog::remClassFromAdv(QString text)
{
    removeTemplateParams(text);
    QListViewItem *it = 0;
    if ((it = access_view->findItem(text, 0)))
        delete it;
    if ((it = methods_view->findItem(text, 0)))
        delete it;
    if ((it = constructors_view->findItem(text, 0)))
    {
        /// @todo changing constructors text in constructors_cpp_edit
        // and constructors_h_edit must be implemented

/*        int *para = new int(1);
        int *index = new int(1);
        if (constructors_cpp_edit->find(text + "(", true, false, true, para, index))
        {
            qWarning("%s( found", text.latin1());
            if (para) constructors_cpp_edit->removeParagraph(*para);
        }*/
        delete it;
    }
}

void CppNewClassDialog::currBaseNameChanged(const QString &text)
{
    if ( baseclasses_view->selectedItem() && (basename_edit->hasFocus()))
    {
        if (class_tabs->isTabEnabled(tab2))
        {
            //check for this class in the adv. inheritance lists
            //and delete if it exists
            remClassFromAdv(baseclasses_view->selectedItem()->text(0));
            //parse new base class
            parseClass(text, baseclasses_view->selectedItem()->text(1));
        }
        baseclasses_view->selectedItem()->setText(0, text);
        updateConstructorsOrder();
    }
}

void CppNewClassDialog::currBasePrivateSet()
{
    if ( baseclasses_view->selectedItem() )
    {
        setAccessForBase(baseclasses_view->selectedItem()->text(0), "private");
        baseclasses_view->selectedItem()->setText(1, (virtual_box->isChecked()?"virtual ":"") + QString("private"));
    }
}

void CppNewClassDialog::currBaseProtectedSet()
{
    if ( baseclasses_view->selectedItem() )
    {
        setAccessForBase(baseclasses_view->selectedItem()->text(0), "protected");
        baseclasses_view->selectedItem()->setText(1, (virtual_box->isChecked()?"virtual ":"") + QString("protected"));
    }
}

void CppNewClassDialog::currBasePublicSet()
{
    if ( baseclasses_view->selectedItem() )
    {
        setAccessForBase(baseclasses_view->selectedItem()->text(0), "public");
        baseclasses_view->selectedItem()->setText(1, (virtual_box->isChecked()?"virtual ":"") + QString("public"));
    }
}

void CppNewClassDialog::scopeboxActivated(int value)
{
    if ( baseclasses_view->selectedItem() )
    {
        baseclasses_view->selectedItem()->setText(2, QString("%1").arg(value));
    }
}

void CppNewClassDialog::currBaseVirtualChanged(int val)
{
    if ( baseclasses_view->selectedItem() )
    {
        baseclasses_view->selectedItem()->setText(1, QString(val?"virtual ":"") +
         QString(private_button->isChecked()?"private":"") +
         QString(protected_button->isChecked()?"protected":"") +
         QString(public_button->isChecked()?"public":""));
    }
}

void CppNewClassDialog::currBaseSelected(QListViewItem *it)
{
    if (it == 0)
    {
        setStateOfInheritanceEditors(false, false);
        return;
    }
    setStateOfInheritanceEditors(true);
    basename_edit->setText(it->text(0));
    baseinclude_edit->setText(it->text(3));
    scope_box->setCurrentItem(it->text(2).toInt());
    if (it->text(1).contains("private"))
        private_button->setChecked(true);
    else
        private_button->setChecked(false);
    if (it->text(1).contains("protected"))
        protected_button->setChecked(true);
    else
        protected_button->setChecked(false);
    if (it->text(1).contains("public"))
        public_button->setChecked(true);
    else
        public_button->setChecked(false);
    if (it->text(1).contains("virtual"))
        virtual_box->setChecked(true);
    else
        virtual_box->setChecked(false);
    checkUpButtonState();
    checkDownButtonState();
    
    if ( it->text(4) == "true" )
        baseincludeModified = true;
    else
        baseincludeModified = false;
}

void CppNewClassDialog::upbaseclass_button_clicked()
{
    bool basename_focused = false;
    if (basename_edit->hasFocus())
    {
        basename_focused = true;
        basename_edit->clearFocus();
    }
    if (baseclasses_view->selectedItem())
    {
        QListViewItem *it = baseclasses_view->selectedItem();
        if (it->itemAbove())
        {
            QListViewItem *newit;
            if (it->itemAbove()->itemAbove())
                newit = new QListViewItem(baseclasses_view, it->itemAbove()->itemAbove(),
                    it->text(0), it->text(1), it->text(2), it->text(3), it->text(4));
            else
                newit = new QListViewItem(baseclasses_view, it->text(0), it->text(1), 
                    it->text(2), it->text(3), it->text(4));
            remBaseClassOnly();
            baseclasses_view->setSelected(newit, true);
            checkUpButtonState();
            updateConstructorsOrder();
        }
    }
    if (basename_focused)
        basename_edit->setFocus();
}

void CppNewClassDialog::downbaseclass_button_clicked()
{
    bool basename_focused = false;
    if (basename_edit->hasFocus())
    {
        basename_focused = true;
        basename_edit->clearFocus();
    }
    if (baseclasses_view->selectedItem())
    {
        QListViewItem *it = baseclasses_view->selectedItem();
        if (it->itemBelow())
        {
            QListViewItem *newit = new QListViewItem(baseclasses_view, it->itemBelow(),
                it->text(0), it->text(1), it->text(2), it->text(3), it->text(3));
            remBaseClassOnly();
            baseclasses_view->setSelected(newit, true);
            setStateOfInheritanceEditors(true);
            checkDownButtonState();
            updateConstructorsOrder();
        }
    }
    if (basename_focused)
        basename_edit->setFocus();
}

void CppNewClassDialog::updateConstructorsOrder()
{
    QListViewItemIterator it( baseclasses_view );
    QListViewItem *c_it;
    QListViewItem *fc_it = 0;

    while ( it.current() )
    {
        if ( (c_it = constructors_view->findItem(it.current()->text(0), 0)) )
        {
            c_it->moveItem(fc_it);
            fc_it = c_it;
        }
        ++it;
    }
}


void CppNewClassDialog::checkUpButtonState()
{
    if (baseclasses_view->selectedItem())
        upbaseclass_button->setEnabled(baseclasses_view->selectedItem()->itemAbove());
}

void CppNewClassDialog::checkDownButtonState()
{
    if (baseclasses_view->selectedItem())
        downbaseclass_button->setEnabled(baseclasses_view->selectedItem()->itemBelow());
}

void CppNewClassDialog::baseclasses_view_selectionChanged()
{
/*    if (baseclasses_view->selectedItem())
    {
        setStateOfInheritanceEditors(false, false);
    }*/
}

void CppNewClassDialog::changeToPrivate()
{
    if (access_view->selectedItem())
        access_view->selectedItem()->setText(2, "private");
}

void CppNewClassDialog::changeToProtected()
{
    if (access_view->selectedItem())
        access_view->selectedItem()->setText(2, "protected");
}

void CppNewClassDialog::changeToPublic()
{
    if (access_view->selectedItem())
        access_view->selectedItem()->setText(2, "public");
}

void CppNewClassDialog::changeToInherited()
{
    if (access_view->selectedItem())
        access_view->selectedItem()->setText(2, QString::null);
}

void CppNewClassDialog::newTabSelected(const QString& /*text*/)
{
/*    if (text == i18n("&Advanced Inheritance"))
        reloadAdvancedInheritance(true);*/
}

void CppNewClassDialog::newTabSelected(QWidget* /*w*/)
{
/*    if ( QString(w->name()) == QString("tab2"))
    {
        reloadAdvancedInheritance(false);
    }*/
}


void CppNewClassDialog::reloadAdvancedInheritance(bool /*clean*/)
{
/*    clearConstructorsList(clean);
    clearMethodsList(clean);
    clearUpgradeList(clean);

    QListViewItemIterator it( baseclasses_view );
    while ( it.current() )
    {
        if (! (it.current()->text(0).isNull()) )
        {
            parseClass(it.current()->text(0), it.current()->text(1));
        }
        ++it;
    }*/
}

void CppNewClassDialog::parseClass(QString clName, QString inheritance)
{
    QString templateAdd = templateActualParamsFormatted(clName);
    removeTemplateParams(clName);
    ParsedClass * myClass = m_part->classStore()->getClassByName(clName);
    if (! myClass)
        myClass = m_part->ccClassStore()->getClassByName(clName);
    if (myClass)
    {
        PCheckListItem<ParsedClass*> *it = new PCheckListItem<ParsedClass*>(myClass, constructors_view, myClass->name());
        it->templateAddition = templateAdd;
        PListViewItem<ParsedClass*> *over = new PListViewItem<ParsedClass*>(myClass, methods_view, myClass->name());
        over->templateAddition = templateAdd;
        QListViewItem *over_methods = new QListViewItem(over, i18n("Methods"));
        QListViewItem *over_slots = new QListViewItem(over, i18n("Slots (Qt-specific)"));
        PListViewItem<ParsedClass*> *access = new PListViewItem<ParsedClass*>(myClass, access_view, myClass->name());
        QListViewItem *access_methods = new QListViewItem(access, i18n("Methods"));
        QListViewItem *access_slots = new QListViewItem(access, i18n("Slots (Qt-specific)"));
        QListViewItem *access_attrs = new QListViewItem(access, i18n("Attributes"));

        ParsedMethod *method = 0;
        myClass->methodIterator.toFirst();
        while ( (method = myClass->methodIterator.current()) != 0)
        {
            if (isConstructor(myClass->name(), method))
//            if (method->isConstructor())
            {
                addToConstructorsList(it, method);
            }
            else
            {
                addToMethodsList(over_methods, method);

                // display only public and protected methods of the base class
                if ( (!method->isDestructor()) && (!method->isPrivate()) )
                {
                    // see what modifier is given for the base class
                    QString inhModifier;
                    //protected inheritance gives protected methods
                    if (inheritance.contains("protected")) inhModifier = "protected";
                    //private inheritance gives private methods
                    else if (inheritance.contains("private")) inhModifier = "private";
                    //public inheritance gives protected and public methods
                    else if (inheritance.contains("public")) inhModifier = method->isPublic() ? "public" : "protected";
                    addToUpgradeList(access_methods, method, inhModifier );
                }
            }
            ++(myClass->methodIterator);
        }

        ParsedAttribute *attr = 0;
        myClass->attributeIterator.toFirst();
        while ( (attr = myClass->attributeIterator.current()) != 0 )
        {
            if (!attr->isPrivate())
            {
                QString inhModifier;
                //protected inheritance gives protected attributes
                if (inheritance.contains("protected")) inhModifier = "protected";
                //private inheritance gives private attributes
                else if (inheritance.contains("private")) inhModifier = "private";
                //public inheritance gives protected and public attributes
                else if (inheritance.contains("public")) inhModifier = attr->isPublic() ? "public" : "protected";
                addToUpgradeList(access_attrs, attr, inhModifier );
            }
            ++(myClass->attributeIterator);
        }

        ParsedMethod *slot = 0;
        myClass->slotIterator.toFirst();
        while ( (slot = myClass->slotIterator.current()) != 0 )
        {
            addToMethodsList(over_slots, slot);

            if (!slot->isPrivate())
            {
                QString inhModifier;
                //protected inheritance gives protected attributes
                if (inheritance.contains("protected")) inhModifier = "protected";
                //private inheritance gives private attributes
                else if (inheritance.contains("private")) inhModifier = "private";
                //public inheritance gives protected and public attributes
                else if (inheritance.contains("public")) inhModifier = slot->isPublic() ? "public" : "protected";
                addToUpgradeList(access_slots, slot, inhModifier );
            }
            ++(myClass->slotIterator);
        }

        parsedClasses << clName;
    }
}

bool CppNewClassDialog::isConstructor(QString className, ParsedMethod *method)
{
//  regexp:  myclass\\s*\\(\\s*(const)?\\s*myclass\\s*&[A-Za-z_0-9\\s]*\\) is for copy constructors
    if ( (className == method->name()) )
    {
        if ( method->asString().contains(QRegExp(className + "\\s*\\(\\s*(const)?\\s*" + className + "\\s*&[A-Za-z_0-9\\s]*\\)", true, false)) )
            return false;
        else
            return true;
    }
    else
        return false;
}

void CppNewClassDialog::addToConstructorsList(QCheckListItem *myClass, ParsedMethod *method)
{
    /*UNUSED! PCheckListItem<ParsedMethod*> *it = */ new PCheckListItem<ParsedMethod*>(method, myClass, method->asString(), QCheckListItem::RadioButton);
}

void CppNewClassDialog::addToMethodsList(QListViewItem *parent, ParsedMethod *method)
{
    if (!method->isDestructor())
    {
        PCheckListItem<ParsedMethod*> *it = new PCheckListItem<ParsedMethod*>(method, parent, method->asString(), QCheckListItem::CheckBox);
        it->setText(1, i18n("extend"));
    }
}

void CppNewClassDialog::addToUpgradeList(QListViewItem *parent, ParsedMethod *method, QString modifier)
{
    PListViewItem<ParsedMethod*> *it = new PListViewItem<ParsedMethod*>(method, parent, method->asString());
    it->setText(1, modifier);
}

void CppNewClassDialog::addToUpgradeList(QListViewItem *parent, ParsedAttribute *attr, QString modifier)
{
    PListViewItem<ParsedAttribute*> *it = new PListViewItem<ParsedAttribute*>(attr, parent, attr->asString());
    it->setText(1, modifier);
}

void CppNewClassDialog::clear_selection_button_clicked()
{
    QListViewItemIterator it( constructors_view );
    while ( it.current() )
    {
        PCheckListItem<ParsedMethod*> *curr;
        if ( (curr = dynamic_cast<PCheckListItem<ParsedMethod*>* >(it.current()) ) )
            curr->setOn(false);
        ++it;
    }
}

void CppNewClassDialog::clearConstructorsList(bool clean)
{
    if (clean)
        constructors_view->clear();
/*    else
    {
        QListViewItemIterator it( constructors_view );
        while ( it.current() )
        {
            if ( ! currBaseClasses.contains(it.current().text(0)) )
                delete it.current();
            ++it;
        }
    }*/
}

void CppNewClassDialog::clearMethodsList(bool clean)
{
    if (clean)
        methods_view->clear();
}

void CppNewClassDialog::clearUpgradeList(bool clean)
{
    if (clean)
        access_view->clear();
}

void CppNewClassDialog::setAccessForItem(QListViewItem *curr, QString newAccess, bool isPublic)
{
    if (newAccess == "public")
        curr->setText(1, isPublic ? "public" : "protected");
    else
        curr->setText(1, newAccess);
    if (!curr->text(2).isNull())
    {
        if ( (curr->text(2) == "private") && ((newAccess == "public") || (newAccess == "protected")) )
            curr->setText(2, QString::null);
        if ( (curr->text(2) == "protected") && ((newAccess == "public") && (isPublic)) )
            curr->setText(2, QString::null);
    }
}

void CppNewClassDialog::setAccessForBase(QString baseclass, QString newAccess)
{
    QListViewItem *base;

    if ( (base = access_view->findItem(baseclass, 0)) )
    {
        QListViewItemIterator it( base );
        while ( it.current() )
        {
            if ( !it.current()->text(1).isNull() )
            {
                PListViewItem<ParsedAttribute*> *curr;
                PListViewItem<ParsedMethod*> *curr_m;
                if ( (curr = dynamic_cast<PListViewItem<ParsedAttribute*>* >(it.current())) )
                    setAccessForItem(curr, newAccess, curr->item()->isPublic());
                else if ( (curr_m = dynamic_cast<PListViewItem<ParsedMethod*>* >(it.current())) )
                    setAccessForItem(curr_m, newAccess, curr_m->item()->isPublic());
            }
            ++it;
        }
    }
}


void CppNewClassDialog::access_view_mouseButtonPressed( int button, QListViewItem * item, const QPoint &p, int /*c*/ )
{
    if (item && ( (button == LeftButton) || (button == RightButton)) && (item->depth() > 1) )
    {
        accessMenu->setItemEnabled(1, true );
        accessMenu->setItemEnabled(2, true );
        accessMenu->setItemEnabled(3, true );
        if (item->text(1) == "protected")
        {
            accessMenu->setItemEnabled(1, false);
        }
        if (item->text(1) == "public")
        {
            accessMenu->setItemEnabled(1, false);
            accessMenu->setItemEnabled(2, false);
        }
        accessMenu->exec(p);

/*        accessMenu->setItemEnabled(1, item->text(1) == "private" ? false : true );
        accessMenu->setItemEnabled(2, item->text(1) == "protected" ? false : true );
        accessMenu->setItemEnabled(3, item->text(1) == "public" ? false : true );*/
    }
}


void CppNewClassDialog::methods_view_mouseButtonPressed(int button ,QListViewItem * item, const QPoint&p ,int /*c*/)
{
    if (item && ( button == RightButton ) && (item->depth() > 1) && (! item->text(1).isNull()) )
    {
        overMenu->exec(p);
    }
}

void CppNewClassDialog::extendFunctionality()
{
    if (methods_view->selectedItem())
        methods_view->selectedItem()->setText(1, i18n("extend"));
}

void CppNewClassDialog::replaceFunctionality()
{
    if (methods_view->selectedItem())
        methods_view->selectedItem()->setText(1, i18n("replace"));
}

void CppNewClassDialog::selectall_button_clicked()
{
    QListViewItemIterator it( constructors_view );
    while ( it.current() )
    {
	PCheckListItem<ParsedMethod*> *curr;
	if ( (curr = dynamic_cast<PCheckListItem<ParsedMethod*>* >(it.current()) ) )
	    curr->setOn(true);
	++it;
    }
}

void CppNewClassDialog::to_constructors_list_clicked()
{
    QString templateAdd = templateStrFormatted().isEmpty() ? QString::null : templateStrFormatted() + "\n";
    QString constructor_h = classNameFormatted();
    QString constructor_cpp = templateAdd + classNameFormatted() + templateParamsFormatted() + "::" + classNameFormatted();
    constructor_h += "(";
    constructor_cpp += "(";
    QString params_h;
    QString params_cpp;
    QString base;
    int unnamed = 1;

    QListViewItemIterator it( constructors_view );
    while ( it.current() )
    {
        PCheckListItem<ParsedMethod*> *curr;
        if ( (curr = dynamic_cast<PCheckListItem<ParsedMethod*>* >(it.current())) )
        {
            if (curr->isOn() && curr->parent())
            {
                //fill the base classes list
                base += base.isEmpty() ? ": " : ", ";
                base += curr->parent()->text(0);
                PCheckListItem<ParsedClass*> *p;
                if ( (p = dynamic_cast<PCheckListItem<ParsedClass*>* >(curr->parent())) )
                {
                    base += p->templateAddition;
                }
                params_h += params_h.isEmpty() ? "" : ", ";

                //fill arguments for both constructor and base class initializer
                //parser does not allow to process default values now ?!
                ParsedArgument *arg;
                QString cparams;
                QString bparams;
                for (arg = curr->item()->arguments.first(); arg; arg = curr->item()->arguments.next())
                {
                    bparams += bparams.isEmpty() ? "" : ", ";
                    cparams += cparams.isEmpty() ? "" : ", ";
                    cparams += arg->type() + " ";
                    if (arg->name().isNull())
                    {
                        cparams += QString("arg%1").arg(unnamed);
                        bparams += QString("arg%1").arg(unnamed++);
                    }
                    else
                    {
                        bparams += arg->name();
                        cparams += arg->name();
                    }
                }
                params_h += cparams;
                params_cpp = params_h;
                base += "(" + bparams + ")";
            }
        }
        ++it;
    }

    constructor_cpp += params_cpp + ")" + base + QString("\n{\n}\n\n\n");
    constructor_h += params_h + ");\n\n";

    constructors_h_edit->append(constructor_h);
    constructors_cpp_edit->append(constructor_cpp);
}

void CppNewClassDialog::updateClassStore()
{
}




bool CppNewClassDialog::ClassGenerator::validateInput()
{
  className = dlg.classname_edit->text().simplifyWhiteSpace();
  QString temp = className;
  className.replace(QRegExp("template *<.*> *(class *)?"), "");
  templateStr = temp.replace(QRegExp(QRegExp::escape(className)), "");
  templateStr.replace(QRegExp(" *class *$"), "");

  templateParams = templateStr;
  templateParams.replace(QRegExp("^ *template *"), "");
  templateParams.replace(QRegExp(" *class *"), "");
  templateParams.simplifyWhiteSpace();

  if (className.isEmpty()) {
    KMessageBox::error(&dlg, i18n("You must enter a classname."));
    return false;
  }

  header = dlg.header_edit->text().simplifyWhiteSpace();
  if (header.isEmpty()) {
    KMessageBox::error(&dlg, i18n("You must enter a name for the header file."));
    return false;
  }
  implementation = dlg.implementation_edit->text().simplifyWhiteSpace();
  if (implementation.isEmpty() ){
    KMessageBox::error(&dlg, i18n("You must enter a name for the implementation file."));
    return false;
  }

  // FIXME:
  if (header.find('/') != -1 || implementation.find('/') != -1) {
    KMessageBox::error(&dlg, i18n("Generated files will always be added to the "
				  "active directory, so you must not give an "
				  "explicit subdirectory."));
    return false;
  }

  return true;
}


bool CppNewClassDialog::ClassGenerator::generate()
{
  if (!validateInput())
    return false;

  common_text();

  gen_implementation();

  gen_interface();

  return true;
}

void CppNewClassDialog::ClassGenerator::common_text()
{

  // common

  project = dlg.m_part->project();
  subDir = project->projectDirectory() + "/";
  if (!project->activeDirectory().isEmpty())
    subDir += project->activeDirectory() + "/";
  headerPath = subDir + header;
  implementationPath = subDir + implementation;

  if (QFileInfo(headerPath).exists() || QFileInfo(implementationPath).exists()) {
    KMessageBox::error(&dlg, i18n("KDevelop is not able to add classes "
				  "to existing header or implementation files."));
    return;
  }

  namespaceStr = dlg.namespace_edit->text();
  namespaces = QStringList::split(QString("::"), namespaceStr);

  childClass = dlg.childclass_box->isChecked();
  objc = dlg.objc_box->isChecked();
  qobject = dlg.qobject_box->isChecked();
  gtk = dlg.gtk_box->isChecked();

  if ( (dlg.baseclasses_view->childCount() == 0) && childClass)
    new QListViewItem(dlg.baseclasses_view, "QWidget", "public");
  if (objc && (dlg.baseclasses_view->childCount() == 0))
    new QListViewItem(dlg.baseclasses_view, "NSObject", "public");

  if (dlg.documentation_edit->text().isEmpty() && (!dlg.gen_config->doc_box->isChecked()) )
    doc = "";
  else
  {
    doc = QString("/**\n");
    if (!dlg.documentation_edit->text().isEmpty())
    {
        doc.append(dlg.documentation_edit->text());
        if (dlg.gen_config->author_box->isChecked())
            doc.append("\n\n");
    }
    QString author = DomUtil::readEntry(*dlg.m_part->projectDom(), "/general/author");
    if (dlg.gen_config->author_box->isChecked())
        doc.append("@author " + author + "\n");
    doc.append("*/");
  }

  if (!namespaceStr.isEmpty()) {
    for ( QStringList::Iterator it = namespaces.begin(); it != namespaces.end(); ++it ) {
        if (!namespaceBeg.isEmpty())
            namespaceBeg += "\n\n";
        if (!namespaceEnd.isEmpty())
            namespaceEnd += "\n\n";
        namespaceBeg += "namespace " + (*it) + " {";
        namespaceEnd += "};";
    }
  }

  //advanced constructor creation

  advConstructorsHeader = QString::null;
  advConstructorsSource = QString::null;
  if (!dlg.constructors_h_edit->text().isEmpty())
  {
      advConstructorsHeader = "    " + dlg.constructors_h_edit->text();
      advConstructorsHeader.replace(QRegExp("\n"), "\n    ");
  }
  if (!dlg.constructors_cpp_edit->text().isEmpty())
  {
      advConstructorsSource = dlg.constructors_cpp_edit->text();
  }
  advConstructorsHeader.replace(QRegExp("[\\n ]*$"), QString::null);
  advConstructorsSource.replace(QRegExp("[\\n ]*$"), QString::null);

  //advanced method overriding

  advH_public = QString::null;
  advH_public_slots = QString::null;
  advH_protected = QString::null;
  advH_protected_slots = QString::null;
  advH_private = QString::null;
  advH_private_slots = QString::null;
  advCpp = QString::null;

  QListViewItemIterator it( dlg.methods_view );
  while ( it.current() )
  {
    PCheckListItem<ParsedMethod*> *curr;
    if ( (curr = dynamic_cast<PCheckListItem<ParsedMethod*>* >(it.current())) )
    {
      if (curr->isOn() && (curr->parent()) && (curr->parent()->parent()))
      {
        QString *adv_h = 0;
        if (curr->item()->isPrivate())
          adv_h = curr->item()->isSlot() ? &advH_private_slots : &advH_private;
        if (curr->item()->isProtected())
          adv_h = curr->item()->isSlot() ? &advH_protected_slots : &advH_protected;
        if (curr->item()->isPublic())
          adv_h = curr->item()->isSlot() ? &advH_public_slots : &advH_public;

//        if (advCpp.isNull()) advCpp += "\n\n";

        QString bcName = curr->parent()->parent()->text(0);
        PListViewItem<ParsedClass*> *bc;
        if ( (bc = dynamic_cast<PListViewItem<ParsedClass*>* >(curr->parent()->parent())) )
        {
            bcName += bc->templateAddition;
        }
        genMethodDeclaration(curr->item(), className, templateStr, adv_h, &advCpp,
            (curr->text(1) == i18n("extend")) ? true : false, bcName);
      }
    }
    ++it;
  }

  //advanced access control and upgrading
  QListViewItemIterator ita( dlg.access_view );
  while ( ita.current() )
  {
    PListViewItem<ParsedAttribute*> *curr;
    PListViewItem<ParsedMethod*> *curr_m;
    if ( (curr = dynamic_cast<PListViewItem<ParsedAttribute*>* >(ita.current())) )
    {
        if ((!curr->text(2).isNull()) && (curr->parent()) && (curr->parent()->parent()) )
        {
            QString *adv_h = 0;
            if (curr->text(2) == "private") adv_h = &advH_private;
            if (curr->text(2) == "public") adv_h = &advH_public;
            if (curr->text(2) == "protected") adv_h = &advH_protected;

/*    if ((*adv_h).isNull())
            *adv_h += "\n\n";*/

            *adv_h += QString("    using ") + curr->parent()->parent()->text(0) + "::"  + curr->item()->name() + ";\n";
        }
    }
    else if ( (curr_m = dynamic_cast<PListViewItem<ParsedMethod*>* >(ita.current())) )
    {
        if ((!curr_m->text(2).isNull())  && (curr_m->parent()) && (curr_m->parent()->parent()) )
        {
            QString *adv_h = 0;
            if (curr_m->text(2) == "private") adv_h = &advH_private;
            if (curr_m->text(2) == "public") adv_h = &advH_public;
            if (curr_m->text(2) == "protected") adv_h = &advH_protected;

/*    if ((*adv_h).isNull())
        *adv_h += "\n\n";*/

            QString methodName = curr_m->item()->name();
            if (!methodName.contains(QRegExp("^[a-zA-z_]")))
                methodName = "operator" + methodName;
            *adv_h += "    using " + curr_m->parent()->parent()->text(0) + "::"  + methodName + ";\n";
        }
    }
    ++ita;
  }

  QRegExp e("[\\n ]*$");
  advH_public.replace(e, QString::null);
  advH_public_slots.replace(e, QString::null);
  advH_protected.replace(e, QString::null);
  advH_protected_slots.replace(e, QString::null);
  advH_private.replace(e, QString::null);
  advH_private_slots.replace(e, QString::null);
  advCpp.replace(e, QString::null);
}

void CppNewClassDialog::ClassGenerator::genMethodDeclaration(ParsedMethod *method,
    QString className, QString templateStr, QString *adv_h, QString *adv_cpp, bool extend, QString baseClassName )
{
/*    if ((*adv_h).isNull())
        *adv_h += "\n\n";*/
    QString methodName = method->name();
    if (!methodName.contains(QRegExp("^[a-zA-z_]")))
        methodName = "operator" + methodName;
    *adv_h += "    " + (method->isVirtual() ? QString("virtual ") : QString(""))
        + (method->isStatic() ? QString("static ") : QString(""))
        + method->type() + " " + methodName + "(";
    if (!templateStr.isEmpty())
        *adv_cpp += templateStr + "\n";
    *adv_cpp += method->type() + " " + className + templateParams + "::" + methodName + "(";

    ParsedArgument *arg;
    QString bparams;
    QString cparams;
    int unnamed = 1;
    for (arg = method->arguments.first(); arg; arg = method->arguments.next())
    {
        bparams += bparams.isEmpty() ? "" : ", ";
        cparams += cparams.isEmpty() ? "" : ", ";
        cparams += arg->type() + " ";
        if (arg->name().isNull())
        {
            cparams += QString("arg%1").arg(unnamed);
            bparams += QString("arg%1").arg(unnamed++);
        }
        else
        {
            bparams += arg->name();
            cparams += arg->name();
        }
    }
    *adv_h += cparams + ")" + (method->isConst() ? " const" : "") + ";\n";
    *adv_cpp += cparams + ")" + (method->isConst() ? " const" : "") + "\n{\n";
    if (extend)
        *adv_cpp += ((method->type() == "void") ? "    " : "    return ") +
                    baseClassName + "::" + methodName + "(" + bparams + ");\n";
    *adv_cpp += "}\n\n";
}


void CppNewClassDialog::ClassGenerator::gen_implementation()
{

  // implementation

  QString classImpl;
  if (dlg.filetemplate_box->isChecked()) {
    QDomDocument dom = *dlg.m_part->projectDom();
    if(DomUtil::readBoolEntry(dom,"/cppsupportpart/filetemplates/choosefiles",false))
      classImpl = FileTemplate::read(dlg.m_part, DomUtil::readEntry(dom,"/cppsupportpart/filetemplates/implementationURL",""), FileTemplate::Custom);
    else
      classImpl = FileTemplate::read(dlg.m_part, "cpp");
  }
  QFileInfo fi(implementationPath);
  QString module = fi.baseName();
  QString basefilename = fi.baseName(true);
  classImpl.replace(QRegExp("\\$MODULE\\$"),module);
  classImpl.replace(QRegExp("\\$FILENAME\\$"),basefilename);

  if (objc) {
    classImpl += dlg.gen_config->objcSource();
  } else if (gtk)
  {
    classImpl += dlg.gen_config->gtkSource();
  } else {
    classImpl += dlg.gen_config->cppSource();
/*    classImpl += QString( 
			 "#include \"$HEADER$\"\n"
			 "\n"
			 "\n")
      + namespaceBeg
      + ( advConstructorsSource.isNull() ? QString("$CLASSNAME$::$CLASSNAME$($ARGS$)\n"
		"$BASEINITIALIZER$"
		"{\n"
		"}\n") : advConstructorsSource )
      + QString("\n"
		"$CLASSNAME$::~$CLASSNAME$()\n"
		"{\n"
		"}\n")
      + advCpp
      + namespaceEnd;*/
  }

  QString relPath;
  for (int i = implementation.findRev('/'); i != -1; i = implementation.findRev('/', --i))
    relPath += "../";

  QString constructors = (advConstructorsSource.isNull() ? QString("$TEMPLATESTR$\n$CLASSNAME$$TEMPLATEPARAMS$::$CLASSNAME$($ARGS$)\n"
    "$BASEINITIALIZER$"
    "{\n"
    "}") : advConstructorsSource)
    + QString("\n\n\n"
        "$TEMPLATESTR$\n$CLASSNAME$$TEMPLATEPARAMS$::~$CLASSNAME$()\n"
        "{\n"
        "}\n");

  if (childClass)
  {
    argsH = "QWidget *parent = 0, const char *name = 0, WFlags f = 0";
    argsCpp = "QWidget *parent, const char *name, WFlags f";
  }
  else if (qobject)
  {
    argsH = "QObject *parent = 0, const char *name = 0";
    argsCpp = "QObject *parent, const char *name";
  }
  else
  {
    argsH = "";
    argsCpp = "";
  }
  QString baseInitializer;

  if (childClass && (dlg.baseclasses_view->childCount() == 0))
    baseInitializer = "  : QWidget(parent, name, f)";
  else if (qobject && (dlg.baseclasses_view->childCount() == 0))
    baseInitializer = "  : QObject(parent, name)";
  else if (dlg.baseclasses_view->childCount() != 0)
  {
    QListViewItemIterator it( dlg.baseclasses_view );
    baseInitializer += " : ";
    while ( it.current() )
    {
      if (!it.current()->text(0).isNull())
      {
        if (baseInitializer != " : ")
          baseInitializer += ", ";
        if (childClass && (baseInitializer == " : "))
          baseInitializer += it.current()->text(0) + "(parent, name, f)";
        else if (qobject && (baseInitializer == " : "))
          baseInitializer += it.current()->text(0) + "(parent, name)";
        else
          baseInitializer += it.current()->text(0) + "()";
      }
      ++it;
    }
    baseInitializer += "\n";
  }

  constructors.replace(QRegExp("\\$BASEINITIALIZER\\$"), baseInitializer);
  constructors.replace(QRegExp("\\$CLASSNAME\\$"), className);
  if (templateStr.isEmpty())
  {
    constructors.replace(QRegExp("\\$TEMPLATESTR\\$\\n"), "");
    constructors.replace(QRegExp("\\$TEMPLATEPARAMS\\$"), "");
  }
  else
  {
    constructors.replace(QRegExp("\\$TEMPLATESTR\\$"), templateStr);
    constructors.replace(QRegExp("\\$TEMPLATEPARAMS\\$"), templateParams);
    classImpl.replace(QRegExp("#include \"\\$HEADER\\$\"\\n"), "");
  }
  constructors.replace(QRegExp("\\$ARGS\\$"), argsCpp);


  //remove unnesessary carriadge returns
  QString hp = relPath+header;
  beautifySource(classImpl, hp, className, namespaceBeg, constructors, advCpp, namespaceEnd, implementation);

  classImpl.replace(QRegExp("\\$HEADER\\$"), relPath+header);
  classImpl.replace(QRegExp("\\$CLASSNAME\\$"), className);
  classImpl.replace(QRegExp("\\$NAMESPACEBEG\\$"), namespaceBeg);
  classImpl.replace(QRegExp("\\$CONSTRUCTORDEFINITIONS\\$"), constructors);
  classImpl.replace(QRegExp("\\$DEFINITIONS\\$"), advCpp);
  classImpl.replace(QRegExp("\\$NAMESPACEEND\\$"), namespaceEnd);
  classImpl.replace(QRegExp("\\$FILENAME\\$"), implementation);

  if (dlg.gen_config->reformat_box->isChecked())
  {
    KDevSourceFormatter *fmt = dlg.m_part->sourceFormatter();
    if (fmt)
        classImpl = fmt->formatSource(classImpl);
  }

  kdDebug(9007) << "implementationPath = " << implementationPath << endl;

  QFile ifile(implementationPath);
  if (!ifile.open(IO_WriteOnly)) {
    KMessageBox::error(&dlg, "Cannot write to implementation file");
    return;
  }
  QTextStream istream(&ifile);
  istream << classImpl;
  ifile.close();
}


void CppNewClassDialog::ClassGenerator::gen_interface()
{
  // interface

  QString classIntf;
  if (dlg.filetemplate_box->isChecked()) {
    QDomDocument dom = *dlg.m_part->projectDom();
    if(DomUtil::readBoolEntry(dom,"/cppsupportpart/filetemplates/choosefiles",false))
      classIntf = FileTemplate::read(dlg.m_part, DomUtil::readEntry(dom,"/cppsupportpart/filetemplates/interfaceURL",""), FileTemplate::Custom);
    else
      classIntf  = FileTemplate::read(dlg.m_part, "h");
  }
  QFileInfo fi(headerPath);
  QString module = fi.baseName();
  QString basefilename = fi.baseName(true);
  classIntf.replace(QRegExp("\\$MODULE\\$"),module);
  classIntf.replace(QRegExp("\\$FILENAME\\$"),basefilename);

  if (objc) {
    classIntf += dlg.gen_config->objcHeader();
  }
  else if (gtk) {
    classIntf += dlg.gen_config->gtkHeader();
  }
  else {
    classIntf += dlg.gen_config->cppHeader();
/*    classIntf = QString("\n"
			"#ifndef $HEADERGUARD$\n"
			"#define $HEADERGUARD$\n"
			"\n"
			"$INCLUDEBASEHEADER$\n"
			"\n")
      + namespaceBeg
      + QString("class $CLASSNAME$$INHERITANCE$\n"
		"{\n"
		"$QOBJECT$"
		"public:\n")
      + ( advConstructorsHeader.isNull() ? QString("    $CLASSNAME$($ARGS$);\n") : advConstructorsHeader )
      + QString("\n    ~$CLASSNAME$();\n")
      + advH_public
      + (advH_public_slots.isNull() ? QString::fromLatin1("") : ("\n\npublic slots:" + advH_public_slots))
      + (advH_protected.isNull() ? QString::fromLatin1("") : ("\n\nprotected:" + advH_protected))
      + (advH_protected_slots.isNull() ? QString::fromLatin1("") : ("\n\nprotected slots:" + advH_protected_slots))
      + (advH_private.isNull() ? QString::fromLatin1("") : ("\n\nprivate:" + advH_private))
      + (advH_private_slots.isNull() ? QString::fromLatin1("") : ("\n\nprivate slots:" + advH_private_slots))
      + QString("};\n"
		"\n")
      + namespaceEnd
      +     "#endif\n";*/
  }

  QString headerGuard;
  switch( dlg.gen_config->defCase() ){
    case ClassGeneratorConfig::UpperCase:
        headerGuard = namespaceStr.upper() + header.upper();
        break;
    case ClassGeneratorConfig::LowerCase:
        headerGuard = namespaceStr.lower() + header.lower();
        break;
    case ClassGeneratorConfig::SameAsFileCase:
        headerGuard = dlg.header_edit->text();
        break;
    case ClassGeneratorConfig::SameAsClassCase:
        headerGuard = namespaceStr + header;
        break;
  }
  headerGuard.replace(QRegExp("\\."),"_");
  headerGuard.replace(QRegExp("::"),"_");
  QString includeBaseHeader;
  if (childClass && (dlg.baseclasses_view->childCount() == 0)) /// @todo do this only if this is a Qt class
  {
    includeBaseHeader = "#include <qwidget.h>";
  }
  else if (qobject && (dlg.baseclasses_view->childCount() == 0))
  {
    includeBaseHeader = "#include <qobject.h>";
  }

  if (objc) {
    if (dlg.baseclasses_view->firstChild())
      if (dlg.baseclasses_view->firstChild()->text(0) != "NSObject")
        includeBaseHeader = "#include "
            + (dlg.baseclasses_view->firstChild()->text(2).toInt() == 0 ? QString("<") : QString("\""))
            + dlg.baseclasses_view->firstChild()->text(3)
            + (dlg.baseclasses_view->firstChild()->text(2).toInt() == 0 ? QString(">") : QString("\""));
  } else
  {
    QListViewItemIterator it( dlg.baseclasses_view );
    while ( it.current() )
    {
      if (!it.current()->text(0).isEmpty())
//        if ((!childClass) || (it.current()->text(0) != "QWidget"))
          includeBaseHeader += (includeBaseHeader.isEmpty() ? QString(""): QString("\n")) + QString::fromLatin1( "#include " ) +
            (it.current()->text(2).toInt() == 0 ? QString("<") : QString("\""))
            + it.current()->text(3)
            + (it.current()->text(2).toInt() == 0 ? QString(">") : QString("\""));
      ++it;
    }
  }

  QString author = DomUtil::readEntry(*dlg.m_part->projectDom(), "/general/author");

  QString inheritance;
  if (dlg.baseclasses_view->childCount() > 0)
  {
    inheritance += " : ";

    QListViewItemIterator it( dlg.baseclasses_view );
    while ( it.current() )
    {
      if (!it.current()->text(0).isEmpty())
      {
        if (inheritance != " : ")
          inheritance += ", ";
        if (it.current()->text(1).contains("virtual"))
          inheritance += "virtual ";
        if (it.current()->text(1).contains("public"))
          inheritance += "public ";
        if (it.current()->text(1).contains("protected"))
          inheritance += "protected ";
        if (it.current()->text(1).contains("private"))
          inheritance += "private ";
        inheritance += it.current()->text(0);
      }
      ++it;
    }
  }
  else if (qobject)
    inheritance += ": public QObject";
    
  QString constructors = QString(advConstructorsHeader.isNull() ?
    QString("    $CLASSNAME$($ARGS$);") : advConstructorsHeader )
    + QString("\n\n    ~$CLASSNAME$();");

  constructors.replace(QRegExp("\\$CLASSNAME\\$"), className);
  constructors.replace(QRegExp("\\$ARGS\\$"), argsH);
    
  QString qobjectStr;
  if (childClass || qobject)
    qobjectStr = "Q_OBJECT";


  QString baseclass;
  if (dlg.baseclasses_view->childCount() > 0)
    baseclass = dlg.baseclasses_view->firstChild()->text(0);
  //remove unnesessary carriadge returns
  beautifyHeader(classIntf, headerGuard, includeBaseHeader, author, doc, className, templateStr,
        baseclass, inheritance, qobjectStr, argsH,
        header, namespaceBeg, constructors, advH_public, advH_public_slots,
        advH_protected, advH_protected_slots, advH_private, advH_private_slots, namespaceEnd);


  classIntf.replace(QRegExp("\\$HEADERGUARD\\$"), headerGuard);
  classIntf.replace(QRegExp("\\$INCLUDEBASEHEADER\\$"), includeBaseHeader);
  classIntf.replace(QRegExp("\\$AUTHOR\\$"), author);
  classIntf.replace(QRegExp("\\$DOC\\$"), doc);
  classIntf.replace(QRegExp("\\$TEMPLATE\\$"), templateStr);
  classIntf.replace(QRegExp("\\$CLASSNAME\\$"), className);
  if (dlg.baseclasses_view->childCount() > 0)
    classIntf.replace(QRegExp("\\$BASECLASS\\$"), dlg.baseclasses_view->firstChild()->text(0));
  classIntf.replace(QRegExp("\\$INHERITANCE\\$"), inheritance);
  classIntf.replace(QRegExp("\\$QOBJECT\\$"), qobjectStr);
  classIntf.replace(QRegExp("\\$ARGS\\$"), argsH);
  classIntf.replace(QRegExp("\\$FILENAME\\$"), header);
  classIntf.replace(QRegExp("\\$NAMESPACEBEG\\$"), namespaceBeg);
  classIntf.replace(QRegExp("\\$CONSTRUCTORDECLARATIONS\\$"), constructors);
  classIntf.replace(QRegExp("\\$PUBLICDECLARATIONS\\$"), advH_public);
  classIntf.replace(QRegExp("\\$PUBLICSLOTS\\$"), advH_public_slots);
  classIntf.replace(QRegExp("\\$PROTECTEDDECLARATIONS\\$"), QString("protected:\n") + advH_protected);
  classIntf.replace(QRegExp("\\$PROTECTEDSLOTS\\$"), QString("protected slots:\n") + advH_protected_slots);
  classIntf.replace(QRegExp("\\$PRIVATEDECLARATIONS\\$"), QString("private:\n") + advH_private);
  classIntf.replace(QRegExp("\\$PRIVATESLOTS\\$"), QString("private slots:\n") + advH_private_slots);
  classIntf.replace(QRegExp("\\$NAMESPACEEND\\$"), namespaceEnd);

  if (!templateStr.isEmpty())
    classIntf.replace(QRegExp("#endif"), "#include \"" + dlg.implementation_edit->text() + "\"\n\n#endif");

  if (dlg.gen_config->reformat_box->isChecked())
  {
    KDevSourceFormatter *fmt = dlg.m_part->sourceFormatter();
    if (fmt)
        classIntf = fmt->formatSource(classIntf);
  }

  QFile hfile(headerPath);
  if (!hfile.open(IO_WriteOnly)) {
    KMessageBox::error(&dlg, "Cannot write to header file");
    return;
  }
  QTextStream hstream(&hfile);
  hstream << classIntf;
  hfile.close();

	QStringList fileList;

	if ( project->activeDirectory().isEmpty() )
	{
		fileList.append ( project->projectDirectory() + "/" + header );
		fileList.append ( project->projectDirectory() + "/" + implementation );
	}
	else
	{
		fileList.append ( project->activeDirectory() + "/" + header );
		fileList.append ( project->activeDirectory() + "/" + implementation );
	}

	project->addFiles ( fileList );
}

void CppNewClassDialog::ClassGenerator::beautifyHeader(QString &templ, QString &headerGuard,
    QString &includeBaseHeader, QString &author, QString &doc, QString &className, QString &templateStr,
    QString &baseclass, QString &inheritance, QString &qobjectStr, QString &args,
    QString &header, QString &namespaceBeg, QString &constructors, QString &advH_public, QString &advH_public_slots,
    QString &advH_protected, QString &advH_protected_slots, QString &advH_private, QString &advH_private_slots,
    QString &namespaceEnd)
{
    if (headerGuard.isEmpty())
        templ.replace(QRegExp("\\$HEADERGUARD\\$[\\n ]*"), QString::null);
    if (includeBaseHeader.isEmpty())
        templ.replace(QRegExp("\\$INCLUDEBASEHEADER\\$[\\n ]*"), QString::null);
    if (author.isEmpty())
        templ.replace(QRegExp("\\$AUTHOR\\$[\\n ]*"), QString::null);
    if (doc.isEmpty())
        templ.replace(QRegExp("\\$DOC\\$[\\n ]*"), QString::null);
    if (className.isEmpty())
        templ.replace(QRegExp("\\$CLASSNAME\\$[\\n ]*"), QString::null);
    if (templateStr.isEmpty())
        templ.replace(QRegExp("\\$TEMPLATE\\$[\\n ]*"), QString::null);
    if (baseclass.isEmpty())
        templ.replace(QRegExp("\\$BASECLASS\\$[\\n ]*"), QString::null);
    if (inheritance.isEmpty())
        templ.replace(QRegExp("\\$INHERITANCE\\$[\\n ]*"), QString::null);
    if (qobjectStr.isEmpty())
        templ.replace(QRegExp("\\$QOBJECT\\$[\\n ]*"), QString::null);
    if (args.isEmpty())
        templ.replace(QRegExp("\\$ARGS\\$[\\n ]*"), QString::null);
    if (header.isEmpty())
        templ.replace(QRegExp("\\$FILENAME\\$[\\n ]*"), QString::null);
    if (namespaceBeg.isEmpty())
        templ.replace(QRegExp("\\$NAMESPACEBEG\\$[\\n ]*"), QString::null);
    if (constructors.isEmpty())
        templ.replace(QRegExp("\\$CONSTRUCTORDECLARATIONS\\$[\\n ]*"), QString::null);
    if (advH_public.isEmpty())
        templ.replace(QRegExp("\\$PUBLICDECLARATIONS\\$[\\n ]*"), QString::null);
    if (advH_public_slots.isEmpty())
        templ.replace(QRegExp("\\$PUBLICSLOTS\\$[\\n ]*"), QString::null);
    if (advH_protected.isEmpty())
        templ.replace(QRegExp("\\$PROTECTEDDECLARATIONS\\$[\\n ]*"), QString::null);
    if (advH_protected_slots.isEmpty())
        templ.replace(QRegExp("\\$PROTECTEDSLOTS\\$[\\n ]*"), QString::null);
    if (advH_private.isEmpty())
        templ.replace(QRegExp("\\$PRIVATEDECLARATIONS\\$[\\n ]*"), QString::null);
    if (advH_private_slots.isEmpty())
        templ.replace(QRegExp("\\$PRIVATESLOTS\\$[\\n ]*"), QString::null);
    if (namespaceEnd.isEmpty())
        templ.replace(QRegExp("\\$NAMESPACEEND\\$[\\n ]*"), QString::null);
}    
    

void CppNewClassDialog::ClassGenerator::beautifySource(QString &templ, QString &header, QString &className, QString &namespaceBeg,
    QString &constructors, QString &advCpp, QString &namespaceEnd, QString &implementation)
{
    if (header.isEmpty())
        templ.replace(QRegExp("\\$HEADER\\$[\\n ]*"), QString::null);
    if (className.isEmpty())
        templ.replace(QRegExp("\\$CLASSNAME\\$[\\n ]*"), QString::null);
    if (namespaceBeg.isEmpty())
        templ.replace(QRegExp("\\$NAMESPACEBEG\\$[\\n ]*"), QString::null);
    if (constructors.isEmpty())
        templ.replace(QRegExp("\\$CONSTRUCTORDEFINITIONS\\$[\\n ]*"), QString::null);
    if (advCpp.isEmpty())
        templ.replace(QRegExp("\\$DEFINITIONS\\$[\\n ]*"), QString::null);
    if (namespaceEnd.isEmpty())
        templ.replace(QRegExp("\\$NAMESPACEEND\\$[\\n ]*"), QString::null);
    if (implementation.isEmpty())
        templ.replace(QRegExp("\\$FILENAME\\$[\\n ]*"), QString::null);
}

QString CppNewClassDialog::classNameFormatted( )
{
    return classNameFormatted(classname_edit->text());
}

QString CppNewClassDialog::classNameFormatted(const QString &name )
{
  QString temp = name.simplifyWhiteSpace();
  return temp.replace(QRegExp("template *<.*> *(class *)?"), "");
}


QString CppNewClassDialog::templateStrFormatted( )
{
  return templateStrFormatted(classname_edit->text());
}

QString CppNewClassDialog::templateStrFormatted(const QString &name )
{
  QString className = name.simplifyWhiteSpace();
  QString temp = className;
  className.replace(QRegExp("template *<.*> *(class *)?"), "");
  QString templateStr = temp.replace(QRegExp(QRegExp::escape(className)), "");
  templateStr.replace(QRegExp(" *class *$"), "");
  return templateStr;
}

QString CppNewClassDialog::templateParamsFormatted( )
{
  return templateParamsFormatted( classname_edit->text() );
}

QString CppNewClassDialog::templateParamsFormatted( const QString &name )
{
  QString className = name.simplifyWhiteSpace();
  QString temp = className;
  className.replace(QRegExp("template *<.*> *(class *)?"), "");
  QString templateStr = temp.replace(QRegExp(QRegExp::escape(className)), "");
  templateStr.replace(QRegExp(" *class *$"), "");

  QString templateParams = templateStr;
  templateParams.replace(QRegExp("^ *template *"), "");
  templateParams.replace(QRegExp(" *class *"), "");
  templateParams.simplifyWhiteSpace();

  return templateParams;
}

QString CppNewClassDialog::templateActualParamsFormatted( const QString & name)
{
  QString className = name.simplifyWhiteSpace();
  QString temp = className;
  className.replace(QRegExp("<.*> *"), "");
  QString templateStr = temp.replace(QRegExp(QRegExp::escape(className)), "");
  return templateStr;
}

void CppNewClassDialog::removeTemplateParams( QString & name)
{
  name.replace(QRegExp("<.*> *"), "");
}

#include "cppnewclassdlg.moc"
